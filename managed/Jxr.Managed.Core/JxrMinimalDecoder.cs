using System;

namespace Jxr.Managed.Core
{
    // Managed pixel decode for Y_ONLY, QP=1, spatial, one-tile, OL_NONE.
    // Macroblocks share one entropy context and two prediction rows.
    // Unsupported JPEG XR variants
    // are rejected instead of silently following the minimal path.
    public static class JxrMinimalDecoder
    {
        private static readonly int[] LocalSampleOrder = {
            0, 1, 5, 4, 2, 3, 7, 6,
            10, 11, 15, 14, 8, 9, 13, 12
        };

        public static JxrError DecodeGrayBmp(byte[] source, out byte[] bitmap)
        {
            bitmap = null;
            JxrImage image;
            JxrError error = JxrCodec.Decode(source,
                new JxrDecoderOptions(), out image);
            if (error != JxrError.None) return error;
            return JxrBmpAdapter.WriteGray8(image, out bitmap);
        }

        internal static JxrError DecodeGrayPixels(byte[] source, out byte[] pixels,
            out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            JxrHeaders headers;
            JxrError error = JxrHeaders.Read(source, out headers);
            if (error != JxrError.None) return error;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            if (main.Width < 1 || main.Height < 1 ||
                main.Width > Int32.MaxValue - 15 ||
                main.Height > Int32.MaxValue - 15 ||
                (long)main.Width * main.Height > Int32.MaxValue ||
                main.Overlap != 0 ||
                main.BitstreamFormat != 0 || main.Orientation != 0 ||
                main.CodedBitDepth != 1 || main.SourceColorFormat != 0 ||
                main.HasAlpha || main.BlackWhite ||
                main.VerticalSliceCountMinusOne != 0 ||
                main.HorizontalSliceCountMinusOne != 0 ||
                main.HasIndexTable ||
                main.ExtraTop != 0 || main.ExtraLeft != 0 ||
                main.ExtraBottom != ((16 - ((int)main.Height & 15)) & 15) ||
                main.ExtraRight != ((16 - ((int)main.Width & 15)) & 15) ||
                plane.ColorFormat != 0 || plane.ChannelCount != 1 ||
                plane.Subband < 0 || plane.Subband > 3 ||
                main.SourceBitDepth != 1 ||
                q.Mode != (plane.Subband == (int)JxrGraySubbandMode.DcOnly ? 0x200 : 0x600) ||
                !q.HasDc || (plane.Subband != 3 && !q.HasLowpass) ||
                (plane.Subband < 2 && !q.HasHighpass))
                return JxrError.UnsupportedFeature;

            int packetOffset;
            error = LocateSingleSpatialPacket(source, headers, out packetOffset);
            if (error != JxrError.None) return error;
            if (packetOffset > Int32.MaxValue / 8)
                return JxrError.UnsupportedFeature;
            JxrBitReader reader = new JxrBitReader(source);
            int remaining = packetOffset * 8;
            while (remaining > 0)
            {
                int count = remaining > 32 ? 32 : remaining;
                error = reader.ConsumeBits(count);
                if (error != JxrError.None) return error;
                remaining -= count;
            }
            JxrPacketHeader packet;
            error = JxrPacketReader.ReadHeader(reader, out packet);
            if (error != JxrError.None) return error;
            if (!packet.IsValid || packet.TileId != 0 || packet.PacketType != 0)
                return JxrError.InvalidBitstream;
            int trimFlexbits = 0;
            if (main.TrimFlexbits)
            {
                uint trim;
                error = reader.ReadBits(4, out trim);
                if (error != JxrError.None) return error;
                trimFlexbits = (int)trim;
            }

            JxrSessionConfiguration sessionConfig;
            error = JxrSessionPlanner.FromHeaders(headers, out sessionConfig);
            if (error != JxrError.None) return error;
            using (JxrDecoderSession session =
                JxrDecoderSession.Create(sessionConfig, 0, 0, 0))
            {
                bool dcOnly = plane.Subband == (int)JxrGraySubbandMode.DcOnly;
                bool scaledArithmetic = plane.ScaledArithmetic;
                bool hasHighpass = plane.Subband < (int)JxrGraySubbandMode.NoHighpass;
                bool skipFlexbits = plane.Subband == (int)JxrGraySubbandMode.NoFlexbits;
                JxrCodecConfiguration format = new JxrCodecConfiguration(
                    JxrCodecColorFormat.YOnly, 1, true, dcOnly, hasHighpass,
                    !skipFlexbits, skipFlexbits, true, true, false, false,
                    0, 0, 1, 1,
                    new int[][] { new int[] { JxrQuantization.Remap(
                        q.GetHighpassIndex(0), plane.ScaledArithmetic, false).Parameter } });
                JxrCodecState state = new JxrCodecState(format,
                    reader, reader, reader, reader);
                state.Entropy.TrimFlexBits = trimFlexbits;
                int imageWidth = (int)main.Width, imageHeight = (int)main.Height;
                int columns = (imageWidth + 15) / 16;
                int rowsCount = (imageHeight + 15) / 16;
                JxrCoefficientPredictionRows rows =
                    new JxrCoefficientPredictionRows(columns, 1);
                int[] topCbp = new int[columns];
                byte[] gray = new byte[imageWidth * imageHeight];
                JxrQuantizer dcQuantizer = JxrQuantization.Remap(
                    q.GetDcIndex(0), plane.ScaledArithmetic, false);
                JxrQuantizer lpQuantizer = JxrQuantization.Remap(
                    plane.Subband == 3 ? q.GetDcIndex(0) : q.GetLowpassIndex(0),
                    plane.ScaledArithmetic, false);
                JxrQuantizer hpQuantizer = JxrQuantization.Remap(
                    plane.Subband < 2 ? q.GetHighpassIndex(0) : q.GetDcIndex(0),
                    plane.ScaledArithmetic, false);
                JxrQuantizerSet quantizers = new JxrQuantizerSet(
                    new JxrQuantizer[] { dcQuantizer.WithDcOffset() },
                    new JxrQuantizer[][] { new JxrQuantizer[] { lpQuantizer } },
                    new JxrQuantizer[][] { new JxrQuantizer[] { hpQuantizer } });
                for (int mbY = 0; mbY < rowsCount; mbY++)
                {
                    if (mbY != 0) rows.AdvanceRow();
                    int leftCbp = 0;
                    for (int mbX = 0; mbX < columns; mbX++)
                    {
                        state.SetMacroblockPosition(mbX, mbY, columns);
                        state.SetNeighborCbp(0, topCbp[mbX], leftCbp);
                        int[] coefficients;
                        error = state.CoefficientPlanes.GetPlane(0, out coefficients);
                        if (error != JxrError.None) return error;
                        Array.Clear(coefficients, 0, coefficients.Length);
                        error = JxrDcCodec.Decode(state);
                        if (error != JxrError.None) return error;
                        if (!dcOnly)
                        {
                            error = JxrLpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                        }
                        error = JxrCoefficientPrediction.DecodeDcLp(state.Macroblock,
                            rows, JxrCodecColorFormat.YOnly, mbX, mbX == 0, mbY == 0);
                        if (error != JxrError.None) return error;
                        error = JxrCoefficientPrediction.StoreCurrent(state.Macroblock,
                            rows, JxrCodecColorFormat.YOnly, mbX);
                        if (error != JxrError.None) return error;
                        error = JxrQuantization.DequantizeMacroblock(state.CoefficientPlanes,
                            state.Macroblock, quantizers, JxrCodecColorFormat.YOnly,
                            1, dcOnly);
                        if (error != JxrError.None) return error;
                        if (hasHighpass)
                        {
                            error = JxrHpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                            int cbp;
                            error = state.MacroblockCbp.GetCbp(0, out cbp);
                            if (error != JxrError.None) return error;
                            leftCbp = topCbp[mbX] = cbp;
                        }
                        error = JxrCoefficientPrediction.DecodeAc(state.Macroblock,
                            state.CoefficientPlanes, JxrCodecColorFormat.YOnly);
                        if (error != JxrError.None) return error;
                        int[] samples = new int[256];
                        Array.Copy(coefficients, samples, 256);
                        InverseMacroblock(samples);
                        for (int y = 0; y < 16; y++)
                            for (int x = 0; x < 16; x++)
                            {
                                int pixelX = mbX * 16 + x, pixelY = mbY * 16 + y;
                                if (pixelX >= imageWidth || pixelY >= imageHeight) continue;
                                int block = (x >> 2) * 64 + (y >> 2) * 16;
                                int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                                int sample = samples[block + local];
                                if (scaledArithmetic)
                                    sample = unchecked(sample + 1027) >> 3;
                                else
                                    sample = unchecked(sample + 128);
                                gray[pixelY * imageWidth + pixelX] =
                                    JxrImagePipeline.ClipByte(sample);
                            }
                    }
                }
                pixels = gray;
                width = imageWidth; height = imageHeight;
                return JxrError.None;
            }
        }

