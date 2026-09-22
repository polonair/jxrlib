using System;

namespace Jxr.Managed.Core
{
    // State, code-table selection and decoder-table selection are explicit.
    public sealed class JxrAdaptiveHuffman
    {
        private static readonly int[] maximumTableCounts = { 0, 0, 0, 0, 1, 2, 4, 2, 2, 2, 0, 0, 5 };
        private static readonly int[] secondDiscriminants = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 };
        private const int Threshold = 8;
        private const int Memory = 8;
        private readonly int symbolCount;
        private int[] delta;
        private int[] delta1;
        private int[] codeTable;
        private JxrHuffmanTable decoderTable;
        private bool isInitialized;
        private int discriminant;
        private int secondaryDiscriminant;
        private int tableIndex;
        private int upperBound;
        private int lowerBound;

        public JxrAdaptiveHuffman(int symbols, int[] primaryDelta, int[] secondaryDelta)
        {
            if (symbols <= 0 || symbols > 255) throw new ArgumentOutOfRangeException("symbols");
            if (primaryDelta != null && primaryDelta.Length < symbols)
                throw new ArgumentException("Primary delta does not cover the alphabet.", "primaryDelta");
            if (secondaryDelta != null && secondaryDelta.Length < symbols)
                throw new ArgumentException("Secondary delta does not cover the alphabet.", "secondaryDelta");
            symbolCount = symbols;
            delta = primaryDelta;
            delta1 = secondaryDelta;
        }

        public bool IsInitialized { get { return isInitialized; } }
        public int Discriminant { get { return discriminant; } set { discriminant = value; } }
        public int SecondaryDiscriminant { get { return secondaryDiscriminant; } set { secondaryDiscriminant = value; } }
        public int TableIndex { get { return tableIndex; } }
        public int UpperBound { get { return upperBound; } }
        public int LowerBound { get { return lowerBound; } }
        public JxrHuffmanTable DecoderTable { get { return decoderTable; } }

        public JxrError ObserveSymbol(int symbol)
        {
            if (delta == null || symbol < 0 || symbol >= symbolCount) return JxrError.InvalidArgument;
            Discriminant += delta[symbol];
            if (delta1 != null)
            {
                if (symbol >= delta1.Length) return JxrError.InvalidArgument;
                SecondaryDiscriminant += delta1[symbol];
            }
            return JxrError.None;
        }

        public JxrError DecodeSymbol(JxrHuffmanTable table, JxrBitReader reader,
            out int symbol)
        {
            JxrError error = JxrHuffmanDecoder.DecodeSymbol(table, reader, out symbol);
            if (error != JxrError.None) return error;
            return ObserveSymbol(symbol);
        }

        public JxrError DecodeSymbol(JxrBitReader reader, out int symbol)
        {
            if (decoderTable == null)
            {
                symbol = 0;
                return JxrError.InvalidArgument;
            }
            return DecodeSymbol(decoderTable, reader, out symbol);
        }

        public JxrError GetCodeWord(int symbol, out uint code, out int bitCount)
        {
            code = 0;
            bitCount = 0;
            if (codeTable == null || symbol < 0 || symbol >= symbolCount)
                return JxrError.InvalidArgument;
            code = (uint)codeTable[1 + symbol * 2];
            bitCount = codeTable[2 + symbol * 2];
            return JxrError.None;
        }

        public JxrError Adapt()
        {
            int maximumTables;
            bool hasSecondDiscriminant;
            bool changed = false;
            int high;

            if (symbolCount >= maximumTableCounts.Length ||
                maximumTableCounts[symbolCount] == 0)
                return JxrError.UnsupportedFeature;
            maximumTables = maximumTableCounts[symbolCount];
            hasSecondDiscriminant = secondDiscriminants[symbolCount] != 0;
            if (!IsInitialized)
            {
                isInitialized = true;
                Discriminant = 0;
                SecondaryDiscriminant = 0;
                tableIndex = secondDiscriminants[symbolCount];
            }

            high = hasSecondDiscriminant ? SecondaryDiscriminant : Discriminant;
            if (Discriminant < LowerBound)
            {
                tableIndex--;
                changed = true;
            }
            else if (high > UpperBound)
            {
                tableIndex++;
                changed = true;
            }
            if (tableIndex < 0 || tableIndex >= maximumTables) return JxrError.InvalidBitstream;
            if (changed)
            {
                Discriminant = 0;
                SecondaryDiscriminant = 0;
            }
            Discriminant = Clamp(Discriminant, -Threshold * Memory, Threshold * Memory);
            SecondaryDiscriminant = Clamp(SecondaryDiscriminant, -Threshold * Memory, Threshold * Memory);
            lowerBound = tableIndex == 0 ? Int32.MinValue : -Threshold;
            upperBound = tableIndex == maximumTables - 1 ? (1 << 30) : Threshold;
            if (!JxrAdaptiveHuffmanTableCatalog.TryGet(symbolCount, tableIndex,
                out codeTable, out delta, out delta1, out decoderTable))
                return JxrError.UnsupportedFeature;
            return JxrError.None;
        }

        private static int Clamp(int value, int minimum, int maximum)
        {
            if (value < minimum) return minimum;
            if (value > maximum) return maximum;
            return value;
        }
    }
}
