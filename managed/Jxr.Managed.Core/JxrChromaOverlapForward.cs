using System;

namespace Jxr.Managed.Core
{
    // Compact chroma macroblocks use the same delayed row/column schedule as
    // full-resolution luma, but their boundary stencils differ. All offsets
    // below are indexes into one padded managed array.
    internal static class JxrChromaOverlapForward
    {
        private static readonly int[] SampleOrder =
            { 0,1,5,4,2,3,7,6,10,11,15,14,8,9,13,12 };

        internal static int[][] Transform(int[] pixels, int columns, int rows,
            JxrCodecColorFormat format, int overlap, bool scaled)
        {
            bool is420 = format == JxrCodecColorFormat.Yuv420;
            int height = is420 ? 8 : 16;
            int blockSize = is420 ? 64 : 128;
            int sourceWidth = columns * 8;
            int rowStride = (columns + 2) * blockSize;
            int[] a = new int[(rows + 2) * rowStride];
            int[][] result = new int[columns * rows][];
            int[] before = new int[2], after = new int[2];
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < columns; x++)
                {
                    int start = Base(rowStride, blockSize, x, y);
                    for (int py = 0; py < height; py++)
                        for (int px = 0; px < 8; px++)
                        {
                            int block = (px >> 2) * height * 4 + (py >> 2) * 16;
                            int local = SampleOrder[(py & 3) * 4 + (px & 3)];
                            a[start + block + local] = pixels[
                                (y * height + py) * sourceWidth + x * 8 + px];
                        }
                }
            for (int y = 0; y <= rows; y++)
                for (int x = 0; x <= columns; x++)
                {
                    int first = Base(rowStride, blockSize, x, y - 1);
                    int second = Base(rowStride, blockSize, x, y);
                    bool left = x == 0, right = x == columns;
                    bool top = y == 0, bottom = y == rows;
                    if (is420)
                        Process420(a, first, second, left, right, top,
                            bottom, x == 1, x == columns - 1, overlap, scaled,
                            before, after);
                    else
                        Process422(a, first, second, left, right, top,
                            bottom, x == 1, x == columns - 1, overlap, scaled,
                            before, after);
                    if (x > 0 && y > 0)
                    {
                        int[] block = new int[blockSize];
                        Array.Copy(a, first - blockSize, block, 0, blockSize);
                        result[(y - 1) * columns + x - 1] = block;
                    }
                }
            return result;
        }

        private static int Base(int stride, int block, int x, int y)
        { return (y + 1) * stride + (x + 1) * block; }

        private static void Pre4(int[] a, int x, int y, int z, int w)
        { JxrForwardTransformMath.ApplyPre4(ref a[x], ref a[y], ref a[z], ref a[w]); }

        private static void Pre2(int[] a, int x, int y)
        { JxrForwardTransformMath.ApplyPre2(ref a[x], ref a[y]); }

        private static void Pre2x2(int[] a, int x, int y, int z, int w)
        { JxrForwardTransformMath.ApplyPre2x2(ref a[x], ref a[y], ref a[z], ref a[w]); }

        private static void CornerSubtract(int[] a, int at, int predictor)
        { a[at] -= predictor; }

        private static void CornerAdd(int[] a, int at, int predictor)
        { a[at] += predictor; }

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

        private static void PreStage1(int[] a, int first, int second, int offset)
        {
            int b1 = first + 12, b2 = second + 4;
            int t1 = first + 72 - offset, t2 = second + 64 - offset;
            for (int i = 0; i < 4; i++)
                JxrForwardTransformMath.ApplyHst4(ref a[b1 + i],
                    ref a[t1 + i], ref a[b2 + i], ref a[t2 + i]);
            for (int i = 0; i < 4; i++)
                JxrForwardTransformMath.ApplyHst1(ref a[b1 + i], ref a[t2 + i]);
            JxrForwardTransformMath.RotateHalf(ref a[b2 + 2], ref a[b2 + 3]);
            JxrForwardTransformMath.RotateHalf(ref a[b2], ref a[b2 + 1]);
            JxrForwardTransformMath.RotateHalf(ref a[t1 + 1], ref a[t1 + 3]);
            JxrForwardTransformMath.RotateHalf(ref a[t1], ref a[t1 + 2]);
            JxrForwardTransformMath.ApplyOddOddPre(ref a[t2], ref a[t2 + 1],
                ref a[t2 + 2], ref a[t2 + 3]);
            for (int i = 0; i < 4; i++)
                JxrTransformMath.ApplyDct2x2Down(a, b1 + i, t1 + i,
                    b2 + i, t2 + i);
        }

