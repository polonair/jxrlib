using System;

namespace Jxr.Managed.Core
{
    // Full-resolution BD_8 input/output boundary. The three planes are
    // row-major signed PixelI values; transform and entropy stages own them.
    // This is intentionally independent of BMP layout and native codec state.
    public static class JxrImagePipeline
    {
        private static bool ValidPixels(int width, int height, int stride,
            int bytesPerPixel, int bufferLength)
        {
            if (width <= 0 || height <= 0 || stride < 0) return false;
            long rowBytes = (long)width * bytesPerPixel;
            long count = (long)width * height;
            long lastByte = (long)(height - 1) * stride + rowBytes;
            return rowBytes <= stride && count <= Int32.MaxValue &&
                lastByte <= bufferLength;
        }

        private static bool ValidPlane(int[] plane, int width, int height)
        {
            return plane != null && (long)plane.Length >= (long)width * height;
        }

        public static JxrError EncodeGray8(byte[] source, int sourceStride,
            int width, int height, int shift, out int[] luminance)
        {
            luminance = null;
            if (source == null || (shift != 0 && shift != 3) ||
                !ValidPixels(width, height, sourceStride, 1, source.Length))
                return JxrError.InvalidArgument;
            int[] result = new int[width * height];
            for (int row = 0; row < height; row++)
                for (int column = 0; column < width; column++)
                    result[row * width + column] =
                        (source[row * sourceStride + column] - 128) << shift;
            luminance = result;
            return JxrError.None;
        }

        public static JxrError DecodeGray8(int[] luminance, int width, int height,
            int shift, byte[] destination, int destinationStride)
        {
            if (destination == null || (shift != 0 && shift != 3) ||
                !ValidPixels(width, height, destinationStride, 1,
                    destination.Length) || !ValidPlane(luminance, width, height))
                return JxrError.InvalidArgument;
            int bias = (128 << shift) + (shift == 0 ? 0 : 3);
            for (int row = 0; row < height; row++)
                for (int column = 0; column < width; column++)
                {
                    int value = unchecked(luminance[row * width + column] + bias);
                    destination[row * destinationStride + column] =
                        ClipByte(value >> shift);
                }
            return JxrError.None;
        }

        public static JxrError EncodeRgb8(byte[] source, int sourceStride,
            int width, int height, bool rgbOrder, int shift,
            out int[] luminance, out int[] chromaU, out int[] chromaV)
        {
            luminance = chromaU = chromaV = null;
            if (source == null || (shift != 0 && shift != 3) ||
                !ValidPixels(width, height, sourceStride, 3, source.Length))
                return JxrError.InvalidArgument;
            int count = width * height;
            int[] y = new int[count], u = new int[count], v = new int[count];
            for (int row = 0; row < height; row++)
                for (int column = 0; column < width; column++)
                {
                    int sourceIndex = row * sourceStride + column * 3;
                    int red = source[sourceIndex + (rgbOrder ? 0 : 2)] << shift;
                    int green = source[sourceIndex + 1] << shift;
                    int blue = source[sourceIndex + (rgbOrder ? 2 : 0)] << shift;
                    ForwardRgb(ref red, ref green, ref blue);
                    int index = row * width + column;
                    y[index] = unchecked(green - (128 << shift));
                    u[index] = unchecked(-red);
                    v[index] = blue;
                }
            luminance = y; chromaU = u; chromaV = v;
            return JxrError.None;
        }

        public static JxrError DecodeRgb8(int[] luminance, int[] chromaU,
            int[] chromaV, int width, int height, bool rgbOrder, int shift,
            byte[] destination, int destinationStride)
        {
            if (destination == null || (shift != 0 && shift != 3) ||
                !ValidPixels(width, height, destinationStride, 3,
                    destination.Length) || !ValidPlane(luminance, width, height) ||
                !ValidPlane(chromaU, width, height) ||
                !ValidPlane(chromaV, width, height))
                return JxrError.InvalidArgument;
            int bias = (128 << shift) + (shift == 0 ? 0 : 3);
            for (int row = 0; row < height; row++)
                for (int column = 0; column < width; column++)
                {
                    int index = row * width + column;
                    int red = unchecked(-chromaU[index]);
                    int green = unchecked(luminance[index] + bias);
                    int blue = chromaV[index];
                    InverseRgb(ref red, ref green, ref blue);
                    int destinationIndex = row * destinationStride + column * 3;
                    destination[destinationIndex + (rgbOrder ? 0 : 2)] =
                        ClipByte(red >> shift);
                    destination[destinationIndex + 1] =
                        ClipByte(green >> shift);
                    destination[destinationIndex + (rgbOrder ? 2 : 0)] =
                        ClipByte(blue >> shift);
                }
            return JxrError.None;
        }

        public static void ForwardRgb(ref int red, ref int green, ref int blue)
        {
            unchecked
            {
                blue -= red;
                red += ((blue + 1) >> 1) - green;
                green += red >> 1;
            }
        }

        public static void InverseRgb(ref int red, ref int green, ref int blue)
        {
            unchecked
            {
                green -= red >> 1;
                red -= ((blue + 1) >> 1) - green;
                blue += red;
            }
        }

        public static void ForwardCmyk(ref int cyan, ref int magenta,
            ref int yellow, ref int black)
        {
            unchecked
            {
                yellow -= cyan;
                cyan += ((yellow + 1) >> 1) - magenta;
                magenta += (cyan >> 1) - black;
                black += (magenta + 1) >> 1;
            }
        }

        public static void InverseCmyk(ref int cyan, ref int magenta,
            ref int yellow, ref int black)
        {
            unchecked
            {
                black -= (magenta + 1) >> 1;
                magenta -= (cyan >> 1) - black;
                cyan -= ((yellow + 1) >> 1) - magenta;
                yellow += cyan;
            }
        }

        public static byte ClipByte(int value)
        {
            if (value < 0) return 0;
            if (value > 255) return 255;
            return (byte)value;
        }
    }
}
