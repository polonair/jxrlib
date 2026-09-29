using System;

namespace Jxr.Managed.Core
{
    public enum JxrPixelFormat
    {
        Gray8 = 0,
        Rgb24 = 1,
        Bgr24 = 2
    }

    // Rows are top-down. Pixels is caller-owned; this class never copies it.
    public sealed class JxrImage
    {
        private readonly int width;
        private readonly int height;
        private readonly int stride;
        private readonly JxrPixelFormat format;
        private readonly byte[] pixels;

        public JxrImage(int width, int height, JxrPixelFormat format,
            byte[] pixels, int stride)
        {
            int channels = format == JxrPixelFormat.Gray8 ? 1 :
                (format == JxrPixelFormat.Rgb24 ||
                 format == JxrPixelFormat.Bgr24) ? 3 : 0;
            if (channels == 0 || width <= 0 || height <= 0 || pixels == null ||
                (long)width * channels > stride || stride <= 0 ||
                (long)stride * height > pixels.Length)
                throw new ArgumentException("Invalid image dimensions, format or buffer.");
            this.width = width;
            this.height = height;
            this.format = format;
            this.pixels = pixels;
            this.stride = stride;
        }

        public int Width { get { return width; } }
        public int Height { get { return height; } }
        public int Stride { get { return stride; } }
        public JxrPixelFormat Format { get { return format; } }
        public byte[] Pixels { get { return pixels; } }
    }
}
