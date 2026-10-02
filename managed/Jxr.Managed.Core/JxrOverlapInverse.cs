using System;

namespace Jxr.Managed.Core
{
    // Scalar inverse companion to JxrOverlapForward. The padded macroblock
    // grid makes every native neighbour reference an explicit array offset.
    internal static class JxrOverlapInverse
    {
        internal static int[][] Transform(int[][] coefficients, int width,
            int height, int overlap, bool scaledChroma, int highpassQuantizer,
            bool highpassAbsent, bool alternateOperators)
        {
            int columns = (width + 15) / 16;
            int rows = (height + 15) / 16;
            int rowStride = (columns + 2) * 256;
            int[] buffer = new int[(rows + 2) * rowStride];
            int[][] result = new int[columns * rows][];
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < columns; x++)
                    Array.Copy(coefficients[y * columns + x], 0, buffer,
                        Base(rowStride, x, y), 256);
            for (int y = 0; y <= rows; y++)
                for (int x = 0; x <= columns; x++)
                {
                    int first = Base(rowStride, x, y - 1);
                    int second = Base(rowStride, x, y);
                    if (x < columns && y < rows)
                    {
                        Stage2(buffer, second);
                        if (scaledChroma)
                            for (int block = 0; block < 16; block++)
                            {
                                int index = second + block * 16;
                                buffer[index] = unchecked(buffer[index] + buffer[index]);
                            }
                    }
                    ProcessStage2(buffer, first, second, x == 0,
                        x == columns, y == 0, y == rows, overlap,
                        alternateOperators);
                    ProcessStage1(buffer, first, second, x == 0,
                        x == columns, y == 0, y == rows, overlap,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                    if (x > 0 && y > 0)
                    {
                        int[] samples = new int[256];
                        Array.Copy(buffer, first - 256, samples, 0, 256);
                        result[(y - 1) * columns + x - 1] = samples;
                    }
                }
            return result;
        }

        private static int Base(int rowStride, int x, int y)
        { return (y + 1) * rowStride + (x + 1) * 256; }

        private static void Post4(int[] a, int x, int y, int z, int w,
            bool alternateOperators)
        {
            if (alternateOperators)
                JxrInverseTransformMath.ApplyAlternatePost4(ref a[x], ref a[y],
                    ref a[z], ref a[w]);
            else
                JxrInverseTransformMath.ApplyPost4(ref a[x], ref a[y],
                    ref a[z], ref a[w]);
        }

        private static void Stage2(int[] a, int start)
        {
            int[] work = new int[256];
            Array.Copy(a, start, work, 0, 256);
            JxrInverseTransformMath.ApplyOdd(ref work[32], ref work[48],
                ref work[96], ref work[112]);
            JxrInverseTransformMath.ApplyOdd(ref work[128], ref work[192],
                ref work[144], ref work[208]);
            JxrInverseTransformMath.ApplyOddOdd(ref work[160], ref work[224],
                ref work[176], ref work[240]);
            JxrTransformMath.ApplyDct2x2Up(work, 0, 64, 16, 80);
            JxrTransformMath.ApplySecondStageFourButterfly(work);
            Array.Copy(work, 0, a, start, 256);
        }

        private static void Stage1(int[] a, int start)
        {
            int[] work = new int[16];
            Array.Copy(a, start, work, 0, 16);
            JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
            JxrInverseTransformMath.ApplyOdd(ref work[5], ref work[4],
                ref work[7], ref work[6]);
            JxrInverseTransformMath.ApplyOdd(ref work[10], ref work[8],
                ref work[11], ref work[9]);
            JxrInverseTransformMath.ApplyOddOdd(ref work[15], ref work[14],
                ref work[13], ref work[12]);
            JxrTransformMath.ApplyFirstStageFourButterfly(work);
            Array.Copy(work, 0, a, start, 16);
        }

