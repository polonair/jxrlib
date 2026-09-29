using System;
using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    public sealed class JxrHuffmanTable
    {
        public const int RootBits = 5;
        public const int EncodedLengthBits = 3;
        public const int BranchOffset = 0x8000;
        private readonly short[] entries;

        public JxrHuffmanTable(short[] source)
        {
            if (source == null) throw new ArgumentNullException("source");
            entries = source;
        }

        public bool TryGetEntry(uint index, out int entry)
        {
            entry = 0;
            if (index >= (uint)entries.Length) return false;
            entry = entries[index];
            return true;
        }

        internal static JxrHuffmanTable CreateFromCodePairs(int[] codePairs)
        {
            int symbolCount;
            int symbol;
            HuffmanNode root = new HuffmanNode();
            List<short> result = new List<short>();
            List<HuffmanNode> branchNodes = new List<HuffmanNode>();
            List<int> branchOffsets = new List<int>();
            if (codePairs == null || codePairs.Length == 0) return null;
            symbolCount = codePairs[0];
            if (symbolCount <= 0 || codePairs.Length != 1 + symbolCount * 2) return null;
            for (symbol = 0; symbol < symbolCount; symbol++)
            {
                if (!Insert(root, codePairs[1 + symbol * 2], codePairs[2 + symbol * 2], symbol))
                    return null;
            }
            for (symbol = 0; symbol < (1 << RootBits); symbol++) result.Add(0);
            for (symbol = 0; symbol < (1 << RootBits); symbol++)
            {
                HuffmanNode node = root;
                int bitIndex;
                for (bitIndex = RootBits - 1; bitIndex >= 0; bitIndex--)
                {
                    if (node.Symbol >= 0) break;
                    node = ((symbol >> bitIndex) & 1) == 0 ? node.Zero : node.One;
                    if (node == null) return null;
                }
                if (node.Symbol >= 0)
                    result[symbol] = (short)((node.Symbol << EncodedLengthBits) | (RootBits - bitIndex - 1));
                else
                    result[symbol] = GetBranchEntry(node, result, branchNodes, branchOffsets);
            }
            return new JxrHuffmanTable(result.ToArray());
        }

        private static bool Insert(HuffmanNode root, int code, int length, int symbol)
        {
            HuffmanNode node = root;
            int bitIndex;
            if (length <= 0 || length > 31 || code < 0 || code >= (1 << length)) return false;
            for (bitIndex = length - 1; bitIndex >= 0; bitIndex--)
            {
                bool one = ((code >> bitIndex) & 1) != 0;
                if (node.Symbol >= 0) return false;
                if (one)
                {
                    if (node.One == null) node.One = new HuffmanNode();
                    node = node.One;
                }
                else
                {
                    if (node.Zero == null) node.Zero = new HuffmanNode();
                    node = node.Zero;
                }
            }
            if (node.Symbol >= 0 || node.Zero != null || node.One != null) return false;
            node.Symbol = symbol;
            return true;
        }

        private static short GetBranchEntry(HuffmanNode node, List<short> entries,
            List<HuffmanNode> branchNodes, List<int> branchOffsets)
        {
            int index = branchNodes.IndexOf(node);
            int offset;
            if (index >= 0) return (short)(branchOffsets[index] - BranchOffset);
            offset = entries.Count;
            if (offset >= BranchOffset) throw new InvalidOperationException("Huffman table is too large.");
            branchNodes.Add(node);
            branchOffsets.Add(offset);
            entries.Add(0);
            entries.Add(0);
            entries[offset] = GetChildEntry(node.Zero, entries, branchNodes, branchOffsets);
            entries[offset + 1] = GetChildEntry(node.One, entries, branchNodes, branchOffsets);
            return (short)(offset - BranchOffset);
        }

        private static short GetChildEntry(HuffmanNode node, List<short> entries,
            List<HuffmanNode> branchNodes, List<int> branchOffsets)
        {
            if (node == null) throw new InvalidOperationException("Incomplete Huffman tree.");
            if (node.Symbol >= 0) return (short)node.Symbol;
            return GetBranchEntry(node, entries, branchNodes, branchOffsets);
        }

        private sealed class HuffmanNode
        {
            internal int Symbol = -1;
            internal HuffmanNode Zero;
            internal HuffmanNode One;
        }
    }

    public static class JxrHuffmanDecoder
    {
        public static JxrError DecodeSymbol(JxrHuffmanTable table,
            JxrBitReader reader, out int symbol)
        {
            uint rootIndex;
            uint branchBit;
            uint branchIndex;
            int encodedEntry;
            int branchEntry;
            int length;
            JxrError error;

            symbol = 0;
            if (table == null || reader == null) return JxrError.InvalidArgument;
            // The last valid symbol may be shorter than the five-bit root
            // lookup. Pad only the lookahead, never the consumed input.
            int available = Math.Min(reader.BitsRemaining, JxrHuffmanTable.RootBits);
            if (available == 0) return reader.PeekBits(1, out rootIndex);
            error = reader.PeekBits(available, out rootIndex);
            if (error != JxrError.None) return error;
            rootIndex <<= JxrHuffmanTable.RootBits - available;
            if (!table.TryGetEntry(rootIndex, out encodedEntry)) return JxrError.InvalidBitstream;

            length = encodedEntry < 0 ? JxrHuffmanTable.RootBits :
                encodedEntry & ((1 << JxrHuffmanTable.EncodedLengthBits) - 1);
            if (length > available) return reader.ConsumeBits(length);
            error = reader.ConsumeBits(length);
            if (error != JxrError.None) return error;

            symbol = encodedEntry >> JxrHuffmanTable.EncodedLengthBits;
            branchEntry = encodedEntry;
            while (symbol < 0)
            {
                error = reader.ReadBits(1, out branchBit);
                if (error != JxrError.None) return error;
                branchIndex = (uint)(branchEntry + JxrHuffmanTable.BranchOffset) + branchBit;
                if (!table.TryGetEntry(branchIndex, out branchEntry)) return JxrError.InvalidBitstream;
                symbol = branchEntry;
            }
            return JxrError.None;
        }

        public static JxrError DecodeShortSymbol(JxrHuffmanTable table,
            JxrBitReader reader, out int symbol)
        {
            uint rootIndex;
            int encodedEntry;
            int length;
            JxrError error;

            symbol = 0;
            if (table == null || reader == null) return JxrError.InvalidArgument;
            int available = Math.Min(reader.BitsRemaining, JxrHuffmanTable.RootBits);
            if (available == 0) return reader.PeekBits(1, out rootIndex);
            error = reader.PeekBits(available, out rootIndex);
            if (error != JxrError.None) return error;
            rootIndex <<= JxrHuffmanTable.RootBits - available;
            if (!table.TryGetEntry(rootIndex, out encodedEntry) || encodedEntry < 0)
                return JxrError.InvalidBitstream;
            length = encodedEntry & ((1 << JxrHuffmanTable.EncodedLengthBits) - 1);
            if (length > available) return reader.ConsumeBits(length);
            error = reader.ConsumeBits(length);
            if (error != JxrError.None) return error;
            symbol = encodedEntry >> JxrHuffmanTable.EncodedLengthBits;
            return JxrError.None;
        }
    }
}
