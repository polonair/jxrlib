using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    // Tile sizes are expressed in macroblocks. The final tile in each axis
    // ends at the coded image boundary and may therefore contain a partial
    // pixel macroblock at the bottom or right edge.
    public sealed class JxrTileLayout
    {
        private readonly int[] columnWidths;
        private readonly int[] rowHeights;

        public JxrTileLayout(int[] columnWidthsInMacroblocks,
            int[] rowHeightsInMacroblocks)
        {
            if (columnWidthsInMacroblocks == null)
                throw new System.ArgumentNullException("columnWidthsInMacroblocks");
            if (rowHeightsInMacroblocks == null)
                throw new System.ArgumentNullException("rowHeightsInMacroblocks");
            columnWidths = (int[])columnWidthsInMacroblocks.Clone();
            rowHeights = (int[])rowHeightsInMacroblocks.Clone();
        }

        public int[] TileColumnWidthsInMacroblocks
        { get { return (int[])columnWidths.Clone(); } }
        public int[] TileRowHeightsInMacroblocks
        { get { return (int[])rowHeights.Clone(); } }

        internal bool GetBoundaries(int width, int height, out int[] x,
            out int[] y)
        {
            x = null;
            y = null;
            int columns = (int)(((long)width + 15) / 16);
            int rows = (int)(((long)height + 15) / 16);
            if (columnWidths.Length < 1 || rowHeights.Length < 1 ||
                columnWidths.Length > 4096 || rowHeights.Length > 4096 ||
                columnWidths.Length > columns || rowHeights.Length > rows)
                return false;
            x = new int[columnWidths.Length + 1];
            y = new int[rowHeights.Length + 1];
            for (int index = 0; index < columnWidths.Length; index++)
            {
                int size = columnWidths[index];
                if (size < 1 || (long)x[index] + size > columns ||
                    size > 65535) return false;
                x[index + 1] = x[index] + size;
            }
            for (int index = 0; index < rowHeights.Length; index++)
            {
                int size = rowHeights[index];
                if (size < 1 || (long)y[index] + size > rows ||
                    size > 65535) return false;
                y[index + 1] = y[index] + size;
            }
            return x[columnWidths.Length] == columns &&
                y[rowHeights.Length] == rows;
        }

        internal bool IsSingleTile
        { get { return columnWidths.Length == 1 && rowHeights.Length == 1; } }
    }

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

    public enum JxrAlphaMode
    {
        None = 0,
        Planar = 2,
        Interleaved = 3
    }

    public enum JxrAlphaDecodeMode
    {
        ColorOnly = 0,
        AlphaOnly = 1,
        ColorAndAlpha = 2
    }

    public sealed class JxrEncoderOptions
    {
        private int qualityIndex = 1;
        private int overlap = 0;
        private JxrBitstreamLayout layout = JxrBitstreamLayout.Spatial;
        private bool progressive = true;
        private int dcQuantizerIndex = -1;
        private int lowpassQuantizerIndex = -1;
        private int highpassQuantizerIndex = -1;
        private int trimFlexbits;
        private JxrGraySubbandMode subbands = JxrGraySubbandMode.All;
        private JxrChromaSubsampling chromaSubsampling = JxrChromaSubsampling.Yuv444;
        private JxrTileLayout tileLayout;
        private int alphaQualityIndex = 1;
        private JxrAlphaMode alphaMode = JxrAlphaMode.Planar;

        public int QualityIndex { get { return qualityIndex; } set { qualityIndex = value; } }
        public int Overlap { get { return overlap; } set { overlap = value; } }
        public JxrBitstreamLayout Layout { get { return layout; } set { layout = value; } }
        // Frequency streams default to progressive packet ordering; false
        // writes packets tile-by-tile (sequential ordering).
        public bool Progressive { get { return progressive; } set { progressive = value; } }
        // -1 inherits QualityIndex. Native QP indexes 0 and 1 both mean lossless.
        public int DcQuantizerIndex { get { return dcQuantizerIndex; } set { dcQuantizerIndex = value; } }
        public int LowpassQuantizerIndex { get { return lowpassQuantizerIndex; } set { lowpassQuantizerIndex = value; } }
        public int HighpassQuantizerIndex { get { return highpassQuantizerIndex; } set { highpassQuantizerIndex = value; } }
        public int TrimFlexbits { get { return trimFlexbits; } set { trimFlexbits = value; } }
        public JxrGraySubbandMode Subbands { get { return subbands; } set { subbands = value; } }
        public JxrChromaSubsampling ChromaSubsampling
        { get { return chromaSubsampling; } set { chromaSubsampling = value; } }
        public JxrTileLayout TileLayout
        { get { return tileLayout; } set { tileLayout = value; } }
        // Used only by 32-bit RGBA/BGRA input. Native alpha QP is independent
        // from the color QP; 0 and 1 both select lossless coding.
        public int AlphaQualityIndex
        { get { return alphaQualityIndex; } set { alphaQualityIndex = value; } }
        public JxrAlphaMode AlphaMode
        { get { return alphaMode; } set { alphaMode = value; } }
    }

    public sealed class JxrDecoderOptions
    {
        private JxrPixelFormat outputFormat = JxrPixelFormat.Gray8;
        private JxrAlphaDecodeMode alphaMode = JxrAlphaDecodeMode.ColorAndAlpha;

        public JxrPixelFormat OutputFormat
        {
            get { return outputFormat; }
            set { outputFormat = value; }
        }
        public JxrAlphaDecodeMode AlphaMode
        { get { return alphaMode; } set { alphaMode = value; } }
    }
}