        private static void PreStage1Single(int[] a, int start, int offset)
        { PreStage1(a, start, start + 16, offset); }

        private static void Process420(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom,
            bool leftAdjacent, bool rightAdjacent, int overlap, bool scaled,
            int[] before, int[] after)
        {
            if (overlap != 0)
            {
                if (top && left) Pre4(a, second, second + 1, second + 2, second + 3);
                if (top && right) Pre4(a, second - 27, second - 28, second - 25, second - 26);
                if (bottom && left) Pre4(a, first + 26, first + 27, first + 24, first + 25);
                if (bottom && right) Pre4(a, first - 1, first - 2, first - 3, first - 4);
                if (!right && !bottom)
                {
                    if (top)
                        for (int o = left ? 0 : -32; o < 32; o += 32)
                        {
                            int p = second + o;
                            Pre4(a, p + 5, p + 4, p + 32, p + 33);
                            Pre4(a, p + 7, p + 6, p + 34, p + 35);
                        }
                    else
                        for (int o = left ? 0 : -32; o < 32; o += 32)
                            PreStage1(a, first + 16 + o, second + o, 32);
                    if (left)
                    {
                        if (!top)
                        {
                            Pre4(a, first + 26, first + 24, second, second + 2);
                            Pre4(a, first + 27, first + 25, second + 1, second + 3);
                        }
                        Pre4(a, second + 10, second + 8, second + 16, second + 18);
                        Pre4(a, second + 11, second + 9, second + 17, second + 19);
                    }
                    else PreStage1Single(a, second - 32, 32);
                    PreStage1Single(a, second, 32);
                }
                if (bottom)
                    for (int o = left ? 16 : -16;
                        o < (right ? -16 : 32); o += 32)
                    {
                        int p = first + o;
                        Pre4(a, p + 15, p + 14, p + 42, p + 43);
                        Pre4(a, p + 13, p + 12, p + 40, p + 41);
                    }
                if (right && !bottom)
                {
                    if (!top)
                    {
                        Pre4(a, first - 1, first - 3, second - 27, second - 25);
                        Pre4(a, first - 2, first - 4, second - 28, second - 26);
                    }
                    Pre4(a, second - 17, second - 19, second - 11, second - 9);
                    Pre4(a, second - 18, second - 20, second - 12, second - 10);
                }
            }
            if (!top)
                for (int o = left ? 16 : -16; o < (right ? 16 : 48); o += 32)
                    Stage1(a, first + o);
            if (!bottom)
                for (int o = left ? 0 : -32; o < (right ? 0 : 32); o += 32)
                    Stage1(a, second + o);
            if (overlap == 2)
            {
                if (leftAdjacent && top) CornerSubtract(a, second - 64, a[second - 32]);
                if (rightAdjacent && top) before[0] = a[second];
                if (right && top) CornerSubtract(a, second - 32, before[0]);
                if (leftAdjacent && bottom) CornerSubtract(a, first - 48, a[first - 16]);
                if (rightAdjacent && bottom) before[1] = a[first + 16];
                if (right && bottom) CornerSubtract(a, first - 16, before[1]);
                if ((left || right) && !(top || bottom))
                {
                    if (left) Pre2(a, first + 16, second);
                    if (right) Pre2(a, first - 16, second - 32);
                }
                if (!(left || right))
                {
                    if (top || bottom)
                    {
                        if (top) Pre2(a, second - 32, second);
                        if (bottom) Pre2(a, first - 16, first + 16);
                    }
                    else Pre2x2(a, first - 16, first + 16, second - 32, second);
                }
                if (leftAdjacent && top) CornerAdd(a, second - 64, a[second - 32]);
                if (rightAdjacent && top) after[0] = a[second];
                if (right && top) CornerAdd(a, second - 32, after[0]);
                if (leftAdjacent && bottom) CornerAdd(a, first - 48, a[first - 16]);
                if (rightAdjacent && bottom) after[1] = a[first + 16];
                if (right && bottom) CornerAdd(a, first - 16, after[1]);
            }
            if (!top && !left)
            {
                int p = first - 64;
                if (scaled)
                    JxrForwardTransformMath.ApplyDct2x2Down(ref a[p],
                        ref a[p + 32], ref a[p + 16], ref a[p + 48]);
                else JxrTransformMath.ApplyDct2x2Down(a, p, p + 32,
                    p + 16, p + 48);
            }
        }