        // Full-resolution three-plane path used by RGB24 images encoded as
        // YUV444.  All three component planes share one spatial entropy
        // stream, prediction state and macroblock position.
        internal static JxrError DecodeRgbPixels(byte[] source, bool rgbOrder,
            out byte[] pixels,
            out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            JxrHeaders headers;
            JxrError error = JxrHeaders.Read(source, out headers);
            if (error != JxrError.None) return error;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            if (main.Width < 1 || main.Height < 1 ||
                main.Width > Int32.MaxValue - 15 || main.Height > Int32.MaxValue - 15 ||
                (long)main.Width * main.Height > Int32.MaxValue ||
                main.Overlap != 0 || main.BitstreamFormat != 0 || main.Orientation != 0 ||
                main.CodedBitDepth != 1 || main.HasAlpha || main.BlackWhite ||
                main.VerticalSliceCountMinusOne != 0 || main.HorizontalSliceCountMinusOne != 0 ||
                main.HasIndexTable || main.ExtraTop != 0 || main.ExtraLeft != 0 ||
                main.ExtraBottom != ((16 - ((int)main.Height & 15)) & 15) ||
                main.ExtraRight != ((16 - ((int)main.Width & 15)) & 15) ||
                plane.ColorFormat != (int)JxrCodecColorFormat.Yuv444 ||
                plane.ChannelCount != 3 || plane.Subband < 0 || plane.Subband > 3 ||
                main.SourceBitDepth != 1 ||
                !q.HasDc || (plane.Subband != 3 && !q.HasLowpass) ||
                (plane.Subband < 2 && !q.HasHighpass))
                return JxrError.UnsupportedFeature;

            int packetOffset;
            error = LocateSingleSpatialPacket(source, headers, out packetOffset);
            if (error != JxrError.None) return error;
            if (packetOffset > Int32.MaxValue / 8) return JxrError.UnsupportedFeature;
            JxrBitReader reader = new JxrBitReader(source);
            int remaining = packetOffset * 8;
            while (remaining > 0)
            {
                int count = remaining > 32 ? 32 : remaining;
                error = reader.ConsumeBits(count);
                if (error != JxrError.None) return error;
                remaining -= count;
            }
            JxrPacketHeader packet;
            error = JxrPacketReader.ReadHeader(reader, out packet);
            if (error != JxrError.None) return error;
            if (!packet.IsValid || packet.TileId != 0 || packet.PacketType != 0)
                return JxrError.InvalidBitstream;
            int trimFlexbits = 0;
            if (main.TrimFlexbits)
            {
                uint trim;
                error = reader.ReadBits(4, out trim);
                if (error != JxrError.None) return error;
                trimFlexbits = (int)trim;
            }

            JxrSessionConfiguration sessionConfig;
            error = JxrSessionPlanner.FromHeaders(headers, out sessionConfig);
            if (error != JxrError.None) return error;
            using (JxrDecoderSession session = JxrDecoderSession.Create(
                sessionConfig, 0, 0, 0))
            {
                bool dcOnly = plane.Subband == (int)JxrGraySubbandMode.DcOnly;
                bool scaledArithmetic = plane.ScaledArithmetic;
                bool hasHighpass = plane.Subband < (int)JxrGraySubbandMode.NoHighpass;
                bool skipFlexbits = plane.Subband == (int)JxrGraySubbandMode.NoFlexbits;
                int[][] hpParameters = new int[3][];
                JxrQuantizer[] dc = new JxrQuantizer[3];
                JxrQuantizer[][] lp = new JxrQuantizer[3][];
                JxrQuantizer[][] hp = new JxrQuantizer[3][];
                for (int channel = 0; channel < 3; channel++)
                {
                    byte dcIndex = q.GetDcIndex(channel);
                    byte lpIndex = plane.Subband == 3 ? dcIndex : q.GetLowpassIndex(channel);
                    byte hpIndex = plane.Subband < 2 ? q.GetHighpassIndex(channel) : dcIndex;
                    dc[channel] = JxrQuantization.Remap(dcIndex, scaledArithmetic, channel != 0).WithDcOffset();
                    lp[channel] = new JxrQuantizer[] {
                        JxrQuantization.Remap(lpIndex, scaledArithmetic, channel != 0) };
                    hp[channel] = new JxrQuantizer[] {
                        JxrQuantization.Remap(hpIndex, scaledArithmetic, false) };
                    hpParameters[channel] = new int[] { hp[channel][0].Parameter };
                }
                JxrQuantizerSet quantizers = new JxrQuantizerSet(dc, lp, hp);
                JxrCodecConfiguration format = new JxrCodecConfiguration(
                    JxrCodecColorFormat.Yuv444, 3, true, dcOnly, hasHighpass,
                    !skipFlexbits, skipFlexbits, true, true, false, false,
                    0, 0, 1, 1, hpParameters);
                JxrCodecState state = new JxrCodecState(format,
                    reader, reader, reader, reader);
                state.Entropy.TrimFlexBits = trimFlexbits;
                int imageWidth = (int)main.Width, imageHeight = (int)main.Height;
                int columns = (imageWidth + 15) / 16;
                int rowsCount = (imageHeight + 15) / 16;
                JxrCoefficientPredictionRows rows =
                    new JxrCoefficientPredictionRows(columns, 3);
                int[][] topCbp = { new int[columns], new int[columns], new int[columns] };
                int[][] planes = new int[3][];
                for (int channel = 0; channel < 3; channel++)
                    state.CoefficientPlanes.GetPlane(channel, out planes[channel]);
                int count = imageWidth * imageHeight;
                int[] yPlane = new int[count], uPlane = new int[count], vPlane = new int[count];
                int[][] outputPlanes = { yPlane, uPlane, vPlane };
                for (int mbY = 0; mbY < rowsCount; mbY++)
                {
                    if (mbY != 0) rows.AdvanceRow();
                    int[] leftCbp = new int[3];
                    for (int mbX = 0; mbX < columns; mbX++)
                    {
                        state.SetMacroblockPosition(mbX, mbY, columns);
                        for (int channel = 0; channel < 3; channel++)
                        {
                            state.SetNeighborCbp(channel, topCbp[channel][mbX], leftCbp[channel]);
                            Array.Clear(planes[channel], 0, 256);
                        }
                        error = JxrDcCodec.Decode(state);
                        if (error != JxrError.None) return error;
                        if (!dcOnly)
                        {
                            error = JxrLpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                        }
                        error = JxrCoefficientPrediction.DecodeDcLp(state.Macroblock,
                            rows, JxrCodecColorFormat.Yuv444, mbX, mbX == 0, mbY == 0);
                        if (error != JxrError.None) return error;
                        error = JxrCoefficientPrediction.StoreCurrent(state.Macroblock,
                            rows, JxrCodecColorFormat.Yuv444, mbX);
                        if (error != JxrError.None) return error;
                        error = JxrQuantization.DequantizeMacroblock(state.CoefficientPlanes,
                            state.Macroblock, quantizers, JxrCodecColorFormat.Yuv444,
                            3, dcOnly);
                        if (error != JxrError.None) return error;
                        if (hasHighpass)
                        {
                            error = JxrHpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                            for (int channel = 0; channel < 3; channel++)
                            {
                                error = state.MacroblockCbp.GetCbp(channel, out leftCbp[channel]);
                                if (error != JxrError.None) return error;
                                topCbp[channel][mbX] = leftCbp[channel];
                            }
                        }
                        error = JxrCoefficientPrediction.DecodeAc(state.Macroblock,
                            state.CoefficientPlanes, JxrCodecColorFormat.Yuv444);
                        if (error != JxrError.None) return error;
                        for (int channel = 0; channel < 3; channel++)
                        {
                            int[] samples = new int[256];
                            Array.Copy(planes[channel], samples, 256);
                            InverseMacroblock(samples, channel != 0 && scaledArithmetic);
                            for (int y = 0; y < 16; y++)
                                for (int x = 0; x < 16; x++)
                                {
                                    int pixelX = mbX * 16 + x, pixelY = mbY * 16 + y;
                                    if (pixelX >= imageWidth || pixelY >= imageHeight) continue;
                                    int block = (x >> 2) * 64 + (y >> 2) * 16;
                                    int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                                    outputPlanes[channel][pixelY * imageWidth + pixelX] =
                                        samples[block + local];
                                }
                        }
                    }
                }
                pixels = new byte[imageWidth * imageHeight * 3];
                error = JxrImagePipeline.DecodeRgb8(yPlane, uPlane, vPlane,
                    imageWidth, imageHeight, rgbOrder, scaledArithmetic ? 3 : 0,
                    pixels, imageWidth * 3);
                if (error != JxrError.None) { pixels = null; return error; }
                width = imageWidth; height = imageHeight;
                return JxrError.None;
            }
        }

