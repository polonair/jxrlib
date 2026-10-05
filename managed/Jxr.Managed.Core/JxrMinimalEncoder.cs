using System;

namespace Jxr.Managed.Core
{
    // Compatibility BMP entry point and pixel-based lossless Y_ONLY core.
    // Macroblocks share one entropy context and two prediction rows.
    public static class JxrMinimalEncoder
    {
        private static readonly int[] LocalSampleOrder =
            { 0,1,5,4,2,3,7,6,10,11,15,14,8,9,13,12 };

        public static JxrError EncodeGrayBmp(byte[] bitmap, out byte[] jxr)
        {
            jxr = null;
            JxrImage image;
            JxrError error = JxrBmpAdapter.ReadGray8(bitmap, out image);
            if (error != JxrError.None) return error;
            return JxrCodec.Encode(image, new JxrEncoderOptions(), out jxr);
        }

        internal static JxrError EncodeGrayPixels(byte[] pixels, int stride,
            out byte[] jxr)
        {
            return EncodeGrayPixels(pixels, stride, 16, 16, out jxr);
        }

        internal static JxrError EncodeGrayPixels(byte[] pixels, int stride,
            int width, int height, out byte[] jxr)
        {
            return EncodeGrayPixels(pixels, stride, width, height,
                new JxrEncoderOptions(), out jxr);
        }

        internal static JxrError EncodeGrayPixels(byte[] pixels, int stride,
            int width, int height, JxrEncoderOptions options, out byte[] jxr)
        {
            return EncodeGrayPixels(pixels, stride, width, height, options,
                (JxrGrayEncodingTrace)null, out jxr);
        }

        internal static JxrError EncodeGrayPixels(byte[] pixels, int stride,
            int width, int height, JxrEncoderOptions options,
            JxrGrayEncodingTrace trace, out byte[] jxr)
        {
            jxr = null;
            if (trace != null) trace.Clear();
            JxrSessionConfiguration sessionConfig = new JxrSessionConfiguration(
                width, height, 0, 1, 4, false);
            using (JxrEncoderSession session = JxrEncoderSession.Create(
                sessionConfig, 0, 0))
                return EncodeWithSession(pixels, stride, width, height, options,
                    session, trace, null, out jxr);
        }

        internal static JxrError EncodeGrayProfilePixels(byte[] pixels, int stride,
            int width, int height, JxrEncoderOptions options,
            JxrProfileEncodingSettings profileSettings, out byte[] jxr)
        {
            jxr = null;
            if (profileSettings == null || !profileSettings.GrayPlane)
                return JxrError.InvalidArgument;
            JxrSessionConfiguration sessionConfig = new JxrSessionConfiguration(
                width, height, 0, 1, 4, false);
            using (JxrEncoderSession session = JxrEncoderSession.Create(
                sessionConfig, 0, 0))
                return EncodeWithSession(pixels, stride, width, height, options,
                    session, null, profileSettings, out jxr);
        }