        private static void Process422(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom,
            bool leftAdjacent, bool rightAdjacent, int overlap, bool scaled,
            int[] before, int[] after)
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
                        for (int o = left ? 0 : -64; o < 64; o += 64)
                        {
                            int p = second + o;
                            Pre4(a, p + 5, p + 4, p + 64, p + 65);
                            Pre4(a, p + 7, p + 6, p + 66, p + 67);
                        }
                    else
                        for (int o = left ? 0 : -64; o < 64; o += 64)
                            PreStage1(a, first + 48 + o, second + o, 0);
                    if (left)
                    {
                        if (!top)
                        {
                            Pre4(a, first + 58, first + 56, second, second + 2);
                            Pre4(a, first + 59, first + 57, second + 1, second + 3);
                        }
                        for (int o = 0; o < 48; o += 16)
                        {
                            int p = second + o;
                            Pre4(a, p + 10, p + 8, p + 16, p + 18);
                            Pre4(a, p + 11, p + 9, p + 17, p + 19);
                        }
                    }
                    else
                        for (int o = -64; o < -16; o += 16)
                            PreStage1Single(a, second + o, 0);
                    PreStage1Single(a, second, 0);
                    PreStage1Single(a, second + 16, 0);
                    PreStage1Single(a, second + 32, 0);
                }
                if (bottom)
                    for (int o = left ? 48 : -16;
                        o < (right ? -16 : 112); o += 64)
                    {
                        int p = first + o;
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
                    for (int o = -64; o < -16; o += 16)
                    {
                        int p = second + o;
                        Pre4(a, p + 15, p + 13, p + 21, p + 23);
                        Pre4(a, p + 14, p + 12, p + 20, p + 22);
                    }
                }
            }
            if (!top)
                for (int o = left ? 48 : -16; o < (right ? 48 : 112); o += 64)
                    Stage1(a, first + o);
            if (!bottom)
                for (int o = left ? 0 : -64; o < (right ? 0 : 64); o += 64)
                {
                    Stage1(a, second + o);
                    Stage1(a, second + o + 16);
                    Stage1(a, second + o + 32);
                }
            if (overlap == 2)
            {
                if (leftAdjacent && top) CornerSubtract(a, second - 128, a[second - 64]);
                if (rightAdjacent && top) before[0] = a[second];
                if (right && top) CornerSubtract(a, second - 64, before[0]);
                if (leftAdjacent && bottom) CornerSubtract(a, first - 80, a[first - 16]);
                if (rightAdjacent && bottom) before[1] = a[first + 48];
                if (right && bottom) CornerSubtract(a, first - 16, before[1]);
                if (!bottom)
                {
                    if (left || right)
                    {
                        if (!top)
                        {
                            if (left) Pre2(a, first + 48, second);
                            if (right) Pre2(a, first - 16, second - 64);
                        }
                        if (left) Pre2(a, second + 16, second + 32);
                        if (right) Pre2(a, second - 48, second - 32);
                    }
                    else
                    {
                        if (top) Pre2(a, second - 64, second);
                        else Pre2x2(a, first - 16, first + 48, second - 64, second);
                        Pre2x2(a, second - 48, second + 16, second - 32, second + 32);
                    }
                }
                if (bottom && !(left || right)) Pre2(a, first - 16, first + 48);
                if (leftAdjacent && top) CornerAdd(a, second - 128, a[second - 64]);
                if (rightAdjacent && top) after[0] = a[second];
                if (right && top) CornerAdd(a, second - 64, after[0]);
                if (leftAdjacent && bottom) CornerAdd(a, first - 80, a[first - 16]);
                if (rightAdjacent && bottom) after[1] = a[first + 48];
                if (right && bottom) CornerAdd(a, first - 16, after[1]);
            }
            if (!top && !left)
            {
                int p = first - 128;
                if (scaled)
                {
                    JxrForwardTransformMath.ApplyDct2x2Down(ref a[p],
                        ref a[p + 64], ref a[p + 16], ref a[p + 80]);
                    JxrForwardTransformMath.ApplyDct2x2Down(ref a[p + 32],
                        ref a[p + 96], ref a[p + 48], ref a[p + 112]);
                }
                else
                {
                    JxrTransformMath.ApplyDct2x2Down(a, p, p + 64,
                        p + 16, p + 80);
                    JxrTransformMath.ApplyDct2x2Down(a, p + 32, p + 96,
                        p + 48, p + 112);
                }
                a[p + 32] -= a[p];
                a[p] += (a[p + 32] + 1) >> 1;
            }
        }
    }
}
