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
                null, out jxr);
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
                    session, trace, out jxr);
        }

        private static JxrError EncodeWithSession(byte[] pixels,
            int stride, int width, int height, JxrEncoderOptions options,
            JxrEncoderSession session, JxrGrayEncodingTrace trace, out byte[] jxr)
        {
            jxr = null;
            JxrError error;
            int columns = (width + 15) / 16, rowsCount = (height + 15) / 16;
            int[] coefficients = new int[256];
            int[][] planeArrays = { coefficients };
            JxrCoefficientPlaneState planes = new JxrCoefficientPlaneState(
                planeArrays, JxrCoefficientColorFormat.Other, 1);
            JxrMacroblockState macroblock = new JxrMacroblockState(1);
            byte dcIndex = QpIndex(options.DcQuantizerIndex, options.QualityIndex);
            byte lpIndex = QpIndex(options.LowpassQuantizerIndex, options.QualityIndex);
            byte hpIndex = QpIndex(options.HighpassQuantizerIndex, options.QualityIndex);
            bool scaledArithmetic = options.Subbands != JxrGraySubbandMode.All ||
                dcIndex > 1 || lpIndex > 1 || hpIndex > 1;
            JxrQuantizer dcQuantizer = JxrQuantization.Remap(dcIndex, scaledArithmetic, false);
            JxrQuantizer lpQuantizer = JxrQuantization.Remap(lpIndex, scaledArithmetic, false);
            JxrQuantizer hpQuantizer = JxrQuantization.Remap(hpIndex, scaledArithmetic, false);
            JxrQuantizerSet quantizers = new JxrQuantizerSet(
                new JxrQuantizer[] { dcQuantizer.WithDcOffset() },
                new JxrQuantizer[][] { new JxrQuantizer[] { lpQuantizer } },
                new JxrQuantizer[][] { new JxrQuantizer[] { hpQuantizer } });
            JxrCoefficientPredictionRows rows =
                new JxrCoefficientPredictionRows(columns, 1);
            JxrCodecConfiguration format = new JxrCodecConfiguration(
                JxrCodecColorFormat.YOnly, 1, true,
                options.Subbands == JxrGraySubbandMode.DcOnly,
                (int)options.Subbands < (int)JxrGraySubbandMode.NoHighpass,
                options.Subbands != JxrGraySubbandMode.NoFlexbits,
                options.Subbands == JxrGraySubbandMode.NoFlexbits,
                true, true, false, false, 0, 0, 1, 1,
                new int[][] { new int[] { hpQuantizer.Parameter } });
            JxrBitReader unused = new JxrBitReader(new byte[0]);
            JxrCodecState state = new JxrCodecState(format, unused, unused, unused, unused);
            int[] dc = new int[16];
            JxrBitWriter writer = new JxrBitWriter();
            int[] topCbp = new int[columns];
            for (int mbY = 0; mbY < rowsCount; mbY++)
            {
                if (mbY != 0) rows.AdvanceRow();
                int leftCbp = 0;
                for (int mbX = 0; mbX < columns; mbX++)
                {
                    state.SetMacroblockPosition(mbX, mbY, columns);
                    state.SetNeighborCbp(0, topCbp[mbX], leftCbp);
                    for (int y = 0; y < 16; y++)
                        for (int x = 0; x < 16; x++)
                        {
                            int pixelX = Math.Min(width - 1, mbX * 16 + x);
                            int pixelY = Math.Min(height - 1, mbY * 16 + y);
                            int block = (x >> 2) * 64 + (y >> 2) * 16;
                            int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                            int centered = pixels[pixelY * stride + pixelX] - 128;
                            // The native encoder keeps three fractional bits
                            // for lossy integer profiles (SHIFTZERO + QPFRACBITS).
                            // Preserve that scale through the integer transform.
                            coefficients[block + local] = scaledArithmetic
                                ? unchecked(centered << 3) : centered;
                        }
                    ForwardMacroblock(coefficients);
                    int[] transformed = trace == null ? null :
                        (int[])coefficients.Clone();
                    error = JxrQuantization.QuantizeMacroblock(planes, macroblock,
                        quantizers, JxrCodecColorFormat.YOnly, 1,
                        options.Subbands == JxrGraySubbandMode.DcOnly,
                (int)options.Subbands >= (int)JxrGraySubbandMode.NoHighpass, false);
                    if (error != JxrError.None) return error;
                    int[] quantized = trace == null ? null :
                        (int[])coefficients.Clone();
                    error = JxrCoefficientPrediction.Encode(macroblock, planes,
                        rows, JxrCodecColorFormat.YOnly, mbX, mbX == 0, mbY == 0);
                    if (error != JxrError.None) return error;
                    int[] predicted = trace == null ? null :
                        (int[])coefficients.Clone();
                    for (int index = 0; index < 16; index++)
                    {
                        error = macroblock.GetDcCoefficient(0, index, out dc[index]);
                        if (error != JxrError.None) return error;
                    }
                    int dcEnd, lpEnd, hpEnd;
                    int macroblockBitStart = writer.BitCount;
                    error = JxrMinimalEntropyEncoder.EncodeMacroblock(state,
                        coefficients, dc, macroblock.Orientation,
                        (int)options.Subbands, options.TrimFlexbits, writer,
                        out dcEnd, out lpEnd, out hpEnd);
                    if (error != JxrError.None) return error;
                    if (trace != null)
                        trace.Add(new JxrGrayMacroblockTrace(mbX, mbY,
                            transformed, quantized, predicted,
                            macroblockBitStart, dcEnd,
                            dcEnd, lpEnd, lpEnd, hpEnd));
                    int previousTop;
                    state.GetNeighborCbp(0, out previousTop, out leftCbp);
                    topCbp[mbX] = leftCbp;
                }
            }
            int entropyBitCount = writer.BitCount;
            writer.AlignByte();
            byte[] codestream;
            error = JxrCodestreamWriter.WriteGraySpatial(writer.ToArray(),
                width, height, dcIndex, lpIndex, hpIndex, options.Subbands,
                scaledArithmetic, options.TrimFlexbits, entropyBitCount,
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