        private static JxrError EncodeWithSession(byte[] pixels,
            int stride, int width, int height, JxrEncoderOptions options,
            JxrEncoderSession session, JxrGrayEncodingTrace trace,
            JxrProfileEncodingSettings profileSettings, out byte[] jxr)
        {
            jxr = null;
            JxrError error;
            int columns = (width + 15) / 16, rowsCount = (height + 15) / 16;
            byte dcIndex = profileSettings == null ?
                QpIndex(options.DcQuantizerIndex, options.QualityIndex) :
                profileSettings.DcIndices[0];
            byte lpIndex = profileSettings == null ?
                QpIndex(options.LowpassQuantizerIndex, options.QualityIndex) :
                profileSettings.LowpassIndices[0];
            byte hpIndex = profileSettings == null ?
                QpIndex(options.HighpassQuantizerIndex, options.QualityIndex) :
                profileSettings.HighpassIndices[0];
            bool scaledArithmetic = profileSettings == null ?
                options.Subbands != JxrGraySubbandMode.All ||
                    dcIndex > 1 || lpIndex > 1 || hpIndex > 1 :
                profileSettings.ScaledArithmetic;
            int[][] overlapped = null;
            if (options.Overlap != 0)
            {
                int[] samples;
                error = JxrImagePipeline.EncodeGray8(pixels, stride, width,
                    height, scaledArithmetic ? 3 : 0, out samples);
                if (error != JxrError.None) return error;
                overlapped = JxrOverlapForward.Transform(samples, width, height,
                    options.Overlap, false);
            }
            JxrQuantizer dcQuantizer = JxrQuantization.Remap(dcIndex, scaledArithmetic, false);
            JxrQuantizer lpQuantizer = JxrQuantization.Remap(lpIndex, scaledArithmetic, false);
            JxrQuantizer hpQuantizer = JxrQuantization.Remap(hpIndex, scaledArithmetic, false);
            JxrQuantizerSet quantizers = new JxrQuantizerSet(
                new JxrQuantizer[] { dcQuantizer.WithDcOffset() },
                new JxrQuantizer[][] { new JxrQuantizer[] { lpQuantizer } },
                new JxrQuantizer[][] { new JxrQuantizer[] { hpQuantizer } });
            JxrTileGeometry tiles;
            error = JxrTileGeometry.Create(width, height, options.TileLayout,
                out tiles);
            if (error != JxrError.None) return error;
            byte[][] entropyPackets = new byte[tiles.Columns * tiles.Rows][];
            int[] entropyBitCounts = new int[entropyPackets.Length];
            bool frequency = options.Layout == JxrBitstreamLayout.Frequency;
            byte[][][] frequencyPackets = frequency ?
                new byte[4][][] : null;
            int[][] frequencyBitCounts = frequency ? new int[4][] : null;
            if (frequency)
                for (int band = 0; band < 4; band++)
                {
                    frequencyPackets[band] = new byte[entropyPackets.Length][];
                    frequencyBitCounts[band] = new int[entropyPackets.Length];
                }
            JxrBitReader unused = new JxrBitReader(new byte[0]);
            for (int tileY = 0; tileY < tiles.Rows; tileY++)
                for (int tileX = 0; tileX < tiles.Columns; tileX++)
                {
                    int startMbX = tiles.GetX(tileX);
                    int startMbY = tiles.GetY(tileY);
                    int tileWidth = tiles.GetX(tileX + 1) - startMbX;
                    int tileHeight = tiles.GetY(tileY + 1) - startMbY;
                    int[] tileCoefficients = new int[256];
                    JxrCoefficientPlaneState tilePlanes =
                        new JxrCoefficientPlaneState(new int[][] { tileCoefficients },
                            JxrCoefficientColorFormat.Other, 1);
                    JxrMacroblockState tileMacroblock = new JxrMacroblockState(1);
                    JxrCoefficientPredictionRows rows =
                        new JxrCoefficientPredictionRows(tileWidth, 1);
                    JxrCodecConfiguration format = new JxrCodecConfiguration(
                        JxrCodecColorFormat.YOnly, 1, !frequency,
                        options.Subbands == JxrGraySubbandMode.DcOnly,
                        (int)options.Subbands < (int)JxrGraySubbandMode.NoHighpass,
                        options.Subbands != JxrGraySubbandMode.NoFlexbits,
                        options.Subbands == JxrGraySubbandMode.NoFlexbits,
                        true, true, false, false, 0, 0, 1, 1,
                        new int[][] { new int[] { hpQuantizer.Parameter } });
                    JxrCodecState state = new JxrCodecState(format, unused,
                        unused, unused, unused);
                    int[] dc = new int[16];
                    int[] topCbp = new int[tileWidth];
                    JxrBitWriter writer = new JxrBitWriter();
                    JxrBitWriter lpWriter = frequency ? new JxrBitWriter() : writer;
                    JxrBitWriter hpWriter = frequency ? new JxrBitWriter() : writer;
                    JxrBitWriter flexWriter = frequency ? new JxrBitWriter() : writer;
                    for (int localY = 0; localY < tileHeight; localY++)
                    {
                        if (localY != 0) rows.AdvanceRow();
                        int mbY = startMbY + localY;
                        int leftCbp = 0;
                        for (int localX = 0; localX < tileWidth; localX++)
                        {
                            int mbX = startMbX + localX;
                            state.SetMacroblockPosition(localX, localY, tileWidth);
                            state.SetNeighborCbp(0, topCbp[localX], leftCbp);
                            if (overlapped != null)
                                Array.Copy(overlapped[mbY * columns + mbX],
                                    tileCoefficients, 256);
                            else
                            {
                                for (int y = 0; y < 16; y++)
                                    for (int x = 0; x < 16; x++)
                                    {
                                        int pixelX = Math.Min(width - 1, mbX * 16 + x);
                                        int pixelY = Math.Min(height - 1, mbY * 16 + y);
                                        int block = (x >> 2) * 64 + (y >> 2) * 16;
                                        int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                                        int centered = pixels[pixelY * stride + pixelX] - 128;
                                        tileCoefficients[block + local] = scaledArithmetic
                                            ? unchecked(centered << 3) : centered;
                                    }
                                ForwardMacroblock(tileCoefficients);
                            }
                            int[] transformed = trace == null ? null :
                                (int[])tileCoefficients.Clone();
                            error = JxrQuantization.QuantizeMacroblock(tilePlanes,
                                tileMacroblock, quantizers,
                                JxrCodecColorFormat.YOnly, 1,
                                options.Subbands == JxrGraySubbandMode.DcOnly,
                                (int)options.Subbands >=
                                    (int)JxrGraySubbandMode.NoHighpass, false);
                            if (error != JxrError.None) return error;
                            int[] quantized = trace == null ? null :
                                (int[])tileCoefficients.Clone();
                            error = JxrCoefficientPrediction.Encode(tileMacroblock,
                                tilePlanes, rows, JxrCodecColorFormat.YOnly,
                                localX, localX == 0, localY == 0);
                            if (error != JxrError.None) return error;
                            int[] predicted = trace == null ? null :
                                (int[])tileCoefficients.Clone();
                            for (int index = 0; index < 16; index++)
                            {
                                error = tileMacroblock.GetDcCoefficient(0,
                                    index, out dc[index]);
                                if (error != JxrError.None) return error;
                            }
                            int dcEnd, lpEnd, hpEnd;
                            int macroblockBitStart = writer.BitCount;
                            error = JxrMinimalEntropyEncoder.EncodeMacroblock(state,
                                tileCoefficients, dc, tileMacroblock.Orientation,
                                (int)options.Subbands, options.TrimFlexbits, writer,
                                lpWriter, hpWriter, flexWriter,
                                out dcEnd, out lpEnd, out hpEnd);
                            if (error != JxrError.None) return error;
                            if (trace != null)
                                trace.Add(new JxrGrayMacroblockTrace(mbX, mbY,
                                    transformed, quantized, predicted,
                                    macroblockBitStart, dcEnd,
                                    dcEnd, lpEnd, lpEnd, hpEnd));
                            int previousTop;
                            state.GetNeighborCbp(0, out previousTop, out leftCbp);
                            topCbp[localX] = leftCbp;
                        }
                    }
                    int packetIndex = tileY * tiles.Columns + tileX;
                    if (frequency)
                    {
                        JxrBitWriter[] writers = { writer, lpWriter, hpWriter,
                            flexWriter };
                        for (int band = 0; band < 4; band++)
                        {
                            frequencyBitCounts[band][packetIndex] =
                                writers[band].BitCount;
                            writers[band].AlignByte();
                            frequencyPackets[band][packetIndex] =
                                writers[band].ToArray();
                        }
                    }
                    else
                    {
                        entropyBitCounts[packetIndex] = writer.BitCount;
                        writer.AlignByte();
                        entropyPackets[packetIndex] = writer.ToArray();
                    }
                }
            byte[] codestream;
            if (frequency && profileSettings != null)
                error = JxrFrequencyCodestreamWriter.WriteGrayProfile(
                    frequencyPackets, frequencyBitCounts, width, height,
                    profileSettings, out codestream);
            else if (!frequency && profileSettings != null)
                error = JxrCodestreamWriter.WriteGraySpatialProfile(
                    entropyPackets[0], entropyBitCounts[0], width, height,
                    profileSettings, out codestream);
            else if (frequency)
                error = JxrFrequencyCodestreamWriter.WriteGray(frequencyPackets,
                    frequencyBitCounts, width, height, dcIndex, lpIndex,
                    hpIndex, options.Subbands, scaledArithmetic,
                    options.TrimFlexbits, options.Overlap,
                    options.TileLayout == null ?
                        new JxrTileLayout(new int[] { columns },
                            new int[] { rowsCount }) : options.TileLayout,
                    options.Progressive, out codestream);
            else error = JxrCodestreamWriter.WriteGraySpatialTiles(entropyPackets,
                entropyBitCounts, width, height, dcIndex, lpIndex, hpIndex,
                options.Subbands, scaledArithmetic, options.TrimFlexbits,
                options.Overlap, options.TileLayout == null ?
                    new JxrTileLayout(new int[] { columns },
                        new int[] { rowsCount }) : options.TileLayout,
                out codestream);
            if (error != JxrError.None) return error;
            return JxrContainerWriter.WriteGray8(codestream, width, height,
                95.9866f, 95.9866f, out jxr);
        }

