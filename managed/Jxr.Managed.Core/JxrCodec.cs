using System;
using System.IO;

namespace Jxr.Managed.Core
{
    // JPEG XR boundary: pixels in/out, with no BMP dependency.
    public static class JxrCodec
    {
        // Re-encodes supported frequency RGB and planar-alpha profiles using
        // the stream parameters inspected from the source file.
        public static JxrError Encode(JxrImage image, JxrSourceProfile sourceProfile,
            out byte[] jxr)
        {
            jxr = null;
            JxrProfileEncodingSettings profileSettings;
            JxrError error = JxrProfileEncodingSettings.Create(image,
                sourceProfile, out profileSettings);
            if (error != JxrError.None) return error;
            JxrEncoderOptions options = new JxrEncoderOptions();
            options.Layout = JxrBitstreamLayout.Frequency;
            options.Overlap = sourceProfile.Headers.Main.Overlap;
            options.Subbands = JxrGraySubbandMode.All;
            options.TrimFlexbits = 0;
            options.ChromaSubsampling = JxrChromaSubsampling.Yuv444;
            options.TileLayout = profileSettings.TileLayout;
            options.Progressive = profileSettings.Progressive;
            if (profileSettings.Alpha != null)
                return EncodePlanarAlphaProfile(image, options,
                    profileSettings, out jxr);
            return JxrMinimalColorEncoder.Encode(image, options,
                profileSettings, out jxr);
        }

