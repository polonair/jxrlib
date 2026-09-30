using System;

namespace Jxr.Managed.Core
{
    // Alternate inverse path used by the current JPEG XR stream subversion.
    // A padded coefficient grid gives the native cross-MB stencils explicit
    // array indexes without pointers or aliasing assumptions.
    internal static class JxrChromaOverlapInverse
    {
        internal static int[][] Transform(int[][] coefficients, int columns,
            int rows, JxrCodecColorFormat format, int overlap, bool scaled)
        {
            bool is420 = format == JxrCodecColorFormat.Yuv420;
            int block = is420 ? 64 : 128;
            int stride = (columns + 2) * block;
            int[] a = new int[(rows + 2) * stride];
            int[][] result = new int[columns * rows][];
            int[] before = new int[2], after = new int[2];
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < columns; x++)
                    Array.Copy(coefficients[y * columns + x], 0, a,
                        Base(stride, block, x, y), block);
            for (int y = 0; y <= rows; y++)
                for (int x = 0; x <= columns; x++)
                {
                    int first = Base(stride, block, x, y - 1);
                    int second = Base(stride, block, x, y);
                    bool left = x == 0, right = x == columns;
                    bool top = y == 0, bottom = y == rows;
                    if (is420)
                        Process420(a, first, second, left, right, top, bottom,
                            x == 1, x == columns - 1, overlap, scaled,
                            before, after);
                    else
                        Process422(a, first, second, left, right, top, bottom,
                            x == 1, x == columns - 1, overlap, scaled,
                            before, after);
                    if (x > 0 && y > 0)
                    {
                        int[] samples = new int[block];
                        Array.Copy(a, first - block, samples, 0, block);
                        result[(y - 1) * columns + x - 1] = samples;
                    }
                }
            return result;
        }

        private static int Base(int stride, int block, int x, int y)
        { return (y + 1) * stride + (x + 1) * block; }

        private static void Post4(int[] a, int x, int y, int z, int w)
        { JxrInverseTransformMath.ApplyAlternatePost4(ref a[x], ref a[y],
            ref a[z], ref a[w]); }

        private static void Post2(int[] a, int x, int y)
        { JxrInverseTransformMath.ApplyAlternatePost2(ref a[x], ref a[y]); }

        private static void Post2x2(int[] a, int x, int y, int z, int w)
        { JxrInverseTransformMath.ApplyAlternatePost2x2(ref a[x], ref a[y],
            ref a[z], ref a[w]); }

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

        private static void PostStage1Split(int[] a, int first, int second,
            int offset)
        {
            int b1 = first + 12, b2 = second + 4;
            int t1 = first + 72 - offset, t2 = second + 64 - offset;
            for (int i = 0; i < 4; i++)
                JxrTransformMath.ApplyDct2x2Down(a, b1 + i, t1 + i,
                    b2 + i, t2 + i);
            JxrInverseTransformMath.ApplyOddOddPost(ref a[t2], ref a[t2 + 1],
                ref a[t2 + 2], ref a[t2 + 3]);
            JxrInverseTransformMath.RotateHalf(ref a[b2 + 2], ref a[b2 + 3]);
            JxrInverseTransformMath.RotateHalf(ref a[b2], ref a[b2 + 1]);
            JxrInverseTransformMath.RotateHalf(ref a[t1 + 1], ref a[t1 + 3]);
            JxrInverseTransformMath.RotateHalf(ref a[t1], ref a[t1 + 2]);
            for (int i = 0; i < 4; i++)
                JxrInverseTransformMath.ApplyAlternateHadamardScale2(
                    ref a[b1 + i], ref a[t2 + i]);
            for (int i = 0; i < 4; i++)
                JxrInverseTransformMath.ApplyHadamardScale4(ref a[b1 + i],
                    ref a[t1 + i], ref a[b2 + i], ref a[t2 + i]);
        }

        private static void PostStage1(int[] a, int start, int offset)
        { PostStage1Split(a, start, start + 16, offset); }

