using System;

namespace Jxr.Managed.Core
{
    // Field-by-field counterpart of the native main/image-plane writers.
    // Single spatial packet for Gray8 or full-resolution RGB24/YUV444.
    public static class JxrHeaderWriter
    {
        public static JxrError WriteRgbSpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits)
        {
            return WriteRgbSpatial(writer, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, 0);
        }

        public static JxrError WriteRgbSpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits,
            int overlap)
        {
            return WriteRgbSpatial(writer, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, overlap,
                JxrChromaSubsampling.Yuv444);
        }

        public static JxrError WriteRgbSpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits,
            int overlap, JxrChromaSubsampling chromaSubsampling)
        {
            return WriteRgbSpatial(writer, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, overlap, chromaSubsampling,
                null);
        }

        public static JxrError WriteRgbSpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits,
            int overlap, JxrChromaSubsampling chromaSubsampling,
            JxrTileLayout tileLayout)
        {
            return WriteRgbSpatialCore(writer, width, height,
                new byte[] { dcQuantizerIndex, dcQuantizerIndex, dcQuantizerIndex },
                new byte[] { lowpassQuantizerIndex, lowpassQuantizerIndex, lowpassQuantizerIndex },
                new byte[] { highpassQuantizerIndex, highpassQuantizerIndex, highpassQuantizerIndex },
                2, 2, 2, subbands, scaledArithmetic, trimFlexbits, overlap,
                chromaSubsampling, tileLayout, 1);
        }

        internal static JxrError WriteRgbProfile(JxrBitWriter writer,
            int width, int height, JxrProfileEncodingSettings profile)
        {
            if (profile == null) return JxrError.InvalidArgument;
            return WriteRgbSpatialCore(writer, width, height,
                profile.DcIndices, profile.LowpassIndices,
                profile.HighpassIndices, profile.DcMode,
                profile.LowpassMode, profile.HighpassMode,
                JxrGraySubbandMode.All, profile.ScaledArithmetic, 0,
                profile.Overlap,
                JxrChromaSubsampling.Yuv444, profile.TileLayout,
                profile.Subversion);
        }

        internal static JxrError WriteRgbSpatialProfile(JxrBitWriter writer,
            int width, int height, JxrProfileEncodingSettings profile)
        {
            if (profile == null || profile.Layout != JxrBitstreamLayout.Spatial ||
                profile.TileLayout == null || !profile.TileLayout.IsSingleTile)
                return JxrError.UnsupportedFeature;
            return WriteRgbProfile(writer, width, height, profile);
        }

        private static JxrError WriteRgbSpatialCore(JxrBitWriter writer,
            int width, int height, byte[] dcQuantizerIndices,
            byte[] lowpassQuantizerIndices, byte[] highpassQuantizerIndices,
            int dcQuantizerMode, int lowpassQuantizerMode,
            int highpassQuantizerMode, JxrGraySubbandMode subbands,
            bool scaledArithmetic, int trimFlexbits, int overlap,
            JxrChromaSubsampling chromaSubsampling, JxrTileLayout tileLayout,
            int subversion)
        {
            if (writer == null || width < 1 || height < 1 ||
                dcQuantizerIndices == null || dcQuantizerIndices.Length != 3 ||
                lowpassQuantizerIndices == null || lowpassQuantizerIndices.Length != 3 ||
                highpassQuantizerIndices == null || highpassQuantizerIndices.Length != 3 ||
                dcQuantizerMode < 0 || dcQuantizerMode > 3 ||
                lowpassQuantizerMode < 0 || lowpassQuantizerMode > 3 ||
                highpassQuantizerMode < 0 || highpassQuantizerMode > 3 ||
                (subversion != 0 && subversion != 1) ||
                writer.BitCount != 0 || (int)subbands < 0 || (int)subbands > 3 ||
                trimFlexbits < 0 || trimFlexbits > 15 || overlap < 0 || overlap > 2 ||
                (int)chromaSubsampling < 1 || (int)chromaSubsampling > 3)
                return JxrError.InvalidArgument;
            JxrTileGeometry tiles;
            JxrError tileError = JxrTileGeometry.Create(width, height,
                tileLayout, out tiles);
            if (tileError != JxrError.None) return tileError;
            bool tiled = !tiles.IsSingleTile;
            bool abbreviated = ((long)width + 15) / 16 <= 255 &&
                ((long)height + 15) / 16 <= 255;
            byte[] signature = { (byte)'W', (byte)'M', (byte)'P', (byte)'H',
                (byte)'O', (byte)'T', (byte)'O', 0 };
            for (int index = 0; index < signature.Length; index++)
                writer.Write(signature[index], 8);
            writer.Write(1, 4); writer.Write((uint)subversion, 4);
            writer.Write(tiled ? 1U : 0U, 1); writer.Write(0, 1); // tiling, spatial
            writer.Write(0, 3); writer.Write(tiled ? 1U : 0U, 1); // orientation, index
            writer.Write((uint)overlap, 2); writer.Write(abbreviated ? 1U : 0U, 1);
            writer.Write(1, 1); writer.Write(0, 1); // short header, no window
            writer.Write(trimFlexbits != 0 ? 1U : 0U, 1);
            writer.Write(0, 1); writer.Write(0, 2); writer.Write(0, 1);
            writer.Write(7, 4); writer.Write(1, 4); // CF_RGB, BD_8
            writer.Write((uint)(width - 1), abbreviated ? 16 : 32);
            writer.Write((uint)(height - 1), abbreviated ? 16 : 32);
            if (tiled)
            {
                writer.Write((uint)(tiles.Columns - 1), 12);
                writer.Write((uint)(tiles.Rows - 1), 12);
                int[] columnWidths = tiles.CopyColumnWidths();
                int[] rowHeights = tiles.CopyRowHeights();
                int tileSizeBits = abbreviated ? 8 : 16;
                for (int index = 0; index < columnWidths.Length - 1; index++)
                    writer.Write((uint)columnWidths[index], tileSizeBits);
                for (int index = 0; index < rowHeights.Length - 1; index++)
                    writer.Write((uint)rowHeights[index], tileSizeBits);
            }
            writer.AlignByte();

            writer.Write((uint)chromaSubsampling, 3);
            writer.Write(scaledArithmetic ? 1U : 0U, 1);
            writer.Write((uint)subbands, 4);
            writer.Write(0, 8); // no chroma centering
            writer.Write(1, 1); // frame DC quantizer
            WriteThreeChannelQuantizer(writer, dcQuantizerMode,
                dcQuantizerIndices);
            if (subbands != JxrGraySubbandMode.DcOnly)
            {
                writer.Write(0, 1); writer.Write(1, 1);
                WriteThreeChannelQuantizer(writer, lowpassQuantizerMode,
                    lowpassQuantizerIndices);
            }
            if ((int)subbands < (int)JxrGraySubbandMode.NoHighpass)
            {
                writer.Write(0, 1); writer.Write(1, 1);
                WriteThreeChannelQuantizer(writer, highpassQuantizerMode,
                    highpassQuantizerIndices);
            }
            writer.AlignByte();
            return JxrError.None;
        }

        private static void WriteThreeChannelQuantizer(JxrBitWriter writer,
            int mode, byte[] indices)
        {
            writer.Write((uint)mode, 2);
            writer.Write(indices[0], 8);
            if (mode == 1) writer.Write(indices[1], 8);
            else if (mode > 1)
                for (int channel = 1; channel < 3; channel++)
                    writer.Write(indices[channel], 8);
        }

        public static JxrError WriteGraySpatial(JxrBitWriter writer,
            int width, int height, byte quantizerIndex)
        {
            return WriteGraySpatial(writer, width, height, quantizerIndex,
                quantizerIndex, quantizerIndex, JxrGraySubbandMode.All, false, 0);
        }

        public static JxrError WriteGraySpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits)
        {
            return WriteGraySpatial(writer, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, 0);
        }

        public static JxrError WriteGraySpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits,
            int overlap)
        {
            return WriteGraySpatial(writer, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, overlap, null);
        }

        public static JxrError WriteGraySpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits,
            int overlap, JxrTileLayout tileLayout)
        {
            if (writer == null || width < 1 || height < 1 ||
                writer.BitCount != 0 || (int)subbands < 0 || (int)subbands > 3 ||
                trimFlexbits < 0 || trimFlexbits > 15 || overlap < 0 || overlap > 2)
                return JxrError.InvalidArgument;
            JxrTileGeometry tiles;
            JxrError tileError = JxrTileGeometry.Create(width, height,
                tileLayout, out tiles);
            if (tileError != JxrError.None) return tileError;
            bool tiled = !tiles.IsSingleTile;
            bool abbreviated = ((long)width + 15) / 16 <= 255 &&
                ((long)height + 15) / 16 <= 255;
            byte[] signature = { (byte)'W', (byte)'M', (byte)'P', (byte)'H',
                (byte)'O', (byte)'T', (byte)'O', 0 };
            for (int index = 0; index < signature.Length; index++)
                writer.Write(signature[index], 8);

            writer.Write(1, 4);  // version
            writer.Write(1, 4);  // new-scaling, soft-tile subversion
            writer.Write(tiled ? 1U : 0U, 1);
            writer.Write(0, 1);  // spatial layout
            writer.Write(0, 3);  // orientation
            writer.Write(tiled ? 1U : 0U, 1);
            writer.Write((uint)overlap, 2);
            writer.Write(abbreviated ? 1U : 0U, 1);
            writer.Write(1, 1);  // short header flag used by native encoder
            writer.Write(0, 1);  // no windowing
            writer.Write(trimFlexbits != 0 ? 1U : 0U, 1);
            writer.Write(0, 1);  // no tile stretching
            writer.Write(0, 2);  // reserved and red/blue swap
            writer.Write(0, 1);  // no alpha
            writer.Write(0, 4);  // Y_ONLY source color format
            writer.Write(1, 4);  // 8-bit source depth
            writer.Write((uint)(width - 1), abbreviated ? 16 : 32);
            writer.Write((uint)(height - 1), abbreviated ? 16 : 32);
            if (tiled)
            {
                writer.Write((uint)(tiles.Columns - 1), 12);
                writer.Write((uint)(tiles.Rows - 1), 12);
                int[] columnWidths = tiles.CopyColumnWidths();
                int[] rowHeights = tiles.CopyRowHeights();
                int tileSizeBits = abbreviated ? 8 : 16;
                for (int index = 0; index < columnWidths.Length - 1; index++)
                    writer.Write((uint)columnWidths[index], tileSizeBits);
                for (int index = 0; index < rowHeights.Length - 1; index++)
                    writer.Write((uint)rowHeights[index], tileSizeBits);
            }
            writer.AlignByte();

            writer.Write(0, 3);  // Y_ONLY plane
            writer.Write(scaledArithmetic ? 1U : 0U, 1);
            writer.Write((uint)subbands, 4);
            writer.Write(1, 1); writer.Write(dcQuantizerIndex, 8); // DC frame QP
            if (subbands != JxrGraySubbandMode.DcOnly)
            {
                writer.Write(0, 1); writer.Write(1, 1);          // LP own frame QP
                writer.Write(lowpassQuantizerIndex, 8);
            }
            if ((int)subbands < (int)JxrGraySubbandMode.NoHighpass)
            {
                writer.Write(0, 1); writer.Write(1, 1);          // HP own frame QP
                writer.Write(highpassQuantizerIndex, 8);
            }
            writer.AlignByte();
            return JxrError.None;
        }
    }

    internal static class JxrGrayProfileHeaderWriter
    {
        internal static JxrError WriteGrayProfileHeader(JxrBitWriter writer,
        int width, int height, JxrProfileEncodingSettings profile)
    {
        if (writer == null || profile == null || !profile.GrayPlane ||
            width < 1 || height < 1 || writer.BitCount != 0 ||
            profile.DcIndices == null || profile.DcIndices.Length != 1 ||
            profile.LowpassIndices == null || profile.LowpassIndices.Length != 1 ||
            profile.HighpassIndices == null || profile.HighpassIndices.Length != 1 ||
            profile.DcMode != 0 || profile.LowpassMode != 0 ||
            profile.HighpassMode != 0 || profile.Subversion < 0 ||
            profile.Subversion > 1 || profile.Overlap < 0 ||
            profile.Overlap > 1 || profile.TileLayout == null ||
            (profile.Layout != JxrBitstreamLayout.Spatial &&
             profile.Layout != JxrBitstreamLayout.Frequency))
            return JxrError.InvalidArgument;
        JxrTileGeometry tiles;
        JxrError tileError = JxrTileGeometry.Create(width, height,
            profile.TileLayout, out tiles);
        if (tileError != JxrError.None) return tileError;
        bool tiled = !tiles.IsSingleTile;
        if (profile.Layout == JxrBitstreamLayout.Spatial && tiled)
            return JxrError.UnsupportedFeature;
        bool abbreviated = ((long)width + 15) / 16 <= 255 &&
            ((long)height + 15) / 16 <= 255;
        byte[] signature = { (byte)'W', (byte)'M', (byte)'P', (byte)'H',
            (byte)'O', (byte)'T', (byte)'O', 0 };
        for (int index = 0; index < signature.Length; index++)
            writer.Write(signature[index], 8);
        writer.Write(1, 4);
        writer.Write((uint)profile.Subversion, 4);
        writer.Write(tiled ? 1U : 0U, 1);
        writer.Write(profile.Layout == JxrBitstreamLayout.Frequency ? 1U : 0U, 1);
        writer.Write(0, 3); // orientation
        writer.Write(profile.Layout == JxrBitstreamLayout.Frequency || tiled ?
            1U : 0U, 1);
        writer.Write((uint)profile.Overlap, 2);
        writer.Write(abbreviated ? 1U : 0U, 1);
        writer.Write(1, 1); // short header
        writer.Write(0, 1); // no windowing
        writer.Write(0, 1); // no trim flexbits
        writer.Write(0, 1); // no tile stretching
        writer.Write(0, 2); // reserved and red/blue swap
        writer.Write(0, 1); // no interleaved alpha
        writer.Write(0, 4); // Y_ONLY source
        writer.Write(1, 4); // 8-bit source
        writer.Write((uint)(width - 1), abbreviated ? 16 : 32);
        writer.Write((uint)(height - 1), abbreviated ? 16 : 32);
        if (tiled)
        {
            writer.Write((uint)(tiles.Columns - 1), 12);
            writer.Write((uint)(tiles.Rows - 1), 12);
            int[] columnWidths = tiles.CopyColumnWidths();
            int[] rowHeights = tiles.CopyRowHeights();
            int tileSizeBits = abbreviated ? 8 : 16;
            for (int index = 0; index < columnWidths.Length - 1; index++)
                writer.Write((uint)columnWidths[index], tileSizeBits);
            for (int index = 0; index < rowHeights.Length - 1; index++)
                writer.Write((uint)rowHeights[index], tileSizeBits);
        }
        writer.AlignByte();
        writer.Write(0, 3); // Y_ONLY plane
        writer.Write(profile.ScaledArithmetic ? 1U : 0U, 1);
        writer.Write((uint)JxrGraySubbandMode.All, 4);
        writer.Write(1, 1); writer.Write(profile.DcIndices[0], 8);
        writer.Write(0, 1); writer.Write(1, 1);
        writer.Write(profile.LowpassIndices[0], 8);
        writer.Write(0, 1); writer.Write(1, 1);
        writer.Write(profile.HighpassIndices[0], 8);
        writer.AlignByte();
        return JxrError.None;
    }

    internal static JxrError WriteGraySpatialProfile(JxrBitWriter writer,
        int width, int height, JxrProfileEncodingSettings profile)
    {
        if (profile == null || profile.Layout != JxrBitstreamLayout.Spatial ||
            profile.TileLayout == null || !profile.TileLayout.IsSingleTile)
            return JxrError.UnsupportedFeature;
        return WriteGrayProfileHeader(writer, width, height, profile);
    }
    }

    public static class JxrPacketWriter
    {
        public static JxrError WriteHeader(JxrBitWriter writer,
            int tileId, int packetType)
        {
            if (writer == null || (writer.BitCount & 7) != 0 ||
                tileId < 0 || tileId > 31 || packetType < 0 || packetType > 4)
                return JxrError.InvalidArgument;
            writer.Write(0, 8);
            writer.Write(0, 8);
            writer.Write(1, 8);
            writer.Write((uint)((tileId << 3) | packetType), 8);
            return JxrError.None;
        }
    }

    public static class JxrCodestreamWriter
    {
        internal static JxrError WriteGraySpatialTiles(byte[][] entropyPackets,
            int[] entropyBitCounts, int width, int height,
            byte dcQuantizerIndex, byte lowpassQuantizerIndex,
            byte highpassQuantizerIndex, JxrGraySubbandMode subbands,
            bool scaledArithmetic, int trimFlexbits, int overlap,
            JxrTileLayout tileLayout, out byte[] codestream)
        {
            return WriteSpatialTiles(entropyPackets, entropyBitCounts, width,
                height, dcQuantizerIndex, lowpassQuantizerIndex,
                highpassQuantizerIndex, subbands, scaledArithmetic,
                trimFlexbits, overlap, JxrChromaSubsampling.Yuv444,
                tileLayout, false, out codestream);
        }

        internal static JxrError WriteRgbSpatialTiles(byte[][] entropyPackets,
            int[] entropyBitCounts, int width, int height,
            byte dcQuantizerIndex, byte lowpassQuantizerIndex,
            byte highpassQuantizerIndex, JxrGraySubbandMode subbands,
            bool scaledArithmetic, int trimFlexbits, int overlap,
            JxrChromaSubsampling chromaSubsampling,
            JxrTileLayout tileLayout, out byte[] codestream)
        {
            return WriteSpatialTiles(entropyPackets, entropyBitCounts, width,
                height, dcQuantizerIndex, lowpassQuantizerIndex,
                highpassQuantizerIndex, subbands, scaledArithmetic,
                trimFlexbits, overlap, chromaSubsampling, tileLayout,
                true, out codestream);
        }

        internal static JxrError WriteGraySpatialProfile(byte[] entropyPacket,
            int entropyBitCount, int width, int height,
            JxrProfileEncodingSettings profile, out byte[] codestream)
        {
            return WriteSpatialProfile(entropyPacket, entropyBitCount, width,
                height, profile, false, out codestream);
        }

        internal static JxrError WriteRgbSpatialProfile(byte[] entropyPacket,
            int entropyBitCount, int width, int height,
            JxrProfileEncodingSettings profile, out byte[] codestream)
        {
            return WriteSpatialProfile(entropyPacket, entropyBitCount, width,
                height, profile, true, out codestream);
        }

        private static JxrError WriteSpatialProfile(byte[] entropyPacket,
            int entropyBitCount, int width, int height,
            JxrProfileEncodingSettings profile, bool rgb, out byte[] codestream)
        {
            codestream = null;
            if (profile == null || profile.Layout != JxrBitstreamLayout.Spatial ||
                profile.TileLayout == null || !profile.TileLayout.IsSingleTile ||
                profile.Overlap < 0 || profile.Overlap > 1 ||
                entropyPacket == null || entropyBitCount < 0 ||
                entropyBitCount > (long)entropyPacket.Length * 8)
                return JxrError.InvalidArgument;
            JxrBitWriter writer = new JxrBitWriter();
            JxrError error = rgb ? JxrHeaderWriter.WriteRgbSpatialProfile(writer,
                width, height, profile) :
                JxrGrayProfileHeaderWriter.WriteGraySpatialProfile(writer,
                    width, height, profile);
            if (error != JxrError.None) return error;
            JxrBitWriter marker = new JxrBitWriter();
            marker.Write(0x6f, 8);
            marker.Write(0xff, 8);
            marker.Write(1, 16);
            byte[] markerBytes = marker.ToArray();
            error = JxrVariableLengthWordWriter.Write(writer,
                (uint)markerBytes.Length);
            if (error != JxrError.None) return error;
            for (int index = 0; index < markerBytes.Length; index++)
                writer.Write(markerBytes[index], 8);
            error = JxrPacketWriter.WriteHeader(writer, 0, 0);
            if (error != JxrError.None) return error;
            JxrBitReader entropyReader = new JxrBitReader(entropyPacket);
            int remainingBits = entropyBitCount;
            while (remainingBits > 0)
            {
                uint bit;
                error = entropyReader.ReadBits(1, out bit);
                if (error != JxrError.None) return error;
                error = writer.Write(bit, 1);
                if (error != JxrError.None) return error;
                remainingBits--;
            }
            writer.AlignByte();
            codestream = writer.ToArray();
            return JxrError.None;
        }

        private static JxrError WriteSpatialTiles(byte[][] entropyPackets,
            int[] entropyBitCounts, int width, int height,
            byte dcQuantizerIndex, byte lowpassQuantizerIndex,
            byte highpassQuantizerIndex, JxrGraySubbandMode subbands,
            bool scaledArithmetic, int trimFlexbits, int overlap,
            JxrChromaSubsampling chromaSubsampling,
            JxrTileLayout tileLayout, bool rgb, out byte[] codestream)
        {
            codestream = null;
            if (entropyPackets == null || entropyBitCounts == null ||
                entropyPackets.Length != entropyBitCounts.Length ||
                tileLayout == null) return JxrError.InvalidArgument;
            JxrTileGeometry tiles;
            JxrError error = JxrTileGeometry.Create(width, height,
                tileLayout, out tiles);
            if (error != JxrError.None) return error;
            if (tiles.IsSingleTile)
            {
                if (entropyPackets.Length != 1) return JxrError.InvalidArgument;
                return rgb ? WriteRgbSpatial(entropyPackets[0], width, height,
                    dcQuantizerIndex, lowpassQuantizerIndex,
                    highpassQuantizerIndex, subbands, scaledArithmetic,
                    trimFlexbits, entropyBitCounts[0], overlap,
                    chromaSubsampling, out codestream) :
                    WriteGraySpatial(entropyPackets[0], width, height,
                        dcQuantizerIndex, lowpassQuantizerIndex,
                        highpassQuantizerIndex, subbands, scaledArithmetic,
                        trimFlexbits, entropyBitCounts[0], overlap,
                        out codestream);
            }
            if (entropyPackets.Length != tiles.Columns * tiles.Rows)
                return JxrError.InvalidArgument;
            if (trimFlexbits < 0 || trimFlexbits > 15)
                return JxrError.InvalidArgument;

            int count = entropyPackets.Length;
            int[] lengths = new int[count];
            long totalPacketBytes = 0;
            for (int index = 0; index < count; index++)
            {
                if (entropyPackets[index] == null || entropyBitCounts[index] < 0 ||
                    entropyBitCounts[index] > (long)entropyPackets[index].Length * 8)
                    return JxrError.InvalidArgument;
                long packetBits = 32L + (trimFlexbits == 0 ? 0 : 4) +
                    entropyBitCounts[index];
                long packetLength = (packetBits + 7) / 8;
                if (packetLength > Int32.MaxValue) return JxrError.UnsupportedFeature;
                lengths[index] = (int)packetLength;
                if (lengths[index] > 4) totalPacketBytes += lengths[index];
            }

            JxrBitWriter writer = new JxrBitWriter();
            error = rgb ? JxrHeaderWriter.WriteRgbSpatial(writer, width, height,
                dcQuantizerIndex, lowpassQuantizerIndex, highpassQuantizerIndex,
                subbands, scaledArithmetic, trimFlexbits, overlap,
                chromaSubsampling, tileLayout) :
                JxrHeaderWriter.WriteGraySpatial(writer, width, height,
                    dcQuantizerIndex, lowpassQuantizerIndex,
                    highpassQuantizerIndex, subbands, scaledArithmetic,
                    trimFlexbits, overlap, tileLayout);
            if (error != JxrError.None) return error;

            // The index-table offsets point from the byte after the table to
            // each packet. Native encoding omits packets of four bytes or less
            // and marks their repeated offsets with 0xff.
            writer.Write(1, 16);
            uint packetOffset = 0;
            for (int index = 0; index < count; index++)
            {
                if (lengths[index] <= 4)
                    JxrVariableLengthWordWriter.WriteEscape(writer, 0xff);
                else
                {
                    error = JxrVariableLengthWordWriter.Write(writer, packetOffset);
                    if (error != JxrError.None) return error;
                    if ((ulong)packetOffset + (uint)lengths[index] > UInt32.MaxValue)
                        return JxrError.UnsupportedFeature;
                    packetOffset += (uint)lengths[index];
                }
            }
            JxrVariableLengthWordWriter.WriteEscape(writer, 0xff);

            if ((long)writer.BitCount / 8 + totalPacketBytes > Int32.MaxValue)
                return JxrError.UnsupportedFeature;
            for (int index = 0; index < count; index++)
            {
                if (lengths[index] <= 4) continue;
                error = JxrPacketWriter.WriteHeader(writer, index & 31, 0);
                if (error != JxrError.None) return error;
                if (trimFlexbits != 0) writer.Write((uint)trimFlexbits, 4);
                JxrBitReader entropyReader = new JxrBitReader(entropyPackets[index]);
                int remainingBits = entropyBitCounts[index];
                while (remainingBits > 0)
                {
                    uint bit;
                    error = entropyReader.ReadBits(1, out bit);
                    if (error != JxrError.None) return error;
                    error = writer.Write(bit, 1);
                    if (error != JxrError.None) return error;
                    remainingBits--;
                }
                writer.AlignByte();
            }
            codestream = writer.ToArray();
            return JxrError.None;
        }

        public static JxrError WriteRgbSpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, out byte[] codestream)
        {
            return WriteRgbSpatial(entropyPacket, width, height,
                dcQuantizerIndex, lowpassQuantizerIndex, highpassQuantizerIndex,
                subbands, scaledArithmetic, trimFlexbits, entropyBitCount, 0,
                out codestream);
        }

        public static JxrError WriteRgbSpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, int overlap,
            out byte[] codestream)
        {
            return WriteRgbSpatial(entropyPacket, width, height,
                dcQuantizerIndex, lowpassQuantizerIndex, highpassQuantizerIndex,
                subbands, scaledArithmetic, trimFlexbits, entropyBitCount,
                overlap, JxrChromaSubsampling.Yuv444, out codestream);
        }

        public static JxrError WriteRgbSpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, int overlap,
            JxrChromaSubsampling chromaSubsampling, out byte[] codestream)
        {
            return WriteSpatial(entropyPacket, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, entropyBitCount, overlap, true,
                chromaSubsampling,
                out codestream);
        }

        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte quantizerIndex, out byte[] codestream)
        {
            return WriteGraySpatial(entropyPacket, width, height, quantizerIndex,
                quantizerIndex, quantizerIndex, JxrGraySubbandMode.All, false, 0,
                out codestream);
        }

        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, out byte[] codestream)
        {
            return WriteGraySpatial(entropyPacket, width, height,
                dcQuantizerIndex, lowpassQuantizerIndex, highpassQuantizerIndex,
                subbands, scaledArithmetic, trimFlexbits,
                entropyPacket == null ? 0 : entropyPacket.Length * 8,
                out codestream);
        }

        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, out byte[] codestream)
        {
            return WriteGraySpatial(entropyPacket, width, height,
                dcQuantizerIndex, lowpassQuantizerIndex, highpassQuantizerIndex,
                subbands, scaledArithmetic, trimFlexbits, entropyBitCount, 0,
                out codestream);
        }

        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, int overlap,
            out byte[] codestream)
        {
            return WriteSpatial(entropyPacket, width, height, dcQuantizerIndex,
                lowpassQuantizerIndex, highpassQuantizerIndex, subbands,
                scaledArithmetic, trimFlexbits, entropyBitCount, overlap, false,
                JxrChromaSubsampling.Yuv444,
                out codestream);
        }

        private static JxrError WriteSpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, int overlap, bool rgb,
            JxrChromaSubsampling chromaSubsampling,
            out byte[] codestream)
        {
            codestream = null;
            if (entropyPacket == null || entropyBitCount < 0 ||
                entropyBitCount > (long)entropyPacket.Length * 8)
                return JxrError.InvalidArgument;
            JxrBitWriter writer = new JxrBitWriter();
            JxrError error = rgb ? JxrHeaderWriter.WriteRgbSpatial(writer,
                width, height, dcQuantizerIndex, lowpassQuantizerIndex,
                highpassQuantizerIndex, subbands, scaledArithmetic, trimFlexbits,
                overlap, chromaSubsampling) :
                JxrHeaderWriter.WriteGraySpatial(writer, width, height,
                    dcQuantizerIndex, lowpassQuantizerIndex,
                    highpassQuantizerIndex, subbands, scaledArithmetic,
                    trimFlexbits, overlap);
            if (error != JxrError.None) return error;

            // No index table in this spatial profile. The length word covers
            // the marker record, not the entropy packet; its actual length is
            // carried by the container's ImageByteCount field.
            JxrBitWriter marker = new JxrBitWriter();
            marker.Write(0x6f, 8);
            marker.Write(0xff, 8);
            marker.Write(1, 16);
            byte[] markerBytes = marker.ToArray();
            error = JxrVariableLengthWordWriter.Write(writer,
                (uint)markerBytes.Length);
            if (error != JxrError.None) return error;
            for (int index = 0; index < markerBytes.Length; index++)
                writer.Write(markerBytes[index], 8);
            error = JxrPacketWriter.WriteHeader(writer, 0, 0);
            if (error != JxrError.None) return error;
            if (trimFlexbits != 0) writer.Write((uint)trimFlexbits, 4);
            JxrBitReader entropyReader = new JxrBitReader(entropyPacket);
            int remainingBits = entropyBitCount;
            while (remainingBits > 0)
            {
                uint bit;
                error = entropyReader.ReadBits(1, out bit);
                if (error != JxrError.None) return error;
                error = writer.Write(bit, 1);
                if (error != JxrError.None) return error;
                remainingBits--;
            }
            writer.AlignByte();
            codestream = writer.ToArray();
            return JxrError.None;
        }
    }

    public static class JxrVariableLengthWordWriter
    {
        // The native 16/40-bit forms needed for 32-bit managed buffers.
        public static JxrError Write(JxrBitWriter writer, uint value)
        {
            if (writer == null || (writer.BitCount & 7) != 0)
                return JxrError.InvalidArgument;
            if (value < 0xfb00)
                writer.Write(value, 16);
            else
            {
                writer.Write(0xfb, 8);
                writer.Write(value >> 16, 16);
                writer.Write(value & 0xffff, 16);
            }
            return JxrError.None;
        }

        internal static void WriteEscape(JxrBitWriter writer, int marker)
        {
            writer.Write((uint)marker, 8);
        }
    }
}
