using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    public sealed class JxrGrayMacroblockTrace
    {
        private readonly int x, y, dcStart, dcEnd, lpStart, lpEnd, hpStart, hpEnd;
        private readonly int[] transformed, quantized, predicted;
        internal JxrGrayMacroblockTrace(int x, int y, int[] transformed,
            int[] quantized, int[] predicted, int dcStart, int dcEnd,
            int lpStart, int lpEnd, int hpStart, int hpEnd)
        {
            this.x = x; this.y = y;
            this.transformed = (int[])transformed.Clone();
            this.quantized = (int[])quantized.Clone();
            this.predicted = (int[])predicted.Clone();
            this.dcStart = dcStart; this.dcEnd = dcEnd;
            this.lpStart = lpStart; this.lpEnd = lpEnd;
            this.hpStart = hpStart; this.hpEnd = hpEnd;
        }
        public int MacroblockX { get { return x; } }
        public int MacroblockY { get { return y; } }
        public int[] TransformCoefficients { get { return (int[])transformed.Clone(); } }
        public int[] QuantizedCoefficients { get { return (int[])quantized.Clone(); } }
        public int[] PredictedCoefficients { get { return (int[])predicted.Clone(); } }
        public int DcBitStart { get { return dcStart; } }
        public int DcBitEnd { get { return dcEnd; } }
        public int LpBitStart { get { return lpStart; } }
        public int LpBitEnd { get { return lpEnd; } }
        public int HpBitStart { get { return hpStart; } }
        public int HpBitEnd { get { return hpEnd; } }
    }

    public sealed class JxrGrayEncodingTrace
    {
        private readonly List<JxrGrayMacroblockTrace> macroblocks =
            new List<JxrGrayMacroblockTrace>();
        public int MacroblockCount { get { return macroblocks.Count; } }
        public JxrGrayMacroblockTrace GetMacroblock(int index)
        { return macroblocks[index]; }
        internal void Add(JxrGrayMacroblockTrace value) { macroblocks.Add(value); }
        internal void Clear() { macroblocks.Clear(); }
    }

    public enum JxrBitstreamLayout
    {
        Spatial = 0,
        Frequency = 1
    }

    public enum JxrGraySubbandMode
    {
        All = 0,
        NoFlexbits = 1,
        NoHighpass = 2,
        DcOnly = 3
    }

    public enum JxrChromaSubsampling
    {
        Yuv444 = 3,
        Yuv422 = 2,
        Yuv420 = 1
    }

    public sealed class JxrEncoderOptions
    {
        private int qualityIndex = 1;
        private int overlap = 0;
        private JxrBitstreamLayout layout = JxrBitstreamLayout.Spatial;
        private int dcQuantizerIndex = -1;
        private int lowpassQuantizerIndex = -1;
        private int highpassQuantizerIndex = -1;
        private int trimFlexbits;
        private JxrGraySubbandMode subbands = JxrGraySubbandMode.All;
        private JxrChromaSubsampling chromaSubsampling = JxrChromaSubsampling.Yuv444;

        public int QualityIndex { get { return qualityIndex; } set { qualityIndex = value; } }
        public int Overlap { get { return overlap; } set { overlap = value; } }
        public JxrBitstreamLayout Layout { get { return layout; } set { layout = value; } }
        // -1 inherits QualityIndex. Native QP indexes 0 and 1 both mean lossless.
        public int DcQuantizerIndex { get { return dcQuantizerIndex; } set { dcQuantizerIndex = value; } }
        public int LowpassQuantizerIndex { get { return lowpassQuantizerIndex; } set { lowpassQuantizerIndex = value; } }
        public int HighpassQuantizerIndex { get { return highpassQuantizerIndex; } set { highpassQuantizerIndex = value; } }
        public int TrimFlexbits { get { return trimFlexbits; } set { trimFlexbits = value; } }
        public JxrGraySubbandMode Subbands { get { return subbands; } set { subbands = value; } }
        public JxrChromaSubsampling ChromaSubsampling
        { get { return chromaSubsampling; } set { chromaSubsampling = value; } }
    }

    public sealed class JxrDecoderOptions
    {
        private JxrPixelFormat outputFormat = JxrPixelFormat.Gray8;

        public JxrPixelFormat OutputFormat
        {
            get { return outputFormat; }
            set { outputFormat = value; }
        }
    }
}
