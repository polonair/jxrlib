using System;
using System.IO;

namespace Jxr.Managed.Core
{
    // JPEG XR boundary: pixels in/out, with no BMP dependency.
    public static class JxrCodec
    {
        public static JxrError Encode(JxrImage image, JxrEncoderOptions options,
            out byte[] jxr)
        {
            return Encode(image, options, out jxr, null);
        }

        public static JxrError Encode(JxrImage image, JxrEncoderOptions options,
            out byte[] jxr, JxrGrayEncodingTrace trace)
        {
            jxr = null;
            if (image == null || options == null) return JxrError.InvalidArgument;
            if (options.QualityIndex < 1 || options.QualityIndex > 255 ||
                options.Overlap < 0 || options.Overlap > 2 ||
                options.DcQuantizerIndex < -1 || options.DcQuantizerIndex > 255 ||
                options.LowpassQuantizerIndex < -1 || options.LowpassQuantizerIndex > 255 ||
                options.HighpassQuantizerIndex < -1 || options.HighpassQuantizerIndex > 255 ||
                options.TrimFlexbits < 0 || options.TrimFlexbits > 15 ||
                (int)options.Subbands < 0 || (int)options.Subbands > 3 ||
                (options.Layout != JxrBitstreamLayout.Spatial &&
                 options.Layout != JxrBitstreamLayout.Frequency))
                return JxrError.InvalidArgument;
            if (image.Format != JxrPixelFormat.Gray8 ||
                options.Overlap != 0 || options.Layout != JxrBitstreamLayout.Spatial)
                return JxrError.UnsupportedFeature;
            return JxrMinimalEncoder.EncodeGrayPixels(image.Pixels,
                image.Stride, image.Width, image.Height, options, trace, out jxr);
        }

        public static JxrError Decode(byte[] jxr, JxrDecoderOptions options,
            out JxrImage image)
        {
            image = null;
            if (jxr == null || options == null) return JxrError.InvalidArgument;
            if (options.OutputFormat != JxrPixelFormat.Gray8 &&
                options.OutputFormat != JxrPixelFormat.Rgb24)
                return JxrError.InvalidArgument;
            if (options.OutputFormat == JxrPixelFormat.Rgb24)
            {
                byte[] rgb;
                int rgbWidth, rgbHeight;
                JxrError rgbError = JxrMinimalDecoder.DecodeRgbPixels(jxr,
                    out rgb, out rgbWidth, out rgbHeight);
                if (rgbError != JxrError.None) return rgbError;
                image = new JxrImage(rgbWidth, rgbHeight,
                    JxrPixelFormat.Rgb24, rgb, rgbWidth * 3);
                return JxrError.None;
            }
            byte[] pixels;
            int width, height;
            JxrError error = JxrMinimalDecoder.DecodeGrayPixels(jxr, out pixels,
                out width, out height);
            if (error != JxrError.None) return error;
            image = new JxrImage(width, height, JxrPixelFormat.Gray8, pixels, width);
            return JxrError.None;
        }

        // Stream overloads buffer one complete JXR in this first integration.
        // They neither seek nor close caller-owned streams.
        public static JxrError Encode(JxrImage image, JxrEncoderOptions options,
            Stream destination)
        {
            if (destination == null) return JxrError.InvalidArgument;
            try
            {
                if (!destination.CanWrite) return JxrError.InvalidArgument;
                byte[] jxr;
                JxrError error = Encode(image, options, out jxr);
                if (error != JxrError.None) return error;
                destination.Write(jxr, 0, jxr.Length);
            }
            catch (IOException) { return JxrError.IoFailure; }
            catch (ObjectDisposedException) { return JxrError.IoFailure; }
            catch (NotSupportedException) { return JxrError.IoFailure; }
            return JxrError.None;
        }

        public static JxrError Decode(Stream source, JxrDecoderOptions options,
            out JxrImage image)
        {
            image = null;
            if (source == null || options == null)
                return JxrError.InvalidArgument;
            try
            {
                if (!source.CanRead) return JxrError.InvalidArgument;
                using (MemoryStream buffer = new MemoryStream())
                {
                    byte[] chunk = new byte[4096];
                    int count;
                    while ((count = source.Read(chunk, 0, chunk.Length)) != 0)
                        buffer.Write(chunk, 0, count);
                    return Decode(buffer.ToArray(), options, out image);
                }
            }
            catch (IOException) { return JxrError.IoFailure; }
            catch (ObjectDisposedException) { return JxrError.IoFailure; }
            catch (NotSupportedException) { return JxrError.IoFailure; }
        }
    }
}
