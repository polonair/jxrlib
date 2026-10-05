using System;
using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    // Restricted adapter from a parsed source profile to the common corpus
    // encoder. The profile is validated before any pixels are transformed.
    internal sealed class JxrProfileEncodingSettings
    {
        private sealed class OrderedPacket
        {
            internal int TileIndex;
            internal JxrProfilePacket Packet;
            internal OrderedPacket(int tileIndex, JxrProfilePacket packet)
            { TileIndex = tileIndex; Packet = packet; }
        }
        internal byte[] DcIndices;
        internal byte[] LowpassIndices;
        internal byte[] HighpassIndices;
        internal int DcMode;
        internal int LowpassMode;
        internal int HighpassMode;
        internal int Overlap;
        internal bool ScaledArithmetic;
        internal int Subversion;
        internal bool Progressive;
        internal JxrTileLayout TileLayout;
        internal Guid PixelFormatGuid;
        internal float HorizontalDpi;
        internal float VerticalDpi;

        internal static JxrError Create(JxrImage image, JxrSourceProfile profile,
            out JxrProfileEncodingSettings settings)
        {
            settings = null;
            if (image == null || profile == null) return JxrError.InvalidArgument;
            if (!profile.PacketSyntaxComplete ||
                !profile.MacroblockQuantizerMapComplete)
                return JxrError.UnsupportedFeature;

            JxrHeaders headers = profile.Headers;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader quantizers = headers.Quantizers;
            if (headers.ContainerKind != JxrContainerKind.TiffLike ||
                headers.HasPlanarAlpha || main.Version != 1 ||
                (main.Subversion != 0 && main.Subversion != 1) ||
                main.BitstreamFormat != (int)JxrBitstreamLayout.Frequency ||
                main.HasHardTileBoundaries || !main.HasIndexTable ||
                main.HasAlpha || main.Orientation != 0 || main.RedBlueSwapped ||
                main.Overlap < 0 || main.Overlap > 1 || main.TrimFlexbits ||
                main.Width != image.Width || main.Height != image.Height ||
                plane.ColorFormat != (int)JxrChromaSubsampling.Yuv444 ||
                plane.ChannelCount != 3 || plane.Subband != (int)JxrGraySubbandMode.All ||
                plane.HasChromaCenteringX || plane.HasChromaCenteringY ||
                plane.HasSampleConversion || main.SourceColorFormat != 7 ||
                main.SourceBitDepth != 1 || main.CodedBitDepth != 1 ||
                !quantizers.HasDc || !quantizers.HasLowpass ||
                !quantizers.HasHighpass ||
                quantizers.DcMode != 2 || quantizers.LowpassMode != 2 ||
                quantizers.HighpassMode != 2 ||
                profile.ColorPlane.TileCount < 1 ||
                profile.ColorPlane.TileCount > 4096)
                return JxrError.UnsupportedFeature;

            JxrPixelFormat expectedFormat;
            Guid pixelFormatGuid;
            if (!TryGetPixelFormat(headers.PixelFormatGuid, out expectedFormat,
                out pixelFormatGuid) || image.Format != expectedFormat)
                return JxrError.UnsupportedFeature;
            if (headers.ContainerWidth != image.Width ||
                headers.ContainerHeight != image.Height ||
                headers.HorizontalDpi <= 0 || headers.VerticalDpi <= 0)
                return JxrError.UnsupportedFeature;

            byte[] dc = ReadIndices(quantizers, 0, 3);
            byte[] lp = ReadIndices(quantizers, 1, 3);
            byte[] hp = ReadIndices(quantizers, 2, 3);
            if (dc == null || lp == null || hp == null ||
                !HasSupportedTileQuantizers(profile.ColorPlane))
                return JxrError.UnsupportedFeature;

            int columns = main.VerticalSliceCountMinusOne + 1;
            int rows = main.HorizontalSliceCountMinusOne + 1;
            if ((long)columns * rows != profile.ColorPlane.TileCount)
                return JxrError.InvalidBitstream;
            int[] columnWidths = new int[columns];
            int[] rowHeights = new int[rows];
            int totalColumns = (image.Width + 15) / 16;
            int totalRows = (image.Height + 15) / 16;
            for (int column = 0; column < columns; column++)
            {
                int end = column + 1 < columns ? main.GetTileX(column + 1) :
                    totalColumns;
                columnWidths[column] = end - main.GetTileX(column);
            }
            for (int row = 0; row < rows; row++)
            {
                int end = row + 1 < rows ? main.GetTileY(row + 1) : totalRows;
                rowHeights[row] = end - main.GetTileY(row);
            }
            JxrTileLayout layout = new JxrTileLayout(columnWidths, rowHeights);
            JxrTileGeometry geometry;
            if (JxrTileGeometry.Create(image.Width, image.Height, layout,
                out geometry) != JxrError.None)
                return JxrError.InvalidBitstream;

            bool progressive;
            JxrError orderError = DetectProgressiveOrder(profile.ColorPlane,
                out progressive);
            if (orderError != JxrError.None) return orderError;

            settings = new JxrProfileEncodingSettings();
            settings.DcIndices = dc;
            settings.LowpassIndices = lp;
            settings.HighpassIndices = hp;
            settings.DcMode = quantizers.DcMode;
            settings.LowpassMode = quantizers.LowpassMode;
            settings.HighpassMode = quantizers.HighpassMode;
            settings.Overlap = main.Overlap;
            settings.ScaledArithmetic = plane.ScaledArithmetic;
            settings.Subversion = main.Subversion;
            settings.Progressive = progressive;
            settings.TileLayout = layout;
            settings.PixelFormatGuid = pixelFormatGuid;
            settings.HorizontalDpi = headers.HorizontalDpi;
            settings.VerticalDpi = headers.VerticalDpi;
            return JxrError.None;
        }

        private static byte[] ReadIndices(JxrImagePlaneQuantizerHeader quantizers,
            int band, int count)
        {
            byte[] values = new byte[count];
            for (int channel = 0; channel < count; channel++)
            {
                if (band == 0) values[channel] = quantizers.GetDcIndex(channel);
                else if (band == 1)
                    values[channel] = quantizers.GetLowpassIndex(channel);
                else values[channel] = quantizers.GetHighpassIndex(channel);
            }
            return values;
        }

        private static bool HasSupportedTileQuantizers(
            JxrSourcePlaneProfile plane)
        {
            for (int tileIndex = 0; tileIndex < plane.TileCount; tileIndex++)
            {
                JxrProfileTile tile = plane.GetTile(tileIndex);
                if (tile.HasTrimFlexbits || tile.TrimFlexbits != 0 ||
                    tile.DcQuantizers != null || tile.LowpassQuantizers != null ||
                    tile.HighpassQuantizers != null || tile.PacketCount < 3 ||
                    tile.PacketCount > 4)
                    return false;
            }
            return true;
        }

        private static JxrError DetectProgressiveOrder(JxrSourcePlaneProfile plane,
            out bool progressive)
        {
            progressive = true;
            List<OrderedPacket> packets = new List<OrderedPacket>();
            for (int tile = 0; tile < plane.TileCount; tile++)
                for (int packet = 0; packet < plane.GetTile(tile).PacketCount; packet++)
                    packets.Add(new OrderedPacket(tile,
                        plane.GetTile(tile).GetPacket(packet)));
            packets.Sort(delegate(OrderedPacket left, OrderedPacket right)
            { return left.Packet.Offset.CompareTo(right.Packet.Offset); });
            bool[,] present = new bool[plane.TileCount, 5];
            for (int packet = 0; packet < packets.Count; packet++)
            {
                int type = packets[packet].Packet.Type;
                int tile = packets[packet].TileIndex;
                if (type < 1 || type > 4 ||
                    packets[packet].Packet.TileId != plane.GetTile(tile).Id ||
                    present[tile, type])
                    return JxrError.InvalidBitstream;
                present[tile, type] = true;
            }
            for (int tile = 0; tile < plane.TileCount; tile++)
                if (!present[tile, 1] || !present[tile, 2] ||
                    !present[tile, 3]) return JxrError.UnsupportedFeature;

            List<OrderedPacket> progressiveOrder = new List<OrderedPacket>();
            for (int band = 1; band <= 4; band++)
                for (int tile = 0; tile < plane.TileCount; tile++)
                    if (present[tile, band])
                        progressiveOrder.Add(new OrderedPacket(tile,
                            FindPacket(plane, tile, band)));
            List<OrderedPacket> sequentialOrder = new List<OrderedPacket>();
            for (int tile = 0; tile < plane.TileCount; tile++)
                for (int band = 1; band <= 4; band++)
                    if (present[tile, band])
                        sequentialOrder.Add(new OrderedPacket(tile,
                            FindPacket(plane, tile, band)));
            bool matchesProgressive = MatchesOrder(packets, progressiveOrder);
            bool matchesSequential = MatchesOrder(packets, sequentialOrder);
            if (!matchesProgressive && !matchesSequential)
                return JxrError.UnsupportedFeature;
            progressive = matchesProgressive;
            return JxrError.None;
        }

        private static JxrProfilePacket FindPacket(JxrSourcePlaneProfile plane,
            int tileIndex, int type)
        {
            JxrProfileTile tile = plane.GetTile(tileIndex);
            for (int packet = 0; packet < tile.PacketCount; packet++)
                if (tile.GetPacket(packet).Type == type)
                    return tile.GetPacket(packet);
            return null;
        }

        private static bool MatchesOrder(List<OrderedPacket> actual,
            List<OrderedPacket> expected)
        {
            if (actual.Count != expected.Count) return false;
            for (int index = 0; index < actual.Count; index++)
                if (actual[index].Packet.Type != expected[index].Packet.Type ||
                    actual[index].TileIndex != expected[index].TileIndex)
                    return false;
            return true;
        }

        private static bool TryGetPixelFormat(string value,
            out JxrPixelFormat format, out Guid guid)
        {
            format = JxrPixelFormat.Bgr24;
            guid = Guid.Empty;
            if (String.IsNullOrEmpty(value)) return false;
            try { guid = new Guid(value); }
            catch (FormatException) { return false; }
            if (guid == new Guid("6fddc324-4e03-4bfe-b185-3d77768dc90c"))
            { format = JxrPixelFormat.Bgr24; return true; }
            if (guid == new Guid("6fddc324-4e03-4bfe-b185-3d77768dc90d"))
            { format = JxrPixelFormat.Rgb24; return true; }
            return false;
        }
    }
}
