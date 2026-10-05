using System;
using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    // Builds frequency-layout codestreams from per-tile subband bitstreams.
    // Index entries follow physical order: band-major for progressive mode,
    // tile-major for sequential mode.
    internal static class JxrFrequencyCodestreamWriter
    {
        internal static JxrError WriteGray(byte[][][] packets, int[][] bitCounts,
            int width, int height, byte dcQp, byte lpQp, byte hpQp,
            JxrGraySubbandMode subbands, bool scaled, int trim, int overlap,
            JxrTileLayout layout, bool progressive, out byte[] codestream)
        {
            return Write(packets, bitCounts, width, height, dcQp, lpQp, hpQp,
                subbands, scaled, trim, overlap, JxrChromaSubsampling.Yuv444,
                layout, progressive, false, null, out codestream);
        }

        internal static JxrError WriteRgb(byte[][][] packets, int[][] bitCounts,
            int width, int height, byte dcQp, byte lpQp, byte hpQp,
            JxrGraySubbandMode subbands, bool scaled, int trim, int overlap,
            JxrChromaSubsampling chroma, JxrTileLayout layout,
            bool progressive, out byte[] codestream)
        {
            return Write(packets, bitCounts, width, height, dcQp, lpQp, hpQp,
                subbands, scaled, trim, overlap, chroma, layout, progressive,
                true, null, out codestream);
        }

        internal static JxrError WriteRgbProfile(byte[][][] packets,
            int[][] bitCounts, int width, int height,
            JxrProfileEncodingSettings profile, out byte[] codestream)
        {
            if (profile == null) { codestream = null; return JxrError.InvalidArgument; }
            return Write(packets, bitCounts, width, height,
                profile.DcIndices[0], profile.LowpassIndices[0],
                profile.HighpassIndices[0], JxrGraySubbandMode.All,
                profile.ScaledArithmetic, 0, profile.Overlap,
                JxrChromaSubsampling.Yuv444, profile.TileLayout,
                profile.Progressive, true, profile, out codestream);
        }

        internal static JxrError WriteGrayProfile(byte[][][] packets,
            int[][] bitCounts, int width, int height,
            JxrProfileEncodingSettings profile, out byte[] codestream)
        {
            if (profile == null) { codestream = null; return JxrError.InvalidArgument; }
            return Write(packets, bitCounts, width, height,
                profile.DcIndices[0], profile.LowpassIndices[0],
                profile.HighpassIndices[0], JxrGraySubbandMode.All,
                profile.ScaledArithmetic, 0, profile.Overlap,
                JxrChromaSubsampling.Yuv444, profile.TileLayout,
                profile.Progressive, false, profile, out codestream);
        }

        private static JxrError Write(byte[][][] packets, int[][] bitCounts,
            int width, int height, byte dcQp, byte lpQp, byte hpQp,
            JxrGraySubbandMode subbands, bool scaled, int trim, int overlap,
            JxrChromaSubsampling chroma, JxrTileLayout layout,
            bool progressive, bool rgb,
            JxrProfileEncodingSettings profileSettings, out byte[] codestream)
        {
            codestream = null;
            if (packets == null || bitCounts == null || packets.Length != 4 ||
                bitCounts.Length != 4 || layout == null || trim < 0 || trim > 15)
                return JxrError.InvalidArgument;
            JxrTileGeometry tiles;
            JxrError error = JxrTileGeometry.Create(width, height, layout, out tiles);
            if (error != JxrError.None) return error;
            int tileCount = tiles.Columns * tiles.Rows;
            if (tileCount > Int32.MaxValue / 4)
                return JxrError.UnsupportedFeature;
            int[] bands = ActiveBands(subbands);
            for (int band = 0; band < 4; band++)
                if (packets[band] == null || bitCounts[band] == null ||
                    packets[band].Length != tileCount || bitCounts[band].Length != tileCount)
                    return JxrError.InvalidArgument;

            JxrBitWriter header = new JxrBitWriter();
            error = profileSettings != null ?
                (rgb ? JxrHeaderWriter.WriteRgbProfile(header, width, height,
                    profileSettings) :
                 JxrGrayProfileHeaderWriter.WriteGrayProfileHeader(header,
                    width, height, profileSettings)) :
                rgb ? JxrHeaderWriter.WriteRgbSpatial(header, width, height,
                    dcQp, lpQp, hpQp, subbands, scaled, trim, overlap, chroma, layout) :
                JxrHeaderWriter.WriteGraySpatial(header, width, height, dcQp,
                    lpQp, hpQp, subbands, scaled, trim, overlap, layout);
            if (error != JxrError.None) return error;
            byte[] headerBytes = header.ToArray();
            // Main header: bitstream-format=frequency, index-table-present=1.
            if (headerBytes.Length < 10) return JxrError.InvalidBitstream;
            headerBytes[9] = (byte)((headerBytes[9] & 0x83) | 0x44);

            List<int> orderBand = new List<int>();
            List<int> orderTile = new List<int>();
            if (progressive)
                for (int b = 0; b < bands.Length; b++)
                    for (int tile = 0; tile < tileCount; tile++)
                    { orderBand.Add(bands[b]); orderTile.Add(tile); }
            else
                for (int tile = 0; tile < tileCount; tile++)
                    for (int b = 0; b < bands.Length; b++)
                    { orderBand.Add(bands[b]); orderTile.Add(tile); }

            int[] packetLengths = new int[orderBand.Count];
            int[] lengthByBandTile = new int[4 * tileCount];
            uint[] offsetsByBandTile = new uint[4 * tileCount];
            long bytesTotal = 0;
            uint packetOffset = 0;
            for (int index = 0; index < packetLengths.Length; index++)
            {
                int band = orderBand[index], tile = orderTile[index];
                if (packets[band][tile] == null || bitCounts[band][tile] < 0 ||
                    bitCounts[band][tile] > (long)packets[band][tile].Length * 8)
                    return JxrError.InvalidArgument;
                long bits = 32L + bitCounts[band][tile] +
                    (band == 3 && trim != 0 ? 4 : 0);
                long length = (bits + 7) / 8;
                if (length > Int32.MaxValue) return JxrError.UnsupportedFeature;
                packetLengths[index] = (int)length;
                lengthByBandTile[band * tileCount + tile] = (int)length;
                offsetsByBandTile[band * tileCount + tile] = packetOffset;
                if (length > 4) bytesTotal += length;
                if (length > 4)
                {
                    if ((ulong)packetOffset + (ulong)length > UInt32.MaxValue)
                        return JxrError.UnsupportedFeature;
                    packetOffset += (uint)length;
                }
            }

            JxrBitWriter writer = new JxrBitWriter();
            for (int index = 0; index < headerBytes.Length; index++)
                writer.Write(headerBytes[index], 8);
            writer.Write(1, 16); // index table marker
            // The index table is tile-major even for progressive streams;
            // each entry points into its band-major physical packet group.
            for (int tile = 0; tile < tileCount; tile++)
                for (int bandIndex = 0; bandIndex < bands.Length; bandIndex++)
            {
                int band = bands[bandIndex];
                if (lengthByBandTile[band * tileCount + tile] <= 4)
                    JxrVariableLengthWordWriter.WriteEscape(writer, 0xff);
                else
                {
                    error = JxrVariableLengthWordWriter.Write(writer,
                        offsetsByBandTile[band * tileCount + tile]);
                    if (error != JxrError.None) return error;
                }
            }
            JxrVariableLengthWordWriter.WriteEscape(writer, 0xff);
            if ((long)writer.BitCount / 8 + bytesTotal > Int32.MaxValue)
                return JxrError.UnsupportedFeature;
            for (int index = 0; index < packetLengths.Length; index++)
            {
                if (packetLengths[index] <= 4) continue;
                int band = orderBand[index], tile = orderTile[index];
                error = JxrPacketWriter.WriteHeader(writer, tile & 31, band + 1);
                if (error != JxrError.None) return error;
                if (band == 3 && trim != 0) writer.Write((uint)trim, 4);
                JxrBitReader reader = new JxrBitReader(packets[band][tile]);
                for (int bit = 0; bit < bitCounts[band][tile]; bit++)
                {
                    uint value;
                    error = reader.ReadBits(1, out value);
                    if (error != JxrError.None) return error;
                    writer.Write(value, 1);
                }
                writer.AlignByte();
            }
            codestream = writer.ToArray();
            return JxrError.None;
        }

        private static int[] ActiveBands(JxrGraySubbandMode subbands)
        {
            if (subbands == JxrGraySubbandMode.DcOnly) return new int[] { 0 };
            if (subbands == JxrGraySubbandMode.NoHighpass) return new int[] { 0, 1 };
            if (subbands == JxrGraySubbandMode.NoFlexbits) return new int[] { 0, 1, 2 };
            return new int[] { 0, 1, 2, 3 };
        }
    }
}
