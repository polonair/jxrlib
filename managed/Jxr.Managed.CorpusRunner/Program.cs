using System;
using System.IO;
using Jxr.Managed.Core;

namespace Jxr.Managed.CorpusRunner
{
    internal static class Program
    {
        private static int Main(string[] args)
        {
            if (args.Length != 5 || args[0] != "decode" ||
                (args[2] != "color" && args[2] != "alpha"))
            {
                Console.Error.WriteLine(
                    "Usage: Jxr.Managed.CorpusRunner.exe decode <input.jxr> <color|alpha> <pixels.bin> <metadata.txt>");
                return 2;
            }

            try
            {
                byte[] bytes = File.ReadAllBytes(args[1]);
                JxrDecoderOptions options = new JxrDecoderOptions();
                if (args[2] == "alpha")
                {
                    options.OutputFormat = JxrPixelFormat.Gray8;
                    options.AlphaMode = JxrAlphaDecodeMode.AlphaOnly;
                }
                else
                {
                    options.OutputFormat = JxrPixelFormat.Bgr24;
                    options.AlphaMode = JxrAlphaDecodeMode.ColorOnly;
                }

                JxrImage image;
                JxrError error = JxrCodec.Decode(bytes, options, out image);
                if (error != JxrError.None)
                {
                    Console.Error.WriteLine(error.ToString());
                    return 1;
                }

                using (FileStream output = new FileStream(args[3],
                    FileMode.Create, FileAccess.Write, FileShare.None))
                    output.Write(image.Pixels, 0, image.Pixels.Length);

                using (StreamWriter metadata = new StreamWriter(args[4], false))
                {
                    metadata.WriteLine("width=" + image.Width);
                    metadata.WriteLine("height=" + image.Height);
                    metadata.WriteLine("stride=" + image.Stride);
                    metadata.WriteLine("format=" + image.Format);
                }
                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(exception.GetType().Name + ": " +
                    exception.Message);
                return 1;
            }
        }
    }
}