        private static void Process420(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom,
            bool leftAdjacent, bool rightAdjacent, int overlap, bool scaled,
            int[] before, int[] after)
        {
            if (!bottom && !right)
            {
                if (scaled)
                    JxrInverseTransformMath.ApplyScaledDct2x2Down(
                        ref a[second], ref a[second + 32],
                        ref a[second + 16], ref a[second + 48]);
                else JxrTransformMath.ApplyDct2x2Down(a, second,
                    second + 32, second + 16, second + 48);
            }
            if (overlap == 2)
            {
                if (leftAdjacent && top) a[second - 64] -= a[second - 32];
                if (rightAdjacent && top) before[0] = a[second];
                if (right && top) a[second - 32] -= before[0];
                if (leftAdjacent && bottom) a[first - 48] -= a[first - 16];
                if (rightAdjacent && bottom) before[1] = a[first + 16];
                if (right && bottom) a[first - 16] -= before[1];
                if ((left || right) && !(top || bottom))
                {
                    if (left) Post2(a, first + 16, second);
                    if (right) Post2(a, first - 16, second - 32);
                }
                if (!(left || right))
                {
                    if (top || bottom)
                    {
                        if (top) Post2(a, second - 32, second);
                        if (bottom) Post2(a, first - 16, first + 16);
                    }
                    else Post2x2(a, first - 16, first + 16,
                        second - 32, second);
                }
                if (leftAdjacent && top) a[second - 64] += a[second - 32];
                if (rightAdjacent && top) after[0] = a[second];
                if (right && top) a[second - 32] += after[0];
                if (leftAdjacent && bottom) a[first - 48] += a[first - 16];
                if (rightAdjacent && bottom) after[1] = a[first + 16];
                if (right && bottom) a[first - 16] += after[1];
            }
            if (!top)
                for (int o = left ? 48 : leftAdjacent ? -48 : -16;
                    o < (right ? 16 : 48); o += 32)
                    Stage1(a, first + o);
            if (!bottom)
                for (int o = left ? 32 : leftAdjacent ? -64 : -32;
                    o < (right ? 0 : 32); o += 32)
                    Stage1(a, second + o);
            if (overlap == 0) return;
            if (top && leftAdjacent) Post4(a, second - 64, second - 63,
                second - 62, second - 61);
            if (top && right) Post4(a, second - 27, second - 28,
                second - 25, second - 26);
            if (bottom && leftAdjacent) Post4(a, first - 38, first - 37,
                first - 40, first - 39);
            if (bottom && right) Post4(a, first - 1, first - 2,
                first - 3, first - 4);
            if (!left && !top)
            {
                if (leftAdjacent)
                {
                    if (!bottom)
                    {
                        Post4(a, first - 38, first - 40,
                            second - 64, second - 62);
                        Post4(a, first - 37, first - 39,
                            second - 63, second - 61);
                    }
                    Post4(a, first - 54, first - 56, first - 48, first - 46);
                    Post4(a, first - 53, first - 55, first - 47, first - 45);
                }
                if (bottom)
                {
                    Horizontal420(a, first - 48, true);
                    if (!right) Horizontal420(a, first - 16, true);
                }
                else
                {
                    PostStage1Split(a, first - 48, second - 64, 32);
                    if (!right) PostStage1Split(a, first - 16,
                        second - 32, 32);
                }
                if (right)
                {
                    if (!bottom)
                    {
                        Post4(a, first - 2, first - 4,
                            second - 28, second - 26);
                        Post4(a, first - 1, first - 3,
                            second - 27, second - 25);
                    }
                    Post4(a, first - 18, first - 20, first - 12, first - 10);
                    Post4(a, first - 17, first - 19, first - 11, first - 9);
                }
                else PostStage1(a, first - 32, 32);
                PostStage1(a, first - 64, 32);
            }
            if (top)
            {
                if (!left) Horizontal420(a, second - 60, false);
                if (!left && !right) Horizontal420(a, second - 28, false);
            }
        }

        private static void Horizontal420(int[] a, int edge, bool bottom)
        {
            if (bottom)
            {
                Post4(a, edge + 15, edge + 14, edge + 42, edge + 43);
                Post4(a, edge + 13, edge + 12, edge + 40, edge + 41);
            }
            else
            {
                Post4(a, edge + 1, edge, edge + 28, edge + 29);
                Post4(a, edge + 3, edge + 2, edge + 30, edge + 31);
            }
        }