        private static JxrError EncodePlanarAlphaProfile(JxrImage image,
            JxrEncoderOptions colorOptions,
            JxrProfileEncodingSettings profileSettings, out byte[] jxr)
        {
            jxr = null;
            if (image == null || profileSettings == null ||
                profileSettings.Alpha == null ||
                (image.Format != JxrPixelFormat.Bgra32 &&
                 image.Format != JxrPixelFormat.Pbgra32) ||
                (long)image.Width * image.Height > Int32.MaxValue / 4)
                return JxrError.InvalidArgument;
            byte[] colorPixels = new byte[image.Width * image.Height * 3];
            byte[] alphaPixels = new byte[image.Width * image.Height];
            for (int y = 0; y < image.Height; y++)
                for (int x = 0; x < image.Width; x++)
                {
                    int source = y * image.Stride + x * 4;
                    int color = (y * image.Width + x) * 3;
                    int alpha = y * image.Width + x;
                    colorPixels[color] = image.Pixels[source];
                    colorPixels[color + 1] = image.Pixels[source + 1];
                    colorPixels[color + 2] = image.Pixels[source + 2];
                    alphaPixels[alpha] = image.Pixels[source + 3];
                }

            JxrImage colorImage = new JxrImage(image.Width, image.Height,
                JxrPixelFormat.Bgr24, colorPixels, image.Width * 3);
            byte[] colorContainer;
            JxrError error = JxrMinimalColorEncoder.Encode(colorImage,
                colorOptions, profileSettings, out colorContainer);
            if (error != JxrError.None) return error;

            JxrProfileEncodingSettings alphaProfile = profileSettings.Alpha;
            JxrEncoderOptions alphaOptions = new JxrEncoderOptions();
            alphaOptions.QualityIndex = 1;
            alphaOptions.Overlap = alphaProfile.Overlap;
            alphaOptions.Layout = JxrBitstreamLayout.Frequency;
            alphaOptions.Subbands = JxrGraySubbandMode.All;
            alphaOptions.TileLayout = alphaProfile.TileLayout;
            alphaOptions.Progressive = alphaProfile.Progressive;
            JxrError alphaError = JxrMinimalEncoder.EncodeGrayProfilePixels(alphaPixels,
                image.Width, image.Width, image.Height, alphaOptions,
                alphaProfile, out jxr);
            if (alphaError != JxrError.None) return alphaError;
            byte[] alphaContainer = jxr;
            jxr = null;

            JxrHeaders colorHeaders, alphaHeaders;
            error = JxrHeaders.Read(colorContainer, out colorHeaders);
            if (error != JxrError.None) return error;
            error = JxrHeaders.Read(alphaContainer, out alphaHeaders);
            if (error != JxrError.None) return error;
            byte[] colorStream = Slice(colorContainer,
                colorHeaders.CodestreamOffset, colorHeaders.CodestreamLength);
            byte[] alphaStream = Slice(alphaContainer,
                alphaHeaders.CodestreamOffset, alphaHeaders.CodestreamLength);
            bool byteCount = profileSettings.AlphaRangeInterpretation ==
                JxrAlphaRangeInterpretation.ByteCount;
            if (!byteCount && profileSettings.AlphaRangeInterpretation !=
                JxrAlphaRangeInterpretation.AbsoluteEndOffset)
                return JxrError.UnsupportedFeature;
            return JxrContainerWriter.WriteRgbaPlanar(colorStream, alphaStream,
                image.Width, image.Height, profileSettings.PixelFormatGuid,
                byteCount, profileSettings.HorizontalDpi,
                profileSettings.VerticalDpi, out jxr);
        }

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
                options.AlphaQualityIndex < 1 || options.AlphaQualityIndex > 255 ||
                (options.AlphaMode != JxrAlphaMode.None &&
                 options.AlphaMode != JxrAlphaMode.Planar &&
                 options.AlphaMode != JxrAlphaMode.Interleaved) ||
                (int)options.Subbands < 0 || (int)options.Subbands > 3 ||
                (int)options.ChromaSubsampling < 1 ||
                (int)options.ChromaSubsampling > 3 ||
                (options.Layout != JxrBitstreamLayout.Spatial &&
                 options.Layout != JxrBitstreamLayout.Frequency))
                return JxrError.InvalidArgument;
            if (trace != null && options.Layout == JxrBitstreamLayout.Frequency)
                return JxrError.UnsupportedFeature;
            JxrTileGeometry tileGeometry;
            JxrError tileError = JxrTileGeometry.Create(image.Width,
                image.Height, options.TileLayout, out tileGeometry);
            if (tileError != JxrError.None) return tileError;
            if (image.Format == JxrPixelFormat.Gray8)
            {
                if (options.ChromaSubsampling != JxrChromaSubsampling.Yuv444)
                    return JxrError.InvalidArgument;
            {
                return JxrMinimalEncoder.EncodeGrayPixels(image.Pixels,
                    image.Stride, image.Width, image.Height, options, trace, out jxr);
            }
            }
            if (image.Format == JxrPixelFormat.Rgb24 ||
                image.Format == JxrPixelFormat.Bgr24)
            {
                if (trace != null) return JxrError.UnsupportedFeature;
                return JxrMinimalColorEncoder.Encode(image, options, out jxr);
            }
            if (image.Format == JxrPixelFormat.Rgba32 ||
                image.Format == JxrPixelFormat.Bgra32)
            {
                if (options.AlphaMode == JxrAlphaMode.Interleaved)
                    return JxrError.UnsupportedFeature;
                if (options.AlphaMode != JxrAlphaMode.Planar)
                    return JxrError.InvalidArgument;
                if (trace != null) return JxrError.UnsupportedFeature;
                return EncodePlanarAlpha(image, options, out jxr);
            }
            return JxrError.UnsupportedFeature;
        }

        public static JxrError Decode(byte[] jxr, JxrDecoderOptions options,
            out JxrImage image)
        {
            return DecodeInternal(jxr, options, out image, null);
        }

        // Diagnostic counterpart used by the corpus runner. It records only
        // the selected macroblock and must not be used as a normal decode API.
        public static JxrError DecodeWithTrace(byte[] jxr,
            JxrDecoderOptions options, out JxrImage image,
            JxrDecoderTrace trace)
        {
            if (trace == null) { image = null; return JxrError.InvalidArgument; }
            return DecodeInternal(jxr, options, out image, trace);
        }

        private static JxrError DecodeInternal(byte[] jxr,
            JxrDecoderOptions options, out JxrImage image,
            JxrDecoderTrace trace)
        {
            image = null;
            if (jxr == null || options == null) return JxrError.InvalidArgument;
            if (options.OutputFormat != JxrPixelFormat.Gray8 &&
                options.OutputFormat != JxrPixelFormat.Rgb24 &&
                options.OutputFormat != JxrPixelFormat.Bgr24 &&
                options.OutputFormat != JxrPixelFormat.Rgba32 &&
                options.OutputFormat != JxrPixelFormat.Bgra32 &&
                options.OutputFormat != JxrPixelFormat.Pbgra32)
                return JxrError.InvalidArgument;
            if (options.AlphaMode != JxrAlphaDecodeMode.ColorOnly &&
                options.AlphaMode != JxrAlphaDecodeMode.AlphaOnly &&
                options.AlphaMode != JxrAlphaDecodeMode.ColorAndAlpha)
                return JxrError.InvalidArgument;
            JxrHeaders headers;
            JxrError headerError = JxrHeaders.Read(jxr, out headers);
            if (headerError != JxrError.None) return headerError;
            if (headers.HasPlanarAlpha)
            {
                if (options.AlphaMode == JxrAlphaDecodeMode.AlphaOnly)
                {
                    if (options.OutputFormat != JxrPixelFormat.Gray8)
                        return JxrError.InvalidArgument;
                    return DecodePlanarAlpha(jxr, headers, out image);
                }
                if (options.AlphaMode == JxrAlphaDecodeMode.ColorAndAlpha)
                {
                    if (options.OutputFormat != JxrPixelFormat.Rgba32 &&
                        options.OutputFormat != JxrPixelFormat.Bgra32 &&
                        options.OutputFormat != JxrPixelFormat.Pbgra32)
                        return JxrError.InvalidArgument;
                    return DecodePlanarRgba(jxr, headers,
                        options.OutputFormat, out image);
                }
                if (options.OutputFormat == JxrPixelFormat.Rgba32 ||
                    options.OutputFormat == JxrPixelFormat.Bgra32 ||
                    options.OutputFormat == JxrPixelFormat.Pbgra32)
                    return JxrError.InvalidArgument;
            }
            else if (options.AlphaMode == JxrAlphaDecodeMode.AlphaOnly ||
                options.OutputFormat == JxrPixelFormat.Rgba32 ||
                options.OutputFormat == JxrPixelFormat.Bgra32 ||
                options.OutputFormat == JxrPixelFormat.Pbgra32)
                return JxrError.UnsupportedFeature;
            if (options.OutputFormat == JxrPixelFormat.Rgb24 ||
                options.OutputFormat == JxrPixelFormat.Bgr24)
            {
                if (trace != null && (headers.Main.HasAlpha ||
                    headers.Main.BitstreamFormat != 1))
                    return JxrError.UnsupportedFeature;
                byte[] rgb;
                int rgbWidth, rgbHeight;
                JxrError rgbError = JxrMinimalDecoder.DecodeRgbPixels(jxr,
                    options.OutputFormat == JxrPixelFormat.Rgb24,
                    out rgb, out rgbWidth, out rgbHeight, trace);
                if (rgbError != JxrError.None) return rgbError;
                image = new JxrImage(rgbWidth, rgbHeight,
                    options.OutputFormat, rgb, rgbWidth * 3);
                return JxrError.None;
            }
            if (trace != null) return JxrError.UnsupportedFeature;
            byte[] pixels;
            int width, height;
            JxrError error = JxrMinimalDecoder.DecodeGrayPixels(jxr, out pixels,
                out width, out height);
            if (error != JxrError.None) return error;
            image = new JxrImage(width, height, JxrPixelFormat.Gray8, pixels, width);
            return JxrError.None;
        }

        private static JxrError EncodePlanarAlpha(JxrImage image,
            JxrEncoderOptions options, out byte[] jxr)
        {
            jxr = null;
            if ((long)image.Width * image.Height > Int32.MaxValue / 4)
                return JxrError.UnsupportedFeature;
            bool rgba = image.Format == JxrPixelFormat.Rgba32;
            byte[] colorPixels = new byte[image.Width * image.Height * 3];
            byte[] alphaPixels = new byte[image.Width * image.Height];
            for (int y = 0; y < image.Height; y++)
                for (int x = 0; x < image.Width; x++)
                {
                    int source = y * image.Stride + x * 4;
                    int color = (y * image.Width + x) * 3;
                    int alpha = y * image.Width + x;
                    if (rgba)
                    {
                        colorPixels[color] = image.Pixels[source];
                        colorPixels[color + 1] = image.Pixels[source + 1];
                        colorPixels[color + 2] = image.Pixels[source + 2];
                    }
                    else
                    {
                        colorPixels[color] = image.Pixels[source + 2];
                        colorPixels[color + 1] = image.Pixels[source + 1];
                        colorPixels[color + 2] = image.Pixels[source];
                    }
                    alphaPixels[alpha] = image.Pixels[source + 3];
                }
            JxrImage colorImage = new JxrImage(image.Width, image.Height,
                JxrPixelFormat.Rgb24, colorPixels, image.Width * 3);
            byte[] colorContainer;
            JxrError error = JxrMinimalColorEncoder.Encode(colorImage, options,
                out colorContainer);
            if (error != JxrError.None) return error;
            JxrEncoderOptions alphaOptions = CopyOptions(options);
            alphaOptions.QualityIndex = options.AlphaQualityIndex;
            alphaOptions.ChromaSubsampling = JxrChromaSubsampling.Yuv444;
            alphaOptions.DcQuantizerIndex = -1;
            alphaOptions.LowpassQuantizerIndex = -1;
            alphaOptions.HighpassQuantizerIndex = -1;
            byte[] alphaContainer;
            error = JxrMinimalEncoder.EncodeGrayPixels(alphaPixels, image.Width,
                image.Width, image.Height, alphaOptions, null, out alphaContainer);
            if (error != JxrError.None) return error;
            JxrHeaders colorHeaders, alphaHeaders;
            error = JxrHeaders.Read(colorContainer, out colorHeaders);
            if (error != JxrError.None) return error;
            error = JxrHeaders.Read(alphaContainer, out alphaHeaders);
            if (error != JxrError.None) return error;
            byte[] colorStream = Slice(colorContainer, colorHeaders.CodestreamOffset,
                colorHeaders.CodestreamLength);
            byte[] alphaStream = Slice(alphaContainer, alphaHeaders.CodestreamOffset,
                alphaHeaders.CodestreamLength);
            return JxrContainerWriter.WriteRgbaPlanar(colorStream, alphaStream,
                image.Width, image.Height, !rgba, 96.012f, 96.012f, out jxr);
        }

        private static JxrEncoderOptions CopyOptions(JxrEncoderOptions source)
        {
            JxrEncoderOptions result = new JxrEncoderOptions();
            result.QualityIndex = source.QualityIndex;
            result.Overlap = source.Overlap;
            result.Layout = source.Layout;
            result.Progressive = source.Progressive;
            result.DcQuantizerIndex = source.DcQuantizerIndex;
            result.LowpassQuantizerIndex = source.LowpassQuantizerIndex;
            result.HighpassQuantizerIndex = source.HighpassQuantizerIndex;
            result.TrimFlexbits = source.TrimFlexbits;
            result.Subbands = source.Subbands;
            result.ChromaSubsampling = source.ChromaSubsampling;
            result.TileLayout = source.TileLayout;
            result.AlphaQualityIndex = source.AlphaQualityIndex;
            result.AlphaMode = source.AlphaMode;
            return result;
        }

        private static byte[] Slice(byte[] source, int offset, int length)
        {
            byte[] result = new byte[length];
            Array.Copy(source, offset, result, 0, length);
            return result;
        }

        private static JxrError DecodePlanarAlpha(byte[] source,
            JxrHeaders headers, out JxrImage image)
        {
            image = null;
            byte[] alphaStream = Slice(source, headers.AlphaOffset,
                headers.AlphaByteCount);
            byte[] pixels;
            int width, height;
            JxrError error = JxrMinimalDecoder.DecodeGrayPixels(alphaStream,
                out pixels, out width, out height);
            if (error != JxrError.None) return error;
            if (width != headers.Main.Width || height != headers.Main.Height)
                return JxrError.InvalidBitstream;
            image = new JxrImage(width, height, JxrPixelFormat.Gray8, pixels, width);
            return JxrError.None;
        }

        private static JxrError DecodePlanarRgba(byte[] source,
            JxrHeaders headers, JxrPixelFormat outputFormat,
            out JxrImage image)
        {
            image = null;
            bool outputRgba = outputFormat == JxrPixelFormat.Rgba32;
            bool outputPremultiplied = outputFormat == JxrPixelFormat.Pbgra32;
            bool sourcePremultiplied = String.Equals(headers.PixelFormatGuid,
                "6fddc324-4e03-4bfe-b185-3d77768dc910",
                StringComparison.OrdinalIgnoreCase);
            byte[] colorPixels, alphaPixels;
            int width, height, alphaWidth, alphaHeight;
            JxrError error = JxrMinimalDecoder.DecodeRgbPixels(source,
                true, out colorPixels, out width, out height);
            if (error != JxrError.None) return error;
            JxrImage alphaImage;
            error = DecodePlanarAlpha(source, headers, out alphaImage);
            if (error != JxrError.None) return error;
            alphaPixels = alphaImage.Pixels;
            alphaWidth = alphaImage.Width;
            alphaHeight = alphaImage.Height;
            if (width != alphaWidth || height != alphaHeight)
                return JxrError.InvalidBitstream;
            byte[] pixels = new byte[width * height * 4];
            for (int index = 0; index < width * height; index++)
            {
                int sourceColor = index * 3, target = index * 4;
                byte red = colorPixels[sourceColor];
                byte green = colorPixels[sourceColor + 1];
                byte blue = colorPixels[sourceColor + 2];
                byte alpha = alphaPixels[index];
                if (outputPremultiplied && !sourcePremultiplied)
                {
                    red = Premultiply(red, alpha);
                    green = Premultiply(green, alpha);
                    blue = Premultiply(blue, alpha);
                }
                else if (!outputPremultiplied && sourcePremultiplied)
                {
                    red = Unpremultiply(red, alpha);
                    green = Unpremultiply(green, alpha);
                    blue = Unpremultiply(blue, alpha);
                }
                if (outputRgba)
                {
                    pixels[target] = red;
                    pixels[target + 1] = green;
                    pixels[target + 2] = blue;
                }
                else
                {
                    pixels[target] = blue;
                    pixels[target + 1] = green;
                    pixels[target + 2] = red;
                }
                pixels[target + 3] = alpha;
            }
            image = new JxrImage(width, height, outputFormat, pixels, width * 4);
            return JxrError.None;
        }

        private static byte Premultiply(byte component, byte alpha)
        {
            return (byte)((component * alpha + 127) / 255);
        }

        private static byte Unpremultiply(byte component, byte alpha)
        {
            if (alpha == 0) return 0;
            int value = (component * 255 + alpha / 2) / alpha;
            return (byte)(value > 255 ? 255 : value);
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

        public static JxrError Encode(JxrImage image,
            JxrSourceProfile sourceProfile, Stream destination)
        {
            if (destination == null) return JxrError.InvalidArgument;
            try
            {
                if (!destination.CanWrite) return JxrError.InvalidArgument;
                byte[] jxr;
                JxrError error = Encode(image, sourceProfile, out jxr);
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
