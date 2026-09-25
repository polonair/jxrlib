using System;

namespace Jxr.Managed.Core
{
    // Logical configuration shared by the managed decoder and encoder.
    // ColorFormat uses the native Y_ONLY/YUV_420/YUV_422/YUV_444/CMYK/
    // NCOMPONENT numbering. ChannelBytes is 2 or 4 (native BD_16/BD_32
    // internal coefficient storage); managed samples remain Int32.
    public sealed class JxrSessionConfiguration
    {
        private readonly int width, height, colorFormat, channelCount, channelBytes;
        private readonly bool alpha;

        public JxrSessionConfiguration(int width, int height, int colorFormat,
            int channelCount, int channelBytes, bool alpha)
        {
            if (width <= 0 || height <= 0 || colorFormat < 0 ||
                colorFormat > 6 || colorFormat == 5 ||
                channelCount < 1 || channelCount > 15 ||
                (channelBytes != 2 && channelBytes != 4))
                throw new ArgumentException("Invalid JPEG XR session configuration.");
            this.width = width;
            this.height = height;
            this.colorFormat = colorFormat;
            this.channelCount = channelCount;
            this.channelBytes = channelBytes;
            this.alpha = alpha;
        }

        public int Width { get { return width; } }
        public int Height { get { return height; } }
        public int ColorFormat { get { return colorFormat; } }
        public int ChannelCount { get { return channelCount; } }
        public int ChannelBytes { get { return channelBytes; } }
        public bool HasAlpha { get { return alpha; } }
    }

    // Mirrors JxrDecoderMemoryLayoutPlan and JxrEncoderMemoryLayoutPlan sizes.
    // The prefix is a reference-C accounting value, not a managed allocation.
    public sealed class JxrSessionMemoryPlan
    {
        internal bool allocationIsSafe;
        internal long macroblocks, channelBytes, chromaBlocks, fullBlockBytes;
        internal long chromaBlockBytes, primaryRowBytes, primaryBufferBytes;
        internal long primaryAllocationBytes, secondaryBufferBytes;
        internal long secondaryAllocationBytes;
        public bool AllocationIsSafe { get { return allocationIsSafe; } }
        public long MacroblockCount { get { return macroblocks; } }
        public long ChannelBytes { get { return channelBytes; } }
        public long ChromaBlockCount { get { return chromaBlocks; } }
        public long FullResolutionMacroblockBytes { get { return fullBlockBytes; } }
        public long ChromaMacroblockBytes { get { return chromaBlockBytes; } }
        public long PrimaryMacroblockRowBytes { get { return primaryRowBytes; } }
        public long PrimaryMacroblockBufferBytes { get { return primaryBufferBytes; } }
        public long PrimaryAllocationBytes { get { return primaryAllocationBytes; } }
        public long SecondaryMacroblockBufferBytes { get { return secondaryBufferBytes; } }
        public long SecondaryAllocationBytes { get { return secondaryAllocationBytes; } }
    }

    public static class JxrSessionPlanner
    {
        private static readonly int[] ChromaBlocks = { 0, 4, 8, 16, 16, 0, 16 };
        private const int PacketLength = 4096;

        public static JxrError FromHeaders(JxrHeaders headers,
            out JxrSessionConfiguration configuration)
        {
            configuration = null;
            if (headers == null) return JxrError.InvalidArgument;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            long paddedWidth = main.Width + main.ExtraLeft + main.ExtraRight;
            long paddedHeight = main.Height + main.ExtraTop + main.ExtraBottom;
            if (paddedWidth <= 0 || paddedHeight <= 0 ||
                paddedWidth > Int32.MaxValue || paddedHeight > Int32.MaxValue)
                return JxrError.UnsupportedFeature;
            configuration = new JxrSessionConfiguration((int)paddedWidth,
                (int)paddedHeight, plane.ColorFormat, plane.ChannelCount,
                main.CodedBitDepth == 0 ? 2 : 4, main.HasAlpha);
            return JxrError.None;
        }

        public static JxrSessionMemoryPlan Decoder(JxrSessionConfiguration config,
            int codecStateBytes, int decoderParametersBytes, int bitIoStateBytes,
            bool thirtyTwoBitBuild)
        {
            Check(config, codecStateBytes, decoderParametersBytes, bitIoStateBytes);
            JxrSessionMemoryPlan plan = BasePlan(config);
            long twoRows = plan.primaryRowBytes * 2;
            plan.allocationIsSafe = !thirtyTwoBitBuild ||
                ((twoRows * (plan.macroblocks >> 16)) & 0xffffc000L) == 0;
            plan.primaryBufferBytes = twoRows * plan.macroblocks;
            plan.primaryAllocationBytes = codecStateBytes + 127L +
                decoderParametersBytes + (PacketLength * 4 - 1L) +
                PacketLength * 2L + bitIoStateBytes + plan.primaryBufferBytes;
            return plan;
        }