        private static JxrError LocateSingleSpatialPacket(byte[] source,
            JxrHeaders headers, out int offset)
        {
            offset = 0;
            int position = headers.CodestreamOffset + headers.ByteCount;
            if (position < 0 || position > source.Length - 2)
                return JxrError.UnexpectedEndOfStream;
            // JxrIndexTableReader always reads a variable-length word even in
            // streaming mode. This profile uses its two-byte form.
            int first = source[position];
            if (first >= 0xfb) return JxrError.UnsupportedFeature;
            int headerSize = (first << 8) | source[position + 1];
            long packetPosition = (long)position + 2 + headerSize;
            if (packetPosition + 4 > source.Length)
                return JxrError.UnexpectedEndOfStream;
            offset = (int)packetPosition;
            return JxrError.None;
        }

        private static void InverseMacroblock(int[] values)
        {
            InverseMacroblock(values, false);
        }

        private static void InverseMacroblock(int[] values, bool normalizeChroma)
        {
            // Stage 2: the 16 DC/LP values are embedded at native offsets.
            JxrInverseTransformMath.ApplyOdd(
                ref values[32], ref values[48], ref values[96], ref values[112]);
            JxrInverseTransformMath.ApplyOdd(
                ref values[128], ref values[192], ref values[144], ref values[208]);
            JxrInverseTransformMath.ApplyOddOdd(
                ref values[160], ref values[224], ref values[176], ref values[240]);
            JxrTransformMath.ApplyDct2x2Up(values, 0, 64, 16, 80);
            JxrTransformMath.ApplySecondStageFourButterfly(values);
            // Native JxrInverseTransformPlaneStage2Apply normalizes scaled
            // full-resolution chroma here, after stage 2 but before stage 1.
            // Doubling quantizers instead changes intermediate rounding.
            if (normalizeChroma)
                for (int offset = 0; offset < 256; offset += 16)
                    values[offset] = unchecked(values[offset] + values[offset]);

            // Stage 1 is independent for each 4x4 coefficient block with no overlap.
            for (int block = 0; block < 16; block++)
            {
                int offset = block * 16;
                int[] work = new int[16];
                Array.Copy(values, offset, work, 0, 16);
                JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
                JxrInverseTransformMath.ApplyOdd(
                    ref work[5], ref work[4], ref work[7], ref work[6]);
                JxrInverseTransformMath.ApplyOdd(
                    ref work[10], ref work[8], ref work[11], ref work[9]);
                JxrInverseTransformMath.ApplyOddOdd(
                    ref work[15], ref work[14], ref work[13], ref work[12]);
                JxrTransformMath.ApplyFirstStageFourButterfly(work);
                Array.Copy(work, 0, values, offset, 16);
            }
        }

    }
}