        private static void PostStage2Split(int[] a, int first, int second,
            bool alternateOperators)
        {
            int[] f = { -96, -32, -80, -16 };
            int[] g = { 96, 32, 112, 48 };
            int[] h = { -112, -48, -128, -64 };
            int[] k = { 80, 16, 64, 0 };
            for (int i = 0; i < 4; i++)
                JxrTransformMath.ApplyDct2x2Down(a, first + f[i],
                    first + g[i], second + h[i], second + k[i]);
            JxrInverseTransformMath.ApplyOddOddPost(ref a[second],
                ref a[second + 64], ref a[second + 16], ref a[second + 80]);
            JxrInverseTransformMath.RotateHalf(ref a[first + 48], ref a[first + 32]);
            JxrInverseTransformMath.RotateHalf(ref a[first + 112], ref a[first + 96]);
            JxrInverseTransformMath.RotateHalf(ref a[second - 64], ref a[second - 128]);
            JxrInverseTransformMath.RotateHalf(ref a[second - 48], ref a[second - 112]);
            for (int i = 0; i < 4; i++)
            {
                if (alternateOperators)
                    JxrInverseTransformMath.ApplyAlternateHadamardScale2(
                        ref a[first + f[i]], ref a[second + k[i]]);
                else
                    JxrInverseTransformMath.ApplyHadamardScale2(
                        ref a[first + f[i]], ref a[second + k[i]]);
            }
            for (int i = 0; i < 4; i++)
                JxrInverseTransformMath.ApplyHadamardScale4(ref a[first + f[i]],
                    ref a[second + h[i]], ref a[first + g[i]],
                    ref a[second + k[i]]);
        }

        private static void ProcessStage2(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom, int overlap,
            bool alternateOperators)
        {
            if (overlap != 2) return;
            if (top && left) Post4(a, second, second + 64, second + 16, second + 80, alternateOperators);
            if (top && right) Post4(a, second - 128, second - 64, second - 112, second - 48, alternateOperators);
            if (bottom && left) Post4(a, first + 32, first + 96, first + 48, first + 112, alternateOperators);
            if (bottom && right) Post4(a, first - 96, first - 32, first - 80, first - 16, alternateOperators);
            if ((left || right) && !(top || bottom))
            {
                int side = left ? 0 : -128;
                Post4(a, first + side + 32, first + side + 48,
                    second + side, second + side + 16, alternateOperators);
                Post4(a, first + side + 96, first + side + 112,
                    second + side + 64, second + side + 80, alternateOperators);
            }
            if (!(left || right))
            {
                if (top || bottom)
                {
                    int edge = top ? second : first + 32;
                    Post4(a, edge - 128, edge - 64, edge, edge + 64, alternateOperators);
                    Post4(a, edge - 112, edge - 48, edge + 16, edge + 80, alternateOperators);
                }
                else PostStage2Split(a, first, second, alternateOperators);
            }
        }

        private static void PostStage1Split(int[] a, int first, int second,
            int offset, int highpassQuantizer, bool highpassAbsent,
            bool alternateOperators)
        {
            int bottomFirst = first + 12, bottomSecond = second + 4;
            int topFirst = first + 72 - offset, topSecond = second + 64 - offset;
            for (int column = 0; column < 4; column++)
                JxrTransformMath.ApplyDct2x2Down(a, bottomFirst + column,
                    topFirst + column, bottomSecond + column,
                    topSecond + column);
            JxrInverseTransformMath.ApplyOddOddPost(ref a[topSecond],
                ref a[topSecond + 1], ref a[topSecond + 2], ref a[topSecond + 3]);
            JxrInverseTransformMath.RotateHalf(ref a[bottomSecond + 2], ref a[bottomSecond + 3]);
            JxrInverseTransformMath.RotateHalf(ref a[bottomSecond], ref a[bottomSecond + 1]);
            JxrInverseTransformMath.RotateHalf(ref a[topFirst + 1], ref a[topFirst + 3]);
            JxrInverseTransformMath.RotateHalf(ref a[topFirst], ref a[topFirst + 2]);
            for (int column = 0; column < 4; column++)
            {
                if (alternateOperators)
                    JxrInverseTransformMath.ApplyAlternateHadamardScale2(
                        ref a[bottomFirst + column], ref a[topSecond + column]);
                else
                    JxrInverseTransformMath.ApplyHadamardScale2(
                        ref a[bottomFirst + column], ref a[topSecond + column]);
            }
            for (int column = 0; column < 4; column++)
                JxrInverseTransformMath.ApplyHadamardScale4(
                    ref a[bottomFirst + column], ref a[topFirst + column],
                    ref a[bottomSecond + column], ref a[topSecond + column]);
            if (!alternateOperators)
                for (int column = 0; column < 4; column++)
                {
                    int bottomLeft = a[bottomFirst + column];
                    int topLeft = a[topFirst + column];
                    int bottomRight = a[bottomSecond + column];
                    int topRight = a[topSecond + column];
                    int sum = unchecked(bottomLeft + bottomRight);
                    sum = unchecked(sum + topLeft);
                    sum = unchecked(sum + topRight);
                    int temporaryCurrent = sum >> 1;
                    int directCurrent = unchecked(temporaryCurrent * 595 + 65536) >> 17;
                    JxrInverseTransformMath.ApplyConditionalDcCompensation(
                        ref a[bottomFirst + column], ref a[topFirst + column],
                        ref a[bottomSecond + column], ref a[topSecond + column],
                        directCurrent, highpassQuantizer, highpassAbsent);
                }
        }