        public static JxrSessionMemoryPlan Encoder(JxrSessionConfiguration config,
            int codecStateBytes, int bitIoStateBytes, bool thirtyTwoBitBuild)
        {
            Check(config, codecStateBytes, 0, bitIoStateBytes);
            JxrSessionMemoryPlan plan = BasePlan(config);
            plan.allocationIsSafe = !thirtyTwoBitBuild ||
                (((plan.macroblocks >> 15) * plan.primaryRowBytes) &
                0xffff0000L) == 0;
            plan.primaryBufferBytes = plan.primaryRowBytes * plan.macroblocks * 2;
            plan.secondaryBufferBytes = plan.fullBlockBytes * plan.macroblocks * 2;
            plan.primaryAllocationBytes = codecStateBytes + 127L +
                (PacketLength * 4 - 1L) + PacketLength * 2L +
                bitIoStateBytes + plan.primaryBufferBytes;
            plan.secondaryAllocationBytes = codecStateBytes + 127L +
                plan.secondaryBufferBytes;
            return plan;
        }

        private static void Check(JxrSessionConfiguration config, int codecStateBytes,
            int decoderParametersBytes, int bitIoStateBytes)
        {
            if (config == null || codecStateBytes < 0 || decoderParametersBytes < 0 ||
                bitIoStateBytes < 0) throw new ArgumentException("Invalid memory plan.");
        }

        private static JxrSessionMemoryPlan BasePlan(JxrSessionConfiguration config)
        {
            JxrSessionMemoryPlan plan = new JxrSessionMemoryPlan();
            plan.macroblocks = ((long)config.Width + 15) / 16;
            plan.channelBytes = config.ChannelBytes;
            plan.chromaBlocks = ChromaBlocks[config.ColorFormat];
            plan.fullBlockBytes = plan.channelBytes * 16 * 16;
            plan.chromaBlockBytes = plan.channelBytes * 16 * plan.chromaBlocks;
            plan.primaryRowBytes = plan.fullBlockBytes +
                plan.chromaBlockBytes * (config.ChannelCount - 1);
            return plan;
        }
    }

    public abstract class JxrSession : IDisposable
    {
        private readonly JxrSessionConfiguration configuration;
        private readonly JxrSessionMemoryPlan plan;
        private int[][] primaryRows, alphaRows;
        private bool closed;

        protected JxrSession(JxrSessionConfiguration configuration,
            JxrSessionMemoryPlan plan)
        {
            if (!plan.AllocationIsSafe) throw new ArgumentException("Unsafe memory plan.");
            this.configuration = configuration;
            this.plan = plan;
            long fullSamples = plan.MacroblockCount * 256;
            long chromaSamples = plan.MacroblockCount *
                plan.ChromaBlockCount * 16;
            if (fullSamples > Int32.MaxValue || chromaSamples > Int32.MaxValue)
                throw new ArgumentException("Image row is too large for managed arrays.");
            primaryRows = new int[configuration.ChannelCount * 2][];
            for (int row = 0; row < 2; row++)
                for (int channel = 0; channel < configuration.ChannelCount; channel++)
                    primaryRows[row * configuration.ChannelCount + channel] =
                        new int[channel == 0 ? (int)fullSamples : (int)chromaSamples];
            if (configuration.HasAlpha)
            {
                alphaRows = new int[2][];
                alphaRows[0] = new int[(int)fullSamples];
                alphaRows[1] = new int[(int)fullSamples];
            }
        }

        public JxrSessionConfiguration Configuration { get { return configuration; } }
        public JxrSessionMemoryPlan MemoryPlan { get { return plan; } }
        public bool IsClosed { get { return closed; } }

        public int[] GetPrimaryRow(int row, int channel)
        {
            if (closed) throw new ObjectDisposedException("JxrSession");
            if (row < 0 || row > 1 || channel < 0 ||
                channel >= configuration.ChannelCount)
                throw new ArgumentOutOfRangeException();
            return primaryRows[row * configuration.ChannelCount + channel];
        }

        public int[] GetAlphaRow(int row)
        {
            if (closed) throw new ObjectDisposedException("JxrSession");
            if (alphaRows == null || row < 0 || row > 1)
                throw new ArgumentOutOfRangeException();
            return alphaRows[row];
        }

        public void Dispose()
        {
            if (closed) return;
            primaryRows = null;
            alphaRows = null;
            closed = true;
        }
    }

    public sealed class JxrDecoderSession : JxrSession
    {
        private JxrDecoderSession(JxrSessionConfiguration config,
            JxrSessionMemoryPlan plan) : base(config, plan) { }

        public static JxrDecoderSession Create(JxrSessionConfiguration config,
            int codecStateBytes, int decoderParametersBytes, int bitIoStateBytes)
        {
            JxrSessionMemoryPlan plan = JxrSessionPlanner.Decoder(config,
                codecStateBytes, decoderParametersBytes, bitIoStateBytes, false);
            return new JxrDecoderSession(config, plan);
        }
    }

    public sealed class JxrEncoderSession : JxrSession
    {
        private JxrEncoderSession(JxrSessionConfiguration config,
            JxrSessionMemoryPlan plan) : base(config, plan) { }

        public static JxrEncoderSession Create(JxrSessionConfiguration config,
            int codecStateBytes, int bitIoStateBytes)
        {
            JxrSessionMemoryPlan plan = JxrSessionPlanner.Encoder(config,
                codecStateBytes, bitIoStateBytes, false);
            return new JxrEncoderSession(config, plan);
        }
    }
}
