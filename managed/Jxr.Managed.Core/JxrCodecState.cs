using System;

namespace Jxr.Managed.Core
{
    public enum JxrCodecColorFormat
    {
        YOnly = 0,
        Yuv420 = 1,
        Yuv422 = 2,
        Yuv444 = 3,
        Cmyk = 4,
        NComponent = 6
    }

    // Immutable syntax and tile choices supplied by header parsing.
    public sealed class JxrCodecConfiguration
    {
        private readonly JxrCodecColorFormat colorFormat;
        private readonly int channelCount;
        private readonly bool spatial;
        private readonly bool dcOnly;
        private readonly bool highpass;
        private readonly bool flexbits;
        private readonly bool skipFlexbits;
        private readonly bool resetScan;
        private readonly bool resetContext;
        private readonly bool adaptDcHuffman;
        private readonly bool transcode;
        private readonly int lowpassQuantizerBits;
        private readonly int highpassQuantizerBits;
        private readonly int lowpassQuantizerCount;
        private readonly int highpassQuantizerCount;
        private readonly int[][] highpassQuantizers;

        public JxrCodecConfiguration(JxrCodecColorFormat colorFormat, int channelCount,
            bool spatial, bool dcOnly, bool highpass, bool flexbits,
            bool skipFlexbits, bool resetScan, bool resetContext,
            bool adaptDcHuffman, bool transcode, int lowpassQuantizerBits,
            int highpassQuantizerBits, int lowpassQuantizerCount,
            int highpassQuantizerCount, int[][] highpassQuantizers)
        {
            if (channelCount < 1 || channelCount > 16 ||
                lowpassQuantizerBits < 0 || lowpassQuantizerBits > 8 ||
                highpassQuantizerBits < 0 || highpassQuantizerBits > 8 ||
                lowpassQuantizerCount < 1 || highpassQuantizerCount < 1)
                throw new ArgumentException("Invalid JPEG XR subband configuration.");
            this.colorFormat = colorFormat;
            this.channelCount = channelCount;
            this.spatial = spatial;
            this.dcOnly = dcOnly;
            this.highpass = highpass;
            this.flexbits = flexbits;
            this.skipFlexbits = skipFlexbits;
            this.resetScan = resetScan;
            this.resetContext = resetContext;
            this.adaptDcHuffman = adaptDcHuffman;
            this.transcode = transcode;
            this.lowpassQuantizerBits = lowpassQuantizerBits;
            this.highpassQuantizerBits = highpassQuantizerBits;
            this.lowpassQuantizerCount = lowpassQuantizerCount;
            this.highpassQuantizerCount = highpassQuantizerCount;
            this.highpassQuantizers = highpassQuantizers;
        }

        public JxrCodecColorFormat ColorFormat { get { return colorFormat; } }
        public int ChannelCount { get { return channelCount; } }
        public bool Spatial { get { return spatial; } }
        public bool DcOnly { get { return dcOnly; } }
        public bool HasHighpass { get { return highpass; } }
        public bool HasFlexbits { get { return flexbits; } }
        public bool SkipFlexbits { get { return skipFlexbits; } }
        public bool ResetScan { get { return resetScan; } }
        public bool ResetContext { get { return resetContext; } }
        public bool AdaptDcHuffman { get { return adaptDcHuffman; } }
        public bool Transcode { get { return transcode; } }
        public int LowpassQuantizerBits { get { return lowpassQuantizerBits; } }
        public int HighpassQuantizerBits { get { return highpassQuantizerBits; } }
        public int LowpassQuantizerCount { get { return lowpassQuantizerCount; } }
        public int HighpassQuantizerCount { get { return highpassQuantizerCount; } }

        public JxrError GetHighpassQuantizer(int channel, int index, out int quantizer)
        {
            quantizer = 0;
            if (highpassQuantizers == null || channel < 0 ||
                channel >= highpassQuantizers.Length || highpassQuantizers[channel] == null ||
                index < 0 || index >= highpassQuantizers[channel].Length)
                return JxrError.InvalidArgument;
            quantizer = highpassQuantizers[channel][index];
            return JxrError.None;
        }
    }

    // The native 21 Huffman contexts and the two HP CBP contexts are created
    // once per coding context.  Readers can alias one object in spatial mode.
    public sealed class JxrCodecState
    {
        private static readonly int[] alphabets =
            { 5,4,8,7,7, 12,6,6,12,6,6,7,7, 12,6,6,12,6,6,7,7 };
        private readonly JxrCodecConfiguration configuration;
        private readonly JxrEntropyContext entropy;
        private readonly JxrHuffmanStateSet huffman;
        private readonly JxrHighpassCbpState highpassCbp;
        private readonly JxrMacroblockState macroblock;
        private readonly JxrMacroblockCbpState macroblockCbp;
        private readonly JxrCoefficientPlaneState coefficientPlanes;
        private readonly JxrBitReader dcReader;
        private readonly JxrBitReader lpReader;
        private readonly JxrBitReader hpReader;
        private readonly JxrBitReader flexReader;
        private readonly int[] previousTopCbp;
        private readonly int[] currentLeftCbp;
        private bool atLeftBoundary = true;
        private bool atTopBoundary = true;

