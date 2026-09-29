using System;

namespace Jxr.Managed.Core
{
    // Optional file-format boundary. The JPEG XR codec itself does not use BMP.
    public static class JxrBmpAdapter
    {
        public static JxrError ReadGray8(byte[] bitmap, out JxrImage image)
        {
            image = null;
            if (bitmap == null) return JxrError.InvalidArgument;
            if (bitmap.Length < 54 || bitmap[0] != 'B' || bitmap[1] != 'M')
                return JxrError.InvalidBitstream;
            int width = Read32(bitmap, 18), height = Read32(bitmap, 22);
            if (width < 1 || height < 1 ||
                (long)width * height > Int32.MaxValue ||
                (long)((width + 3L) & ~3L) * height > Int32.MaxValue - 1078 ||
                bitmap.Length != 1078 + ((width + 3) & ~3) * height ||
                Read32(bitmap, 14) != 40 ||
                Read16(bitmap, 26) != 1 || Read16(bitmap, 28) != 8 ||
                Read32(bitmap, 30) != 0 || Read32(bitmap, 38) != 3779 ||
                Read32(bitmap, 42) != 3779 || Read32(bitmap, 10) != 1078)
                return JxrError.UnsupportedFeature;
            for (int index = 0; index < 256; index++)
            {
                int palette = 54 + index * 4;
                if (bitmap[palette] != index || bitmap[palette + 1] != index ||
                    bitmap[palette + 2] != index || bitmap[palette + 3] != 0)
                    return JxrError.UnsupportedFeature;
            }
            byte[] pixels = new byte[width * height];
            int bmpStride = (width + 3) & ~3;
            for (int row = 0; row < height; row++)
                Array.Copy(bitmap, 1078 + (height - 1 - row) * bmpStride,
                    pixels, row * width, width);
            image = new JxrImage(width, height, JxrPixelFormat.Gray8, pixels, width);
            return JxrError.None;
        }

        public static JxrError WriteGray8(JxrImage image, out byte[] bitmap)
        {
            bitmap = null;
            if (image == null) return JxrError.InvalidArgument;
            if (image.Format != JxrPixelFormat.Gray8 ||
                (long)((image.Width + 3L) & ~3L) * image.Height >
                Int32.MaxValue - 1078) return JxrError.UnsupportedFeature;
            int bmpStride = (image.Width + 3) & ~3;
            byte[] bmp = new byte[1078 + bmpStride * image.Height];
            bmp[0] = (byte)'B'; bmp[1] = (byte)'M';
            Write32(bmp, 2, bmp.Length);
            Write32(bmp, 10, 1078);
            Write32(bmp, 14, 40);
            Write32(bmp, 18, image.Width);
            Write32(bmp, 22, image.Height);
            Write16(bmp, 26, 1);
            Write16(bmp, 28, 8);
            Write32(bmp, 34, bmpStride * image.Height);
            Write32(bmp, 38, 3779);
            Write32(bmp, 42, 3779);
            for (int index = 0; index < 256; index++)
            {
                int entry = 54 + index * 4;
                bmp[entry] = bmp[entry + 1] = bmp[entry + 2] = (byte)index;
            }
            for (int row = 0; row < image.Height; row++)
                Array.Copy(image.Pixels, row * image.Stride,
                    bmp, 1078 + (image.Height - 1 - row) * bmpStride, image.Width);
            bitmap = bmp;
            return JxrError.None;
        }

        private static int Read16(byte[] data, int offset)
        {
            return data[offset] | (data[offset + 1] << 8);
        }

        private static int Read32(byte[] data, int offset)
        {
            return data[offset] | (data[offset + 1] << 8) |
                (data[offset + 2] << 16) | (data[offset + 3] << 24);
        }

        private static void Write16(byte[] data, int offset, int value)
        {
            data[offset] = (byte)value;
            data[offset + 1] = (byte)(value >> 8);
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
