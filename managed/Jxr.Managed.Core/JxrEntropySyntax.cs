using System;

namespace Jxr.Managed.Core
{
    // Shared entropy syntax from JxrEntropyLevelDecoder,
    // JxrEntropyBlockDecoder, JxrLpResidualDecoder and the QP index reader.
    internal static class JxrEntropySyntax
    {
        private static readonly int[] baseLevels = { 2, 3, 4, 6, 10, 14 };
        private static readonly int[] extraLevelBits = { 0, 0, 1, 2, 2, 2 };
        private static readonly int[] runRemap =
            { 1,2,3,5,7, 1,2,3,5,7, 1,2,3,4,5 };
        private static readonly int[] runBins =
            { -1,-1,-1,-1, 2,2,2, 1,1,1,1, 0,0,0,0 };
        private static readonly int[] runFixedBits =
            { 0,0,1,1,3, 0,0,1,1,2, 0,0,0,0,1 };

        internal static JxrError Read(JxrBitReader reader, int count, out int value)
        {
            uint bits;
            value = 0;
            JxrError error = reader.ReadBits(count, out bits);
            if (error == JxrError.None) value = unchecked((int)bits);
            return error;
        }

        internal static JxrError DecodeShortSymbol(JxrAdaptiveHuffman state,
            JxrBitReader reader, out int symbol)
        {
            symbol = 0;
            if (state == null || state.DecoderTable == null) return JxrError.InvalidArgument;
            return JxrHuffmanDecoder.DecodeShortSymbol(state.DecoderTable, reader, out symbol);
        }

        internal static JxrError DecodeLevel(JxrAdaptiveHuffman state,
            JxrBitReader reader, out int level)
        {
            int symbol, count, extra;
            JxrError error;
            level = 0;
            if (state == null) return JxrError.InvalidArgument;
            error = state.DecodeSymbol(reader, out symbol);
            if (error != JxrError.None) return error;
            if (symbol < 0 || symbol > 6) return JxrError.InvalidBitstream;
            if (symbol < 2) { level = symbol + 2; return JxrError.None; }
            if (symbol < 6)
            {
                error = Read(reader, extraLevelBits[symbol], out extra);
                if (error == JxrError.None) level = baseLevels[symbol] + extra;
                return error;
            }
            error = Read(reader, 4, out count);
            if (error != JxrError.None) return error;
            count += 4;
            if (count == 19)
            {
                error = Read(reader, 2, out extra);
                if (error != JxrError.None) return error;
                count += extra;
                if (count == 22)
                {
                    error = Read(reader, 3, out extra);
                    if (error != JxrError.None) return error;
                    count += extra;
                }
            }
            error = Read(reader, count, out extra);
            if (error == JxrError.None) level = unchecked(2 + (1 << count) + extra);
            return error;
        }

        internal static JxrError DecodeRun(int maximumRun, JxrAdaptiveHuffman state,
            JxrBitReader reader, out int run)
        {
            int bit, symbol, index;
            JxrError error;
            run = 0;
            if (maximumRun < 1 || maximumRun > 14) return JxrError.InvalidBitstream;
            if (maximumRun < 5)
            {
                if (maximumRun == 1) { run = 1; return JxrError.None; }
                error = Read(reader, 1, out bit);
                if (error != JxrError.None) return error;
                if (bit != 0) { run = 1; return JxrError.None; }
                if (maximumRun == 2) { run = 2; return JxrError.None; }
                error = Read(reader, 1, out bit);
                if (error != JxrError.None) return error;
                if (bit != 0) { run = 2; return JxrError.None; }
                if (maximumRun == 3) { run = 3; return JxrError.None; }
                error = Read(reader, 1, out bit);
                if (error != JxrError.None) return error;
                run = bit != 0 ? 3 : 4;
                return JxrError.None;
            }
            error = DecodeShortSymbol(state, reader, out symbol);
            if (error != JxrError.None) return error;
            index = symbol + runBins[maximumRun] * 5;
            if (index < 0 || index >= runRemap.Length) return JxrError.InvalidBitstream;
            run = runRemap[index];
            if (runFixedBits[index] == 0) return JxrError.None;
            error = Read(reader, runFixedBits[index], out bit);
            if (error == JxrError.None) run += bit;
            return error;
        }