        public JxrCodecState(JxrCodecConfiguration configuration, JxrBitReader dcReader,
            JxrBitReader lpReader, JxrBitReader hpReader, JxrBitReader flexReader)
        {
            int index;
            int[][] planes;
            JxrAdaptiveHuffman[] states = new JxrAdaptiveHuffman[alphabets.Length];
            if (configuration == null || dcReader == null || lpReader == null ||
                hpReader == null || flexReader == null)
                throw new ArgumentNullException("configuration/readers");
            this.configuration = configuration;
            this.dcReader = dcReader;
            this.lpReader = lpReader;
            this.hpReader = hpReader;
            this.flexReader = flexReader;
            entropy = new JxrEntropyContext();
            for (index = 0; index < states.Length; index++)
            {
                states[index] = new JxrAdaptiveHuffman(alphabets[index], null, null);
                if (states[index].Adapt() != JxrError.None)
                    throw new InvalidOperationException("Cannot initialize a JPEG XR Huffman table.");
            }
            huffman = new JxrHuffmanStateSet(states);
            JxrAdaptiveHuffman pattern = new JxrAdaptiveHuffman(
                configuration.ColorFormat == JxrCodecColorFormat.YOnly ||
                configuration.ColorFormat == JxrCodecColorFormat.Cmyk ||
                configuration.ColorFormat == JxrCodecColorFormat.NComponent ? 5 : 9, null, null);
            JxrAdaptiveHuffman count = new JxrAdaptiveHuffman(5, null, null);
            if (pattern.Adapt() != JxrError.None || count.Adapt() != JxrError.None)
                throw new InvalidOperationException("Cannot initialize JPEG XR CBP tables.");
            highpassCbp = new JxrHighpassCbpState(pattern, count, entropy.HighpassCbp);
            macroblock = new JxrMacroblockState(16);
            macroblockCbp = new JxrMacroblockCbpState(16);
            planes = new int[configuration.ChannelCount][];
            for (index = 0; index < planes.Length; index++) planes[index] = new int[256];
            JxrCoefficientColorFormat planeFormat = JxrCoefficientColorFormat.Other;
            if (configuration.ColorFormat == JxrCodecColorFormat.Yuv420) planeFormat = JxrCoefficientColorFormat.Yuv420;
            if (configuration.ColorFormat == JxrCodecColorFormat.Yuv422) planeFormat = JxrCoefficientColorFormat.Yuv422;
            if (configuration.ColorFormat == JxrCodecColorFormat.Yuv444) planeFormat = JxrCoefficientColorFormat.Yuv444;
            coefficientPlanes = new JxrCoefficientPlaneState(planes, planeFormat, planes.Length);
            previousTopCbp = new int[configuration.ChannelCount];
            currentLeftCbp = new int[configuration.ChannelCount];
        }

        public JxrCodecConfiguration Configuration { get { return configuration; } }
        public JxrEntropyContext Entropy { get { return entropy; } }
        public JxrHuffmanStateSet Huffman { get { return huffman; } }
        public JxrHighpassCbpState HighpassCbp { get { return highpassCbp; } }
        public JxrMacroblockState Macroblock { get { return macroblock; } }
        public JxrMacroblockCbpState MacroblockCbp { get { return macroblockCbp; } }
        public JxrCoefficientPlaneState CoefficientPlanes { get { return coefficientPlanes; } }
        public JxrBitReader DcReader { get { return dcReader; } }
        public JxrBitReader LpReader { get { return lpReader; } }
        public JxrBitReader HpReader { get { return hpReader; } }
        public JxrBitReader FlexReader { get { return flexReader; } }
        public bool AtLeftBoundary { get { return atLeftBoundary; } set { atLeftBoundary = value; } }
        public bool AtTopBoundary { get { return atTopBoundary; } set { atTopBoundary = value; } }

        public JxrError SetNeighborCbp(int channel, int top, int left)
        {
            if (channel < 0 || channel >= configuration.ChannelCount) return JxrError.InvalidArgument;
            previousTopCbp[channel] = top;
            currentLeftCbp[channel] = left;
            return JxrError.None;
        }

        public JxrError GetNeighborCbp(int channel, out int top, out int left)
        {
            top = 0; left = 0;
            if (channel < 0 || channel >= configuration.ChannelCount) return JxrError.InvalidArgument;
            top = previousTopCbp[channel]; left = currentLeftCbp[channel];
            return JxrError.None;
        }

        public JxrError SetCurrentCbp(int channel, int cbp)
        {
            if (channel < 0 || channel >= configuration.ChannelCount) return JxrError.InvalidArgument;
            currentLeftCbp[channel] = cbp;
            return JxrError.None;
        }
    }
}
