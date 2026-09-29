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
            jxr = null;
            JxrSessionConfiguration sessionConfig = new JxrSessionConfiguration(
                width, height, 0, 1, 4, false);
            using (JxrEncoderSession session = JxrEncoderSession.Create(
                sessionConfig, 0, 0))
                return EncodeWithSession(pixels, stride, width, height, session, out jxr);
        }

        private static JxrError EncodeWithSession(byte[] pixels,
            int stride, int width, int height, JxrEncoderSession session, out byte[] jxr)
        {
            jxr = null;
            JxrError error;
            int columns = (width + 15) / 16, rowsCount = (height + 15) / 16;
            int[] coefficients = new int[256];
            int[][] planeArrays = { coefficients };
            JxrCoefficientPlaneState planes = new JxrCoefficientPlaneState(
                planeArrays, JxrCoefficientColorFormat.Other, 1);
            JxrMacroblockState macroblock = new JxrMacroblockState(1);
            JxrQuantizer lossless = JxrQuantization.Remap(0, false, false);
            JxrQuantizerSet quantizers = new JxrQuantizerSet(
                new JxrQuantizer[] { lossless.WithDcOffset() },
                new JxrQuantizer[][] { new JxrQuantizer[] { lossless } },
                new JxrQuantizer[][] { new JxrQuantizer[] { lossless } });
            JxrCoefficientPredictionRows rows =
                new JxrCoefficientPredictionRows(columns, 1);
            JxrCodecConfiguration format = new JxrCodecConfiguration(
                JxrCodecColorFormat.YOnly, 1, true, false, true, true,
                false, true, true, false, false, 0, 0, 1, 1,
                new int[][] { new int[] { 1 } });
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
                            coefficients[block + local] =
                                pixels[pixelY * stride + pixelX] - 128;
                        }
                    ForwardMacroblock(coefficients);
                    error = JxrQuantization.QuantizeMacroblock(planes, macroblock,
                        quantizers, JxrCodecColorFormat.YOnly, 1, false, false, false);
                    if (error != JxrError.None) return error;
                    error = JxrCoefficientPrediction.Encode(macroblock, planes,
                        rows, JxrCodecColorFormat.YOnly, mbX, mbX == 0, mbY == 0);
                    if (error != JxrError.None) return error;
                    for (int index = 0; index < 16; index++)
                    {
                        error = macroblock.GetDcCoefficient(0, index, out dc[index]);
                        if (error != JxrError.None) return error;
                    }
                    int dcEnd, lpEnd, hpEnd;
                    error = JxrMinimalEntropyEncoder.EncodeMacroblock(state,
                        coefficients, dc, macroblock.Orientation, writer,
                        out dcEnd, out lpEnd, out hpEnd);
                    if (error != JxrError.None) return error;
                    int previousTop;
                    state.GetNeighborCbp(0, out previousTop, out leftCbp);
                    topCbp[mbX] = leftCbp;
                }
            }
            writer.AlignByte();
            byte[] codestream;
            error = JxrCodestreamWriter.WriteGraySpatial(writer.ToArray(),
                width, height, 0, out codestream);
            if (error != JxrError.None) return error;
            return JxrContainerWriter.WriteGray8(codestream, width, height,
                95.9866f, 95.9866f, out jxr);
        }

        private static void ForwardMacroblock(int[] values)
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
