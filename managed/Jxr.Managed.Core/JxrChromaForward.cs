using System;

namespace Jxr.Managed.Core
{
    // The compact 4:2:2 and 4:2:0 coefficient layouts retain native
    // column-major 4x4 blocks. No-overlap transform; overlap is staged
    // separately because it crosses macroblock boundaries.
    internal static class JxrChromaForward
    {
        private static readonly int[] LocalSampleOrder =
            { 0,1,5,4,2,3,7,6,10,11,15,14,8,9,13,12 };

        internal static void LoadAndTransform(int[] source, int sourceWidth,
            int macroblockX, int macroblockY, JxrCodecColorFormat format,
            bool scaledArithmetic, int[] coefficients)
        {
            int blockHeight = format == JxrCodecColorFormat.Yuv420 ? 8 : 16;
            int baseX = macroblockX * 8, baseY = macroblockY * blockHeight;
            for (int y = 0; y < blockHeight; y++)
                for (int x = 0; x < 8; x++)
                {
                    int block = (x >> 2) * blockHeight * 4 + (y >> 2) * 16;
                    int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                    coefficients[block + local] =
                        source[(baseY + y) * sourceWidth + baseX + x];
                }
            for (int offset = 0; offset < coefficients.Length; offset += 16)
            {
                int[] work = new int[16];
                Array.Copy(coefficients, offset, work, 0, 16);
                JxrTransformMath.ApplyFirstStageFourButterfly(work);
                JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
                JxrForwardTransformMath.ApplyOddOdd(ref work[15],
                    ref work[14], ref work[13], ref work[12]);
                JxrForwardTransformMath.ApplyOdd(ref work[5], ref work[4],
                    ref work[7], ref work[6]);
                JxrForwardTransformMath.ApplyOdd(ref work[10], ref work[8],
                    ref work[11], ref work[9]);
                Array.Copy(work, 0, coefficients, offset, 16);
            }
            if (format == JxrCodecColorFormat.Yuv420)
            {
                if (scaledArithmetic)
                    JxrForwardTransformMath.ApplyDct2x2Down(
                        ref coefficients[0], ref coefficients[32],
                        ref coefficients[16], ref coefficients[48]);
                else
                    JxrTransformMath.ApplyDct2x2Down(coefficients,
                        0, 32, 16, 48);
            }
            else
            {
                if (scaledArithmetic)
                {
                    JxrForwardTransformMath.ApplyDct2x2Down(
                        ref coefficients[0], ref coefficients[64],
                        ref coefficients[16], ref coefficients[80]);
                    JxrForwardTransformMath.ApplyDct2x2Down(
                        ref coefficients[32], ref coefficients[96],
                        ref coefficients[48], ref coefficients[112]);
                }
                else
                {
                    JxrTransformMath.ApplyDct2x2Down(coefficients,
                        0, 64, 16, 80);
                    JxrTransformMath.ApplyDct2x2Down(coefficients,
                        32, 96, 48, 112);
                }
                coefficients[32] -= coefficients[0];
                coefficients[0] += (coefficients[32] + 1) >> 1;
            }
        }
    }
}