        private static void PostStage1(int[] a, int start, int qp, bool absent,
            bool alternateOperators)
        { PostStage1Split(a, start, start + 16, 0, qp, absent, alternateOperators); }

        private static void ProcessStage1(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom, int overlap,
            int highpassQuantizer, bool highpassAbsent, bool alternateOperators)
        {
            if (!top)
                for (int offset = left ? 32 : -96;
                    offset < (right ? 32 : 160); offset += 64)
                {
                    Stage1(a, first + offset);
                    Stage1(a, first + offset + 16);
                }
            if (!bottom)
                for (int offset = left ? 0 : -128;
                    offset < (right ? 0 : 128); offset += 64)
                {
                    Stage1(a, second + offset);
                    Stage1(a, second + offset + 16);
                }
            if (overlap == 0) return;
            if (left || right)
            {
                if (top && left) Post4(a, second, second + 1, second + 2, second + 3, alternateOperators);
                if (top && right) Post4(a, second - 59, second - 60, second - 57, second - 58, alternateOperators);
                if (bottom && left) Post4(a, first + 58, first + 59, first + 56, first + 57, alternateOperators);
                if (bottom && right) Post4(a, first - 1, first - 2, first - 3, first - 4, alternateOperators);
                int j = left ? 10 : -50;
                if (!top)
                {
                    int p = first + 16 + j;
                    Post4(a, p, p - 2, p + 6, p + 8, alternateOperators);
                    Post4(a, p + 1, p - 1, p + 7, p + 9, alternateOperators);
                    Post4(a, p + 16, p + 14, p + 22, p + 24, alternateOperators);
                    Post4(a, p + 17, p + 15, p + 23, p + 25, alternateOperators);
                }
                if (!bottom)
                {
                    int p = second + j;
                    Post4(a, p, p - 2, p + 6, p + 8, alternateOperators);
                    Post4(a, p + 1, p - 1, p + 7, p + 9, alternateOperators);
                }
                if (!(top || bottom))
                {
                    Post4(a, first + 48 + j, first + 46 + j,
                        second - 10 + j, second - 8 + j, alternateOperators);
                    Post4(a, first + 49 + j, first + 47 + j,
                        second - 9 + j, second - 7 + j, alternateOperators);
                }
            }
            for (int j = left ? 0 : -192;
                j < (right ? -64 : 64); j += 64)
            {
                if (top)
                {
                    int p = second + j;
                    Post4(a, p + 5, p + 4, p + 64, p + 65, alternateOperators);
                    Post4(a, p + 7, p + 6, p + 66, p + 67, alternateOperators);
                    PostStage1(a, p, highpassQuantizer, highpassAbsent, alternateOperators);
                }
                else if (bottom)
                {
                    PostStage1(a, first + 16 + j,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                    PostStage1(a, first + 32 + j,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                    int p = first + 48 + j;
                    Post4(a, p + 15, p + 14, p + 74, p + 75, alternateOperators);
                    Post4(a, p + 13, p + 12, p + 72, p + 73, alternateOperators);
                }
                else
                {
                    PostStage1(a, first + 16 + j,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                    PostStage1(a, first + 32 + j,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                    PostStage1Split(a, first + 48 + j, second + j, 0,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                    PostStage1(a, second + j,
                        highpassQuantizer, highpassAbsent, alternateOperators);
                }
            }
        }
    }
}
