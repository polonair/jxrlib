using System;

namespace Jxr.Managed.Core
{
    // Full-resolution RGB input is reduced to YUV 4:2:2/4:2:0 chroma using
    // the native five-tap filter. Decoding interpolates between sample sites.
    internal static class JxrChromaResampler
    {
        private static int Filter(int first, int second, int center,
            int fourth, int fifth)
        {
            return unchecked((((second + center + fourth) << 2) +
                (center << 1) + first + fifth + 8) >> 4);
        }

        internal static int[] Downsample(int[] source, int width, int height,
            bool vertical)
        {
            int columns = (width + 15) / 16;
            int rows = (height + 15) / 16;
            int paddedWidth = columns * 16, paddedHeight = rows * 16;
            int halfWidth = paddedWidth / 2;
            int[] horizontal = new int[halfWidth * paddedHeight];
            for (int y = 0; y < paddedHeight; y++)
                for (int x = 0; x < halfWidth; x++)
                {
                    int sx = x * 2;
                    int sampleY = Math.Min(y, height - 1);
                    int left2 = sx == 0 ? 2 : sx - 2;
                    int left1 = sx == 0 ? 1 : sx - 1;
                    int right1 = Math.Min(sx + 1, paddedWidth - 1);
                    int right2 = sx + 2 >= paddedWidth ? sx : sx + 2;
                    horizontal[y * halfWidth + x] = Filter(
                        source[sampleY * width + Math.Min(left2, width - 1)],
                        source[sampleY * width + Math.Min(left1, width - 1)],
                        source[sampleY * width + Math.Min(sx, width - 1)],
                        source[sampleY * width + Math.Min(right1, width - 1)],
                        source[sampleY * width + Math.Min(right2, width - 1)]);
                }
            if (!vertical) return horizontal;
            int[] result = new int[halfWidth * (paddedHeight / 2)];
            for (int y = 0; y < paddedHeight / 2; y++)
                for (int x = 0; x < halfWidth; x++)
                {
                    int sy = y * 2;
                    int top2 = sy == 0 ? 2 : sy - 2;
                    int top1 = sy == 0 ? 1 : sy - 1;
                    int bottom1 = Math.Min(sy + 1, paddedHeight - 1);
                    int bottom2 = sy + 2 >= paddedHeight ? sy : sy + 2;
                    result[y * halfWidth + x] = Filter(
                        horizontal[top2 * halfWidth + x],
                        horizontal[top1 * halfWidth + x],
                        horizontal[sy * halfWidth + x],
                        horizontal[bottom1 * halfWidth + x],
                        horizontal[bottom2 * halfWidth + x]);
                }
            return result;
        }

        internal static int[] Interpolate(int[] chroma, int width, int height,
            int paddedWidth, int paddedHeight, bool vertical)
        {
            int halfWidth = paddedWidth / 2;
            int[] full = new int[width * height];
            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++)
                {
                    int sampleY = vertical ? y / 2 : y;
                    int left = chroma[sampleY * halfWidth + x / 2];
                    if (vertical && (y & 1) != 0)
                    {
                        int nextY = Math.Min(sampleY + 1, paddedHeight / 2 - 1);
                        left = unchecked(left + chroma[nextY * halfWidth + x / 2] + 1) >> 1;
                    }
                    if ((x & 1) != 0 && x / 2 + 1 < halfWidth)
                    {
                        int right = chroma[sampleY * halfWidth + x / 2 + 1];
                        if (vertical && (y & 1) != 0)
                        {
                            int nextY = Math.Min(sampleY + 1, paddedHeight / 2 - 1);
                            right = unchecked(right + chroma[nextY * halfWidth + x / 2 + 1] + 1) >> 1;
                        }
                        left = unchecked(left + right + 1) >> 1;
                    }
                    full[y * width + x] = left;
                }
            return full;
        }
    }
}