        internal static JxrError DecodeNextSymbol(int coefficientPosition,
            JxrAdaptiveHuffman state, JxrBitReader reader, out int symbol)
        {
            int bit;
            JxrError error;
            symbol = 0;
            if (coefficientPosition < 15)
            {
                error = DecodeShortSymbol(state, reader, out symbol);
                if (error != JxrError.None) return error;
                return state.ObserveSymbol(symbol);
            }
            if (coefficientPosition == 15)
            {
                error = Read(reader, 1, out bit);
                if (error != JxrError.None) return error;
                if (bit == 0) return JxrError.None;
                error = Read(reader, 1, out bit);
                if (error != JxrError.None) return error;
                if (bit == 0) { symbol = 2; return JxrError.None; }
                error = Read(reader, 1, out bit);
                if (error == JxrError.None) symbol = 1 + 2 * bit;
                return error;
            }
            return Read(reader, 1, out symbol);
        }

        internal static JxrError DecodeLowpassBlock(bool chroma, int[] pairs,
            JxrHuffmanStateSet huffman, JxrBitReader reader, int startPosition,
            out int nonZeroCount)
        {
            int symbolState = 5 + (chroma ? 3 : 0);
            int symbol, significantRun, remaining, context, sign, level, run;
            JxrError error;
            nonZeroCount = 0;
            error = huffman.Get(symbolState).DecodeSymbol(reader, out symbol);
            if (error != JxrError.None) return error;
            significantRun = symbol & 1;
            remaining = symbol >> 2;
            context = significantRun & remaining;
            error = Read(reader, 1, out sign);
            if (error != JxrError.None) return error;
            level = 1;
            if ((symbol & 2) != 0)
            {
                error = DecodeLevel(huffman.Get(11 + context), reader, out level);
                if (error != JxrError.None) return error;
            }
            pairs[1] = sign != 0 ? -level : level;
            pairs[0] = 0;
            if (significantRun == 0)
            {
                error = DecodeRun(15 - startPosition, huffman.Get(0), reader, out run);
                if (error != JxrError.None) return error;
                pairs[0] = run;
            }
            startPosition += pairs[0] + 1;
            nonZeroCount = 1;
            while (remaining != 0)
            {
                if (nonZeroCount >= 16) return JxrError.InvalidBitstream;
                significantRun = remaining & 1;
                run = 0;
                if (significantRun == 0)
                {
                    error = DecodeRun(15 - startPosition, huffman.Get(0), reader, out run);
                    if (error != JxrError.None) return error;
                }
                pairs[nonZeroCount * 2] = run;
                startPosition += run + 1;
                error = DecodeNextSymbol(startPosition,
                    huffman.Get(symbolState + context + 1), reader, out symbol);
                if (error != JxrError.None) return error;
                remaining = symbol >> 1;
                if (remaining < 0 || remaining >= 3) return JxrError.InvalidBitstream;
                context &= remaining;
                error = Read(reader, 1, out sign);
                if (error != JxrError.None) return error;
                level = 1;
                if ((symbol & 1) != 0)
                {
                    error = DecodeLevel(huffman.Get(11 + context), reader, out level);
                    if (error != JxrError.None) return error;
                }
                pairs[nonZeroCount * 2 + 1] = sign != 0 ? -level : level;
                nonZeroCount++;
            }
            return JxrError.None;
        }

        internal static JxrError ReadQuantizerIndex(JxrBitReader reader,
            int bitCount, out byte index)
        {
            int flag, value;
            JxrError error;
            index = 0;
            error = Read(reader, 1, out flag);
            if (error != JxrError.None || flag == 0) return error;
            error = Read(reader, bitCount, out value);
            if (error == JxrError.None) index = unchecked((byte)(value + 1));
            return error;
        }

        internal static int CombineNonZero(int coefficient, uint residual, int bits)
        {
            uint rotated = JxrBitMath.RotateLeft32(unchecked((uint)coefficient), (uint)bits);
            uint mask = JxrBitMath.LowMask32((uint)bits);
            return unchecked((int)((rotated ^ residual) - (rotated & mask)));
        }

        internal static int CombineSignedMagnitude(int coefficient, uint residual, int bits)
        {
            uint magnitude = unchecked((uint)(coefficient < 0 ? -coefficient : coefficient));
            int refined = unchecked((int)((magnitude << bits) + residual));
            return coefficient < 0 ? unchecked(-refined) : refined;
        }

        internal static JxrError ReadSignedResidual(JxrBitReader reader,
            int bitCount, out int value)
        {
            uint encoded;
            JxrError error;
            value = 0;
            error = reader.PeekBits(bitCount + 1, out encoded);
            if (error != JxrError.None) return error;
            int magnitude = unchecked((int)(encoded >> 1));
            value = magnitude == 0 ? 0 : (encoded & 1) != 0 ? -magnitude : magnitude;
            return reader.ConsumeBits(bitCount + (value != 0 ? 1 : 0));
        }
    }
}