        private static byte QpIndex(int overrideIndex, int qualityIndex)
        {
            int index = overrideIndex < 0 ? qualityIndex : overrideIndex;
            return (byte)(index < 2 ? 0 : index);
        }

        internal static void ForwardMacroblock(int[] values)
        {
            ForwardMacroblock(values, false);
        }

        internal static void ForwardMacroblock(int[] values, bool normalizeChroma)
        {
            for (int block = 0; block < 16; block++)
            {
                int offset = block * 16;
                int[] work = new int[16];
                Array.Copy(values, offset, work, 0, 16);
                JxrTransformMath.ApplyFirstStageFourButterfly(work);
                JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
                JxrForwardTransformMath.ApplyOddOdd(
                    ref work[15], ref work[14], ref work[13], ref work[12]);
                JxrForwardTransformMath.ApplyOdd(
                    ref work[5], ref work[4], ref work[7], ref work[6]);
                JxrForwardTransformMath.ApplyOdd(
                    ref work[10], ref work[8], ref work[11], ref work[9]);
                Array.Copy(work, 0, values, offset, 16);
            }
            // Native scaled YUV444 divides the sixteen chroma DCs before
            // stage 2, not after quantization. The arithmetic shift matters
            // for negative odd coefficients.
            if (normalizeChroma)
                for (int offset = 0; offset < 256; offset += 16)
                    values[offset] >>= 1;
            JxrTransformMath.ApplySecondStageFourButterfly(values);
            JxrTransformMath.ApplyDct2x2Up(values, 0, 64, 16, 80);
            JxrForwardTransformMath.ApplyOddOdd(
                ref values[160], ref values[224], ref values[176], ref values[240]);
            JxrForwardTransformMath.ApplyOdd(
                ref values[128], ref values[192], ref values[144], ref values[208]);
            JxrForwardTransformMath.ApplyOdd(
                ref values[32], ref values[48], ref values[96], ref values[112]);
        }

    }
}