        private static void Process422(int[] a, int first, int second,
            bool left, bool right, bool top, bool bottom,
            bool leftAdjacent, bool rightAdjacent, int overlap, bool scaled,
            int[] before, int[] after)
        {
            if (!bottom && !right)
            {
                a[second] -= (a[second + 32] + 1) >> 1;
                a[second + 32] += a[second];
                if (scaled)
                {
                    JxrInverseTransformMath.ApplyScaledDct2x2Down(
                        ref a[second], ref a[second + 64],
                        ref a[second + 16], ref a[second + 80]);
                    JxrInverseTransformMath.ApplyScaledDct2x2Down(
                        ref a[second + 32], ref a[second + 96],
                        ref a[second + 48], ref a[second + 112]);
                }
                else
                {
                    JxrTransformMath.ApplyDct2x2Down(a, second,
                        second + 64, second + 16, second + 80);
                    JxrTransformMath.ApplyDct2x2Down(a, second + 32,
                        second + 96, second + 48, second + 112);
                }
            }
            if (overlap == 2)
            {
                if (leftAdjacent && top) a[second - 128] -= a[second - 64];
                if (rightAdjacent && top) before[0] = a[second];
                if (right && top) a[second - 64] -= before[0];
                if (leftAdjacent && bottom) a[first - 80] -= a[first - 16];
                if (rightAdjacent && bottom) before[1] = a[first + 48];
                if (right && bottom) a[first - 16] -= before[1];
                if (!bottom)
                {
                    if (left || right)
                    {
                        if (!top)
                        {
                            if (left) Post2(a, first + 48, second);
                            if (right) Post2(a, first - 16, second - 64);
                        }
                        if (left) Post2(a, second + 16, second + 32);
                        if (right) Post2(a, second - 48, second - 32);
                    }
                    else
                    {
                        if (top) Post2(a, second - 64, second);
                        else Post2x2(a, first - 16, first + 48,
                            second - 64, second);
                        Post2x2(a, second - 48, second + 16,
                            second - 32, second + 32);
                    }
                }
                if (bottom && !(left || right)) Post2(a, first - 16, first + 48);
                if (leftAdjacent && top) a[second - 128] += a[second - 64];
                if (rightAdjacent && top) after[0] = a[second];
                if (right && top) a[second - 64] += after[0];
                if (leftAdjacent && bottom) a[first - 80] += a[first - 16];
                if (rightAdjacent && bottom) after[1] = a[first + 48];
                if (right && bottom) a[first - 16] += after[1];
            }
            if (!top)
                for (int o = left ? 112 : leftAdjacent ? -80 : -16;
                    o < (right ? 48 : 112); o += 64)
                    Stage1(a, first + o);
            if (!bottom)
                for (int o = left ? 64 : leftAdjacent ? -128 : -64;
                    o < (right ? 0 : 64); o += 64)
                {
                    Stage1(a, second + o);
                    Stage1(a, second + o + 16);
                    Stage1(a, second + o + 32);
                }
            if (overlap == 0) return;
            if (top && leftAdjacent) Post4(a, second - 128, second - 127,
                second - 126, second - 125);
            if (top && right) Post4(a, second - 59, second - 60,
                second - 57, second - 58);
            if (bottom && leftAdjacent) Post4(a, first - 70, first - 69,
                first - 72, first - 71);
            if (bottom && right) Post4(a, first - 1, first - 2,
                first - 3, first - 4);
            if (!top)
            {
                if (leftAdjacent) Pair422(a, first - 86);
                if (right) Pair422(a, first - 18);
                for (int o = left ? 0 : -128; o < (right ? -64 : 0); o += 64)
                    PostStage1(a, first + o + 32, 0);
            }
            if (!bottom)
            {
                if (leftAdjacent)
                {
                    Pair422(a, second - 118);
                    Pair422(a, second - 102);
                }
                if (right)
                {
                    Pair422(a, second - 50);
                    Pair422(a, second - 34);
                }
                for (int o = left ? 0 : -128; o < (right ? -64 : 0); o += 64)
                {
                    PostStage1(a, second + o, 0);
                    PostStage1(a, second + o + 16, 0);
                }
            }
            if (top || bottom)
            {
                if (top) Horizontal422(a, second + 5, left, right);
                if (bottom) Horizontal422(a, first + 61, left, right);
            }
            else
            {
                if (leftAdjacent)
                {
                    Post4(a, first - 70, first - 72,
                        second - 128, second - 126);
                    Post4(a, first - 69, first - 71,
                        second - 127, second - 125);
                }
                if (right)
                {
                    Post4(a, first - 2, first - 4,
                        second - 60, second - 58);
                    Post4(a, first - 1, first - 3,
                        second - 59, second - 57);
                }
                for (int o = left ? 0 : -128; o < (right ? -64 : 0); o += 64)
                    PostStage1Split(a, first + o + 48, second + o, 0);
            }
        }

        private static void Pair422(int[] a, int p)
        {
            Post4(a, p, p - 2, p + 6, p + 8);
            Post4(a, p + 1, p - 1, p + 7, p + 9);
        }

        private static void Horizontal422(int[] a, int edge,
            bool left, bool right)
        {
            for (int o = left ? 0 : -128; o < (right ? -64 : 0); o += 64)
            {
                Post4(a, edge + o, edge + o - 1,
                    edge + o + 59, edge + o + 60);
                Post4(a, edge + o + 2, edge + o + 1,
                    edge + o + 61, edge + o + 62);
            }
        }
    }
}
