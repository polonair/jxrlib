using System;
using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    // Validated, explicit settings copied from a parsed source profile. Planar
    // alpha has an independent gray-plane settings object.
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
        internal JxrBitstreamLayout Layout;
        internal int Overlap;
        internal bool ScaledArithmetic;
        internal int Subversion;
        internal bool Progressive;
        internal JxrTileLayout TileLayout;
        internal Guid PixelFormatGuid;
        internal Guid ColorContainerPixelFormatGuid;
        internal float HorizontalDpi;
        internal float VerticalDpi;
        internal bool GrayPlane;
        internal JxrProfileEncodingSettings Alpha;
        internal JxrAlphaRangeInterpretation AlphaRangeInterpretation;

        internal static JxrError Create(JxrImage image, JxrSourceProfile profile,
            out JxrProfileEncodingSettings settings)
        {
            settings = null;
            if (image == null || profile == null) return JxrError.InvalidArgument;
            if (!profile.PacketSyntaxComplete ||
                !profile.MacroblockQuantizerMapComplete)
                return JxrError.UnsupportedFeature;

            JxrHeaders container = profile.Headers;
            if (container.ContainerKind != JxrContainerKind.TiffLike ||
                container.Main.Width != image.Width ||
                container.Main.Height != image.Height ||
                container.ContainerWidth != image.Width ||
                container.ContainerHeight != image.Height ||
                container.HorizontalDpi <= 0 || container.VerticalDpi <= 0 ||
                container.HasPlanarAlpha != profile.HasPlanarAlpha)
                return JxrError.UnsupportedFeature;

            JxrPixelFormat expectedFormat;
            Guid pixelFormatGuid;
            if (!TryGetPixelFormat(container.PixelFormatGuid, out expectedFormat,
                out pixelFormatGuid) || image.Format != expectedFormat)
                return JxrError.UnsupportedFeature;

            JxrProfileEncodingSettings colorSettings;
            JxrError error = CreatePlaneSettings(profile.ColorPlane,
                container, false, out colorSettings);
            if (error != JxrError.None) return error;
            colorSettings.PixelFormatGuid = pixelFormatGuid;
            colorSettings.ColorContainerPixelFormatGuid = profile.HasPlanarAlpha ?
                new Guid("6fddc324-4e03-4bfe-b185-3d77768dc90c") : pixelFormatGuid;
            colorSettings.HorizontalDpi = container.HorizontalDpi;
            colorSettings.VerticalDpi = container.VerticalDpi;
            colorSettings.AlphaRangeInterpretation =
                container.AlphaRangeInterpretation;

            if (profile.HasPlanarAlpha)
            {
                if (image.Format != JxrPixelFormat.Bgra32 &&
                    image.Format != JxrPixelFormat.Pbgra32 ||
                    profile.AlphaPlane.Headers.Main.Width != image.Width ||
                    profile.AlphaPlane.Headers.Main.Height != image.Height)
                    return JxrError.UnsupportedFeature;
                JxrProfileEncodingSettings alphaSettings;
                error = CreatePlaneSettings(profile.AlphaPlane,
                    profile.AlphaPlane.Headers, true, out alphaSettings);
                if (error != JxrError.None) return error;
                colorSettings.Alpha = alphaSettings;
            }
            else if (image.Format != JxrPixelFormat.Bgr24 &&
                image.Format != JxrPixelFormat.Rgb24)
                return JxrError.UnsupportedFeature;

            settings = colorSettings;
            return JxrError.None;
        }

        private static JxrError CreatePlaneSettings(
            JxrSourcePlaneProfile profilePlane, JxrHeaders headers,
            bool grayPlane, out JxrProfileEncodingSettings settings)
        {
            settings = null;
            if (profilePlane == null || headers == null ||
                !profilePlane.SyntaxComplete ||
                !profilePlane.MacroblockQuantizerMapComplete)
                return JxrError.UnsupportedFeature;

            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader quantizers = headers.Quantizers;
            int expectedSourceFormat = grayPlane ? 0 : 7;
            int expectedPlaneFormat = grayPlane ? 0 :
                (int)JxrChromaSubsampling.Yuv444;
            int expectedChannels = grayPlane ? 1 : 3;
            bool spatial = main.BitstreamFormat ==
                (int)JxrBitstreamLayout.Spatial;
            bool frequency = main.BitstreamFormat ==
                (int)JxrBitstreamLayout.Frequency;
            int expectedQuantizerMode = grayPlane ? 0 : 2;
            if (main.Version != 1 || (main.Subversion != 0 && main.Subversion != 1) ||
                (!spatial && !frequency) || main.HasHardTileBoundaries ||
                (frequency && !main.HasIndexTable) ||
                main.HasAlpha || main.Orientation != 0 || main.RedBlueSwapped ||
                main.Overlap < 0 || main.Overlap > 1 || main.TrimFlexbits ||
                main.SourceColorFormat != expectedSourceFormat ||
                main.SourceBitDepth != 1 || main.CodedBitDepth != 1 ||
                plane.ColorFormat != expectedPlaneFormat ||
                plane.ChannelCount != expectedChannels ||
                plane.Subband != (int)JxrGraySubbandMode.All ||
                plane.HasChromaCenteringX || plane.HasChromaCenteringY ||
                plane.HasSampleConversion || !quantizers.HasDc ||
                !quantizers.HasLowpass || !quantizers.HasHighpass ||
                (!spatial && quantizers.DcMode != expectedQuantizerMode) ||
                (!spatial && quantizers.LowpassMode != expectedQuantizerMode) ||
                (!spatial && quantizers.HighpassMode != expectedQuantizerMode) ||
                (spatial && (!SupportedQuantizerMode(quantizers.DcMode) ||
                    !SupportedQuantizerMode(quantizers.LowpassMode) ||
                    !SupportedQuantizerMode(quantizers.HighpassMode))) ||
                profilePlane.TileCount < 1 || profilePlane.TileCount > 4096)
                return JxrError.UnsupportedFeature;

            int tileColumns = main.VerticalSliceCountMinusOne + 1;
            int tileRows = main.HorizontalSliceCountMinusOne + 1;
            if ((spatial && (main.HasIndexTable || profilePlane.TileCount != 1 ||
                    tileColumns != 1 || tileRows != 1)) ||
                (frequency && !main.HasIndexTable))
                return JxrError.UnsupportedFeature;

            byte[] dc = ReadIndices(quantizers, 0, expectedChannels,
                quantizers.DcMode);
            byte[] lp = ReadIndices(quantizers, 1, expectedChannels,
                quantizers.LowpassMode);
            byte[] hp = ReadIndices(quantizers, 2, expectedChannels,
                quantizers.HighpassMode);
            if (!HasSupportedTileQuantizers(profilePlane, spatial))
                return JxrError.UnsupportedFeature;

            int columns = tileColumns;
            int rows = tileRows;
            if ((long)columns * rows != profilePlane.TileCount)
                return JxrError.InvalidBitstream;
            int[] columnWidths = new int[columns];
            int[] rowHeights = new int[rows];
            int totalColumns = ((int)main.Width + 15) / 16;
            int totalRows = ((int)main.Height + 15) / 16;
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
            if (JxrTileGeometry.Create((int)main.Width, (int)main.Height,
                layout, out geometry) != JxrError.None)
                return JxrError.InvalidBitstream;

            bool progressive = true;
            if (!spatial)
            {
                JxrError orderError = DetectProgressiveOrder(profilePlane,
                    out progressive);
                if (orderError != JxrError.None) return orderError;
            }

            settings = new JxrProfileEncodingSettings();
            settings.DcIndices = dc;
            settings.LowpassIndices = lp;
            settings.HighpassIndices = hp;
            settings.DcMode = quantizers.DcMode;
            settings.LowpassMode = quantizers.LowpassMode;
            settings.HighpassMode = quantizers.HighpassMode;
            settings.Layout = (JxrBitstreamLayout)main.BitstreamFormat;
            settings.Overlap = main.Overlap;
            settings.ScaledArithmetic = plane.ScaledArithmetic;
            settings.Subversion = main.Subversion;
            settings.Progressive = progressive;
            settings.TileLayout = layout;
            settings.GrayPlane = grayPlane;
            return JxrError.None;
        }

        private static bool SupportedQuantizerMode(int mode)
        { return mode >= 0 && mode <= 3; }

        private static byte[] ReadIndices(JxrImagePlaneQuantizerHeader quantizers,
            int band, int count, int mode)
        {
            byte[] values = new byte[count];
            byte first;
            if (band == 0) first = quantizers.GetDcIndex(0);
            else if (band == 1) first = quantizers.GetLowpassIndex(0);
            else first = quantizers.GetHighpassIndex(0);
            values[0] = first;
            for (int channel = 1; channel < count; channel++)
            {
                if (mode == 0) values[channel] = first;
                else if (mode == 1)
                {
                    values[channel] = band == 0 ? quantizers.GetDcIndex(1) :
                        band == 1 ? quantizers.GetLowpassIndex(1) :
                        quantizers.GetHighpassIndex(1);
                }
                else values[channel] = band == 0 ? quantizers.GetDcIndex(channel) :
                    band == 1 ? quantizers.GetLowpassIndex(channel) :
                    quantizers.GetHighpassIndex(channel);
            }
            return values;
        }

        private static bool HasSupportedTileQuantizers(
            JxrSourcePlaneProfile plane, bool spatial)
        {
            for (int tileIndex = 0; tileIndex < plane.TileCount; tileIndex++)
            {
                JxrProfileTile tile = plane.GetTile(tileIndex);
                if (tile.HasTrimFlexbits || tile.TrimFlexbits != 0 ||
                    tile.DcQuantizers != null || tile.LowpassQuantizers != null ||
                    tile.HighpassQuantizers != null ||
                    (spatial ? tile.PacketCount != 1 ||
                        tile.GetPacket(0).Type != 0 :
                        tile.PacketCount < 3 || tile.PacketCount > 4))
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
            if (guid == new Guid("6fddc324-4e03-4bfe-b185-3d77768dc90f"))
            { format = JxrPixelFormat.Bgra32; return true; }
            if (guid == new Guid("6fddc324-4e03-4bfe-b185-3d77768dc910"))
            { format = JxrPixelFormat.Pbgra32; return true; }
            return false;
        }
    }
}
