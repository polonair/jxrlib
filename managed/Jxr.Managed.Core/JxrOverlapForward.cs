using System;

namespace Jxr.Managed.Core
{
    // The two-stage native forward transform operates on the previous and
    // current macroblock rows, including a flush column and row. An explicit
    // padded array replaces the native moving pointers and negative offsets.
    internal static class JxrOverlapForward
    {
        private static readonly int[] SampleOrder =
            { 0, 1, 5, 4, 2, 3, 7, 6, 10, 11, 15, 14, 8, 9, 13, 12 };

        internal static int[][] Transform(int[] pixels, int width, int height,
            int overlap, bool scaledChroma)
        {
            int columns = (width + 15) / 16;
            int rows = (height + 15) / 16;
            int rowStride = (columns + 2) * 256;
            int[] buffer = new int[(rows + 2) * rowStride];
            int[][] result = new int[columns * rows][];
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < columns; x++)
                {
                    int start = Base(rowStride, x, y);
                    for (int localY = 0; localY < 16; localY++)
                        for (int localX = 0; localX < 16; localX++)
                        {
                            int sx = Math.Min(width - 1, x * 16 + localX);
                            int sy = Math.Min(height - 1, y * 16 + localY);
                            int block = (localX >> 2) * 64 + (localY >> 2) * 16;
                            int local = SampleOrder[(localY & 3) * 4 + (localX & 3)];
                            buffer[start + block + local] = pixels[sy * width + sx];
                        }
                }
            for (int y = 0; y <= rows; y++)
                for (int x = 0; x <= columns; x++)
                {
                    int first = Base(rowStride, x, y - 1);
                    int second = Base(rowStride, x, y);
                    Process(buffer, first, second, x == 0, x == columns,
                        y == 0, y == rows, overlap);
                    if (x > 0 && y > 0)
                    {
                        int start = first - 256;
                        if (scaledChroma)
                            for (int block = 0; block < 16; block++)
                                buffer[start + block * 16] >>= 1;
                        Stage2(buffer, start);
                        int[] coefficients = new int[256];
                        Array.Copy(buffer, start, coefficients, 0, 256);
                        result[(y - 1) * columns + x - 1] = coefficients;
                    }
                }
            return result;
        }

        private static int Base(int rowStride, int x, int y)
        { return (y + 1) * rowStride + (x + 1) * 256; }

        private static void Pre4(int[] a, int x, int y, int z, int w)
        { JxrForwardTransformMath.ApplyPre4(ref a[x], ref a[y], ref a[z], ref a[w]); }

        private static void PreStage1(int[] a, int first, int second, int offset)
        {
            int bottomFirst = first + 12, bottomSecond = second + 4;
            int topFirst = first + 72 - offset, topSecond = second + 64 - offset;
            for (int column = 0; column < 4; column++)
                JxrForwardTransformMath.ApplyHst4(ref a[bottomFirst + column],
                    ref a[topFirst + column], ref a[bottomSecond + column],
                    ref a[topSecond + column]);
            for (int column = 0; column < 4; column++)
                JxrForwardTransformMath.ApplyHst1(ref a[bottomFirst + column],
                    ref a[topSecond + column]);
            JxrForwardTransformMath.RotateHalf(ref a[bottomSecond + 2], ref a[bottomSecond + 3]);
            JxrForwardTransformMath.RotateHalf(ref a[bottomSecond], ref a[bottomSecond + 1]);
            JxrForwardTransformMath.RotateHalf(ref a[topFirst + 1], ref a[topFirst + 3]);
            JxrForwardTransformMath.RotateHalf(ref a[topFirst], ref a[topFirst + 2]);
            JxrForwardTransformMath.ApplyOddOddPre(ref a[topSecond],
                ref a[topSecond + 1], ref a[topSecond + 2], ref a[topSecond + 3]);
            for (int column = 0; column < 4; column++)
                JxrTransformMath.ApplyDct2x2Down(a, bottomFirst + column,
                    topFirst + column, bottomSecond + column, topSecond + column);
        }

        private static void PreStage2(int[] a, int first, int second)
        {
            int[] f = { -96, -32, -80, -16 };
            int[] g = { 96, 32, 112, 48 };
            int[] h = { -112, -48, -128, -64 };
            int[] k = { 80, 16, 64, 0 };
            for (int i = 0; i < 4; i++)
                JxrForwardTransformMath.ApplyHst4(ref a[first + f[i]],
                    ref a[first + g[i]], ref a[second + h[i]],
                    ref a[second + k[i]]);
            for (int i = 0; i < 4; i++)
                JxrForwardTransformMath.ApplyHst1(ref a[first + f[i]],
                    ref a[second + k[i]]);
            JxrForwardTransformMath.RotateHalf(ref a[second - 48], ref a[second - 112]);
            JxrForwardTransformMath.RotateHalf(ref a[second - 64], ref a[second - 128]);
            JxrForwardTransformMath.RotateHalf(ref a[first + 112], ref a[first + 96]);
            JxrForwardTransformMath.RotateHalf(ref a[first + 48], ref a[first + 32]);
            JxrForwardTransformMath.ApplyOddOddPre(ref a[second], ref a[second + 64],
                ref a[second + 16], ref a[second + 80]);
            for (int i = 0; i < 4; i++)
                JxrTransformMath.ApplyDct2x2Down(a, first + f[i],
                    second + h[i], first + g[i], second + k[i]);
        }

