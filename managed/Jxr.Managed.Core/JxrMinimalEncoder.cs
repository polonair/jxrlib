using System;

namespace Jxr.Managed.Core
{
    // Compatibility BMP entry point and pixel-based core for one lossless
    // 16x16 Y_ONLY macroblock.
    public static class JxrMinimalEncoder
    {
        private static readonly int[] LocalSampleOrder =
            { 0,1,5,4,2,3,7,6,10,11,15,14,8,9,13,12 };

        // Fixed TIFF/JPEG XR syntax through the single spatial packet header.
        // Only the four-byte codestream length at offset 126 varies by image.
        // This is container/header syntax, not fixture entropy or pixel data.
        private static readonly byte[] Header = Convert.FromBase64String(
            "SUm8ASAAAAAkw91vA07+S7GFPXd2jckIAAAAAAAAAAAIAAG8AQAQAAAACAAAAAK8BAABAAAAAAAAAIC8BAABAAAAEAAAAIG8BAABAAAAEAAAAIK8CwABAAAAJPm/QoO8CwABAAAAJPm/QsC8BAABAAAAhgAAAMG8BAABAAAAMAEAAAAAAABXTVBIT1RPABEAwAEADwAPAIAgCAAABG//AAEAAAEA");

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
            jxr = null;
            JxrSessionConfiguration sessionConfig = new JxrSessionConfiguration(
                16, 16, 0, 1, 4, false);
            using (JxrEncoderSession session = JxrEncoderSession.Create(
                sessionConfig, 0, 0))
                return EncodeWithSession(pixels, stride, session, out jxr);
        }

        private static JxrError EncodeWithSession(byte[] pixels,
            int stride, JxrEncoderSession session, out byte[] jxr)
        {
            jxr = null;
            JxrError error;
            int[] coefficients = session.GetPrimaryRow(0, 0);
            for (int y = 0; y < 16; y++)
                for (int x = 0; x < 16; x++)
                {
                    int block = (x >> 2) * 64 + (y >> 2) * 16;
                    int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                    coefficients[block + local] =
                        pixels[y * stride + x] - 128;
                }
            ForwardMacroblock(coefficients);

            int[][] planeArrays = { coefficients };
            JxrCoefficientPlaneState planes = new JxrCoefficientPlaneState(
                planeArrays, JxrCoefficientColorFormat.Other, 1);
            JxrMacroblockState macroblock = new JxrMacroblockState(1);
            JxrQuantizer lossless = JxrQuantization.Remap(0, false, false);
            JxrQuantizerSet quantizers = new JxrQuantizerSet(
                new JxrQuantizer[] { lossless.WithDcOffset() },
                new JxrQuantizer[][] { new JxrQuantizer[] { lossless } },
                new JxrQuantizer[][] { new JxrQuantizer[] { lossless } });
            error = JxrQuantization.QuantizeMacroblock(planes, macroblock,
                quantizers, JxrCodecColorFormat.YOnly, 1, false, false, false);
            if (error != JxrError.None) return error;
            JxrCoefficientPredictionRows rows = new JxrCoefficientPredictionRows(1, 1);
            error = JxrCoefficientPrediction.Encode(macroblock, planes,
                rows, JxrCodecColorFormat.YOnly, 0, true, true);
            if (error != JxrError.None) return error;
            int[] dc = new int[16];
            for (int index = 0; index < 16; index++)
            {
                error = macroblock.GetDcCoefficient(0, index, out dc[index]);
                if (error != JxrError.None) return error;
            }
            JxrBitWriter writer = new JxrBitWriter();
            int dcEnd, lpEnd, hpEnd;
            error = JxrMinimalEntropyEncoder.Encode(coefficients, dc,
                macroblock.Orientation, writer, out dcEnd, out lpEnd, out hpEnd);
            if (error != JxrError.None) return error;
            writer.AlignByte();
            byte[] packet = writer.ToArray();
            byte[] result = new byte[Header.Length + packet.Length];
            Array.Copy(Header, result, Header.Length);
            Array.Copy(packet, 0, result, Header.Length, packet.Length);
            int codestreamLength = result.Length - 134;
            Write32(result, 126, codestreamLength);
            jxr = result;
            return JxrError.None;
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

        private static void Write32(byte[] data, int offset, int value)
        {
            data[offset] = (byte)value;
            data[offset + 1] = (byte)(value >> 8);
            data[offset + 2] = (byte)(value >> 16);
            data[offset + 3] = (byte)(value >> 24);
        }
    }
}