        private static void Stage1(int[] a, int start)
        {
            int[] work = new int[16];
            Array.Copy(a, start, work, 0, 16);
            JxrTransformMath.ApplyFirstStageFourButterfly(work);
            JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
            JxrForwardTransformMath.ApplyOddOdd(ref work[15], ref work[14],
                ref work[13], ref work[12]);
            JxrForwardTransformMath.ApplyOdd(ref work[5], ref work[4],
                ref work[7], ref work[6]);
            JxrForwardTransformMath.ApplyOdd(ref work[10], ref work[8],
                ref work[11], ref work[9]);
            Array.Copy(work, 0, a, start, 16);
        }

        private static void Stage2(int[] a, int start)
        {
            int[] work = new int[256];
            Array.Copy(a, start, work, 0, 256);
            JxrTransformMath.ApplySecondStageFourButterfly(work);
            JxrTransformMath.ApplyDct2x2Up(work, 0, 64, 16, 80);
            JxrForwardTransformMath.ApplyOddOdd(ref work[160], ref work[224],
                ref work[176], ref work[240]);
            JxrForwardTransformMath.ApplyOdd(ref work[128], ref work[192],
                ref work[144], ref work[208]);
            JxrForwardTransformMath.ApplyOdd(ref work[32], ref work[48],
                ref work[96], ref work[112]);
            Array.Copy(work, 0, a, start, 256);
        }

        private static void Process(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom, int overlap)
        {
            if (overlap != 0)
            {
                if (top && left) Pre4(a, second, second + 1, second + 2, second + 3);
                if (top && right) Pre4(a, second - 59, second - 60, second - 57, second - 58);
                if (bottom && left) Pre4(a, first + 58, first + 59, first + 56, first + 57);
                if (bottom && right) Pre4(a, first - 1, first - 2, first - 3, first - 4);
                if (!right && !bottom)
                {
                    if (top)
                    {
                        for (int offset = left ? 0 : -64; offset < 192; offset += 64)
                        {
                            int p = second + offset;
                            Pre4(a, p + 5, p + 4, p + 64, p + 65);
                            Pre4(a, p + 7, p + 6, p + 66, p + 67);
                        }
                    }
                    else
                        for (int offset = left ? 0 : -64; offset < 192; offset += 64)
                            PreStage1(a, first + 48 + offset, second + offset, 0);
                    if (left)
                    {
                        if (!top)
                        {
                            Pre4(a, first + 58, first + 56, second, second + 2);
                            Pre4(a, first + 59, first + 57, second + 1, second + 3);
                        }
                        for (int offset = -64; offset < -16; offset += 16)
                        {
                            int p = second + offset;
                            Pre4(a, p + 74, p + 72, p + 80, p + 82);
                            Pre4(a, p + 75, p + 73, p + 81, p + 83);
                        }
                    }
                    else
                        for (int offset = -64; offset < -16; offset += 16)
                            PreStage1(a, second + offset, second + offset + 16, 0);
                    StagePre1(a, second);
                    StagePre1(a, second + 16);
                    StagePre1(a, second + 32);
                    StagePre1(a, second + 64);
                    StagePre1(a, second + 80);
                    StagePre1(a, second + 96);
                    StagePre1(a, second + 128);
                    StagePre1(a, second + 144);
                    StagePre1(a, second + 160);
                }
                if (bottom)
                    for (int offset = left ? 48 : -16;
                        offset < (right ? -16 : 240); offset += 64)
                    {
                        int p = first + offset;
                        Pre4(a, p + 15, p + 14, p + 74, p + 75);
                        Pre4(a, p + 13, p + 12, p + 72, p + 73);
                    }
                if (right && !bottom)
                {
                    if (!top)
                    {
                        Pre4(a, first - 1, first - 3, second - 59, second - 57);
                        Pre4(a, first - 2, first - 4, second - 60, second - 58);
                    }
                    for (int offset = -64; offset < -16; offset += 16)
                    {
                        int p = second + offset;
                        Pre4(a, p + 15, p + 13, p + 21, p + 23);
                        Pre4(a, p + 14, p + 12, p + 20, p + 22);
                    }
                }
            }
            if (!top)
                for (int offset = left ? 48 : -16;
                    offset < (right ? 48 : 240); offset += 64)
                    Stage1(a, first + offset);
            if (!bottom)
                for (int offset = left ? 0 : -64;
                    offset < (right ? 0 : 192); offset += 64)
                {
                    Stage1(a, second + offset);
                    Stage1(a, second + offset + 16);
                    Stage1(a, second + offset + 32);
                }
            if (overlap == 2)
            {
                if (top && left) Pre4(a, second, second + 64, second + 16, second + 80);
                if (top && right) Pre4(a, second - 128, second - 64, second - 112, second - 48);
                if (bottom && left) Pre4(a, first + 32, first + 96, first + 48, first + 112);
                if (bottom && right) Pre4(a, first - 96, first - 32, first - 80, first - 16);
                if ((left || right) && !(top || bottom))
                {
                    if (left)
                    {
                        Pre4(a, first + 32, first + 48, second, second + 16);
                        Pre4(a, first + 96, first + 112, second + 64, second + 80);
                    }
                    if (right)
                    {
                        Pre4(a, first - 96, first - 80, second - 128, second - 112);
                        Pre4(a, first - 32, first - 16, second - 64, second - 48);
                    }
                }
                if (!(left || right))
                {
                    if (top || bottom)
                    {
                        int p = top ? second : first + 32;
                        Pre4(a, p - 128, p - 64, p, p + 64);
                        Pre4(a, p - 112, p - 48, p + 16, p + 80);
                    }
                    else PreStage2(a, first, second);
                }
            }
        }

        private static void StagePre1(int[] a, int start)
        { PreStage1(a, start, start + 16, 0); }
    }
}
