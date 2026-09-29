using System;

namespace Jxr.Managed.Core
{
    // One-channel, per-macroblock counterpart of segenc.c. This accepts the
    // already quantized and predicted coefficient layout used by JxrCodecState.
    public static class JxrMinimalEntropyEncoder
    {
        private static readonly int[] BlockOffsets =
            { 0,64,16,80,128,192,144,208,32,96,48,112,160,224,176,240 };
        private static readonly int[] CoefficientOrder =
            { 0,5,1,6,10,12,8,14,2,4,3,7,9,13,11,15 };
        private static readonly int[] LevelIndex =
            { 0,1,2,2,3,3,3,3,4,4,4,4,5,5,5,5 };
        private static readonly int[] LevelFixed = { 0,0,1,2,2,2 };
        private static readonly int[] RunIndex = {
            0,1,2,2,3,3,4,4,4,4,4,4,4,4,
            0,1,2,2,3,3,4,4,4,4,0,0,0,0,
            0,1,2,3,4,4 };
        private static readonly int[] RunBin =
            { -1,-1,-1,-1,2,2,2,1,1,1,1,0,0,0,0 };
        private static readonly int[] RunFixed =
            { 0,0,1,1,3,0,0,1,1,2,0,0,0,0,1 };
        private static readonly int[] CountOnes =
            { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
        private static readonly int[] PatternLength =
            { 0,2,2,2,2,2,3,2,2,3,3,2,3,2,2,0 };
        private static readonly int[] PatternCode =
            { 0,0,1,0,2,1,4,3,3,5,6,2,7,1,0,0 };
        private static readonly int[] BlockClass =
            { 0,1,1,2,1,3,3,4,1,3,3,4,2,4,4,5 };
        private static readonly int[] BlockLength =
            { 0,2,2,1,2,2,2,2,2,2,2,2,1,2,2,0 };
        private static readonly int[] BlockCode =
            { 0,0,1,0,2,0,1,0,3,2,3,1,1,2,3,0 };

        public static JxrError Encode(int[] coefficients, int[] dc, int orientation,
            JxrBitWriter writer, out int dcEnd, out int lpEnd, out int hpEnd)
        {
            dcEnd = lpEnd = hpEnd = 0;
            if (coefficients == null || coefficients.Length != 256 ||
                dc == null || dc.Length != 16 || writer == null ||
                orientation < 0 || orientation > 2) return JxrError.InvalidArgument;
            JxrCodecConfiguration format = new JxrCodecConfiguration(
                JxrCodecColorFormat.YOnly, 1, true, false, true, true,
                false, true, true, false, false, 0, 0, 1, 1,
                new int[][] { new int[] { 1 } });
            JxrBitReader unused = new JxrBitReader(new byte[0]);
            JxrCodecState state = new JxrCodecState(format, unused, unused, unused, unused);
            return EncodeMacroblock(state, coefficients, dc, orientation,
                writer, out dcEnd, out lpEnd, out hpEnd);
        }

        internal static JxrError EncodeMacroblock(JxrCodecState state,
            int[] coefficients, int[] dc, int orientation, JxrBitWriter writer,
            out int dcEnd, out int lpEnd, out int hpEnd)
        {
            dcEnd = lpEnd = hpEnd = 0;
            if (state == null || coefficients == null || coefficients.Length != 256 ||
                dc == null || dc.Length != 16 || writer == null ||
                orientation < 0 || orientation > 2) return JxrError.InvalidArgument;
            JxrError error = EncodeDc(state, dc, writer);
            if (error != JxrError.None) return error;
            dcEnd = writer.BitCount;
            error = EncodeLp(state, dc, writer);
            if (error != JxrError.None) return error;
            lpEnd = writer.BitCount;
            error = EncodeHp(state, coefficients, orientation, writer);
            if (error != JxrError.None) return error;
            hpEnd = writer.BitCount;
            return JxrError.None;
        }

        private static JxrError Code(JxrAdaptiveHuffman state, int symbol,
            JxrBitWriter writer, bool observe)
        {
            uint code; int bits;
            JxrError error = state.GetCodeWord(symbol, out code, out bits);
            if (error != JxrError.None) return error;
            error = writer.Write(code, bits);
            if (error != JxrError.None) return error;
            return observe ? state.ObserveSymbol(symbol) : JxrError.None;
        }

        private static JxrError EncodeLevel(int magnitude, JxrAdaptiveHuffman state,
            JxrBitWriter writer)
        {
            if (magnitude < 2) return JxrError.InvalidArgument;
            int reduced = magnitude - 2, symbol, fixedBits;
            if (reduced < 16)
            {
                symbol = LevelIndex[reduced]; fixedBits = LevelFixed[symbol];
            }
            else
            {
                symbol = 6;
                int high = reduced >> 5;
                fixedBits = 4;
                while (high != 0) { fixedBits++; high >>= 1; }
                if (fixedBits > 29) return JxrError.UnsupportedFeature;
            }
            JxrError error = Code(state, symbol, writer, true);
            if (error != JxrError.None) return error;
            if (symbol == 6)
            {
                if (fixedBits > 18)
                {
                    writer.Write(15, 4);
                    if (fixedBits > 21)
                    { writer.Write(3, 2); writer.Write((uint)(fixedBits - 22), 3); }
                    else writer.Write((uint)(fixedBits - 19), 2);
                }
                else writer.Write((uint)(fixedBits - 4), 4);
            }
            return writer.Write((uint)reduced, fixedBits);
        }

        private static JxrError EncodeRun(int run, int maximum,
            JxrAdaptiveHuffman state, JxrBitWriter writer)
        {
            if (run < 1 || run > maximum || maximum < 1 || maximum > 14)
                return JxrError.InvalidArgument;
            if (maximum < 5)
            {
                int[] lengths = { 3,3,2,1 };
                return maximum == 1 ? JxrError.None : writer.Write(
                    (uint)(maximum != run ? 1 : 0),
                    lengths[maximum - run] - (4 - maximum));
            }
            int bin = RunBin[maximum];
            int symbol = RunIndex[run + bin * 14 - 1];
            JxrError error = Code(state, symbol, writer, false);
            if (error != JxrError.None) return error;
            return writer.Write((uint)(run + 1), RunFixed[symbol + bin * 5]);
        }

        private static JxrError EncodeBlock(JxrCodecState state, int[] pairs,
            int count, int firstPosition, int contextOffset, JxrBitWriter writer)
        {
            int level = pairs[1], significantRun = pairs[0] == 0 ? 1 : 0;
            int large = Math.Abs(level) > 1 ? 1 : 0;
            int remaining = count == 1 ? 0 : pairs[2] > 0 ? 2 : 1;
            int symbol = remaining * 4 + large * 2 + significantRun;
            JxrAdaptiveHuffman table = state.Huffman.Get(contextOffset);
            uint code; int bits;
            JxrError error = table.GetCodeWord(symbol, out code, out bits);
            if (error != JxrError.None) return error;
            error = writer.Write(code * 2U + (uint)(level < 0 ? 1 : 0), bits + 1);
            if (error != JxrError.None) return error;
            error = table.ObserveSymbol(symbol);
            if (error != JxrError.None) return error;
            int continuation = significantRun & remaining;
            if (large != 0)
            {
                error = EncodeLevel(Math.Abs(level), state.Huffman.Get(6 + contextOffset + continuation), writer);
                if (error != JxrError.None) return error;
            }
            if (significantRun == 0)
            {
                error = EncodeRun(pairs[0], 15 - firstPosition, state.Huffman.Get(0), writer);
                if (error != JxrError.None) return error;
            }
            int position = firstPosition + pairs[0] + 1;
            for (int k = 1; k < count; k++)
            {
                if (remaining == 2)
                {
                    error = EncodeRun(pairs[2 * k], 15 - position, state.Huffman.Get(0), writer);
                    if (error != JxrError.None) return error;
                }
                position += pairs[2 * k] + 1;
                remaining = k == count - 1 ? 0 : pairs[2 * k + 2] > 0 ? 2 : 1;
                level = pairs[2 * k + 1];
                large = Math.Abs(level) > 1 ? 1 : 0;
                symbol = remaining * 2 + large;
                if (position < 15)
                {
                    table = state.Huffman.Get(contextOffset + continuation + 1);
                    error = table.GetCodeWord(symbol, out code, out bits);
                    if (error != JxrError.None) return error;
                    error = writer.Write(code * 2U + (uint)(level < 0 ? 1 : 0), bits + 1);
                    if (error != JxrError.None) return error;
                    error = table.ObserveSymbol(symbol);
                    if (error != JxrError.None) return error;
                }
                else if (position == 15)
                {
                    int[] codes = { 0,6,2,7 }, lengths = { 1,3,2,3 };
                    error = writer.Write((uint)(codes[symbol] * 2 + (level < 0 ? 1 : 0)),
                        lengths[symbol] + 1);
                    if (error != JxrError.None) return error;
                }
                else
                {
                    error = writer.Write((uint)(symbol * 2 + (level < 0 ? 1 : 0)), 2);
                    if (error != JxrError.None) return error;
                }
                continuation &= remaining;
                if (large != 0)
                {
                    error = EncodeLevel(Math.Abs(level),
                        state.Huffman.Get(6 + contextOffset + continuation), writer);
                    if (error != JxrError.None) return error;
                }
            }
            return JxrError.None;
        }

        private static int Scan(int[] values, int offset, JxrAdaptiveScan scan,
            int modelBits, int[] residuals, int[] pairs)
        {
            int run = 0, count = 0, mask = (1 << modelBits) - 1;
            for (int position = 1; position < 16; position++)
            {
                uint index;
                scan.GetCoefficientIndex(position, out index);
                int coefficient = values[offset + (int)index];
                int magnitude = Math.Abs(coefficient);
                int coarse = magnitude >> modelBits;
                if (coarse != 0)
                {
                    residuals[index] = (magnitude & mask) * 2;
                    pairs[2 * count] = run;
                    pairs[2 * count + 1] = coefficient < 0 ? -coarse : coarse;
                    count++; run = 0;
                    scan.ObserveNonZero(position);
                }
                else
                {
                    residuals[index] = magnitude * 4 +
                        (coefficient < 0 ? 2 : 0) + (coefficient != 0 ? 1 : 0);
                    run++;
                }
            }
            return count;
        }

        private static JxrError EncodeDc(JxrCodecState state, int[] dc, JxrBitWriter writer)
        {
            int ignored, bits;
            state.Entropy.DcModel.Get(0, out ignored, out bits);
            int magnitude = Math.Abs(dc[0]);
            int coarse = magnitude >> bits;
            writer.Write((uint)(coarse == 0 ? 0 : 1), 1);
            if (coarse != 0)
            {
                JxrError error = EncodeLevel(coarse + 1, state.Huffman.Get(3), writer);
                if (error != JxrError.None) return error;
            }
            writer.Write((uint)magnitude, bits);
            if (magnitude != 0) writer.Write((uint)(dc[0] < 0 ? 1 : 0), 1);
            return state.Entropy.DcModel.UpdateForMacroblock(
                JxrCodecColorFormat.YOnly, 1, new int[] { coarse == 0 ? 0 : 1, 0 });
        }

        private static JxrError EncodeLp(JxrCodecState state, int[] dc, JxrBitWriter writer)
        {
            JxrAdaptiveScan scan = state.Entropy.LowpassScan;
            if (state.ResetScan) scan.ResetTotals(16);
            int ignored, bits;
            state.Entropy.LpModel.Get(0, out ignored, out bits);
            int[] residuals = new int[16], pairs = new int[32];
            int count = Scan(dc, 0, scan, bits, residuals, pairs);
            writer.Write((uint)(count > 0 ? 1 : 0), 1);
            if (count != 0)
            {
                JxrError error = EncodeBlock(state, pairs, count, 1, 5, writer);
                if (error != JxrError.None) return error;
            }
            if (bits != 0)
                for (int index = 1; index < 16; index++)
                    writer.Write((uint)(residuals[index] >> 1), bits + (residuals[index] & 1));
            JxrError update = state.Entropy.LpModel.UpdateForMacroblock(
                JxrCodecColorFormat.YOnly, 1, new int[] { count, 0 });
            if (update != JxrError.None) return update;
            for (int table = 0; state.ResetContext && table < 13; table++)
            {
                update = state.Huffman.Adapt(table);
                if (update != JxrError.None) return update;
            }
            return JxrError.None;
        }

        private static JxrError EncodeCbp(JxrCodecState state, int differential,
            JxrBitWriter writer)
        {
            int pattern = 0, value = differential;
            for (int block = 0; block < 4; block++)
            {
                pattern |= (value & 15) != 0 ? 16 : 0;
                value >>= 4; pattern >>= 1;
            }
            JxrAdaptiveHuffman countTable = state.HighpassCbp.CountHuffman;
            JxrAdaptiveHuffman patternTable = state.HighpassCbp.PatternHuffman;
            JxrError error = Code(countTable, CountOnes[pattern], writer, true);
            if (error != JxrError.None) return error;
            writer.Write((uint)PatternCode[pattern], PatternLength[pattern]);
            value = differential;
            for (int block = 0; block < 4; block++)
            {
                int nibble = value & 15;
                value >>= 4;
                if (nibble == 0) continue;
                error = Code(patternTable, BlockClass[nibble] - 1, writer, true);
                if (error != JxrError.None) return error;
                writer.Write((uint)BlockCode[nibble], BlockLength[nibble]);
            }
            return JxrError.None;
        }

        private static int PredictCbp(JxrCodecState state, int cbp)
        {
            int zero, one, mode;
            state.HighpassCbp.PredictionModel.Get(0, out zero, out one, out mode);
            int top, left;
            state.GetNeighborCbp(0, out top, out left);
            int prediction = state.AtLeftBoundary ?
                (state.AtTopBoundary ? 1 : (top >> 10) & 1) :
                (left >> 5) & 1;
            prediction |= (cbp & 0x3300) << 2;
            prediction |= (cbp & 0xcc) << 6;
            prediction |= (cbp & 0x33) << 2;
            prediction |= (cbp & 0x11) << 1;
            prediction |= (cbp & 0x2) << 3;
            int differential = mode == 0 ? prediction ^ cbp :
                mode == 1 ? cbp : cbp ^ 0xffff;
            int ones = 0;
            for (int index = 0; index < 16; index++) ones += (cbp >> index) & 1;
            zero = Math.Max(-16, Math.Min(15, zero + ones - 3));
            one = Math.Max(-16, Math.Min(15, one + 16 - ones - 3));
            mode = zero < 0 ? (zero < one ? 1 : 2) : one < 0 ? 2 : 0;
            state.HighpassCbp.PredictionModel.Set(0, zero, one, mode);
            state.SetCurrentCbp(0, cbp);
            return differential;
        }

        private static JxrError EncodeHp(JxrCodecState state, int[] coefficients,
            int orientation, JxrBitWriter writer)
        {
            JxrAdaptiveScan scan = orientation == 1 ?
                state.Entropy.VerticalScan : state.Entropy.HorizontalScan;
            if (state.ResetScan)
            {
                state.Entropy.HorizontalScan.ResetTotals(16);
                state.Entropy.VerticalScan.ResetTotals(16);
            }
            int ignored, bits;
            state.Entropy.AcModel.Get(0, out ignored, out bits);
            int cbp = 0, threshold = (1 << bits) - 1;
            for (int block = 0; block < 16; block++)
                for (int index = 1; index < 16; index++)
                    if ((uint)(coefficients[BlockOffsets[block] + index] + threshold) >=
                        (uint)(2 * threshold + 1))
                    { cbp |= 1 << block; break; }
            int differential = PredictCbp(state, cbp);
            JxrError error = EncodeCbp(state, differential, writer);
            if (error != JxrError.None) return error;
            int[] residuals = new int[16], pairs = new int[32];
            int nonzero = 0;
            for (int block = 0; block < 16; block++)
            {
                int offset = BlockOffsets[block];
                Array.Clear(residuals, 0, residuals.Length);
                int count = 0;
                if ((cbp & (1 << block)) != 0)
                {
                    count = Scan(coefficients, offset, scan, bits, residuals, pairs);
                    nonzero += count;
                    error = EncodeBlock(state, pairs, count, 1, 13, writer);
                    if (error != JxrError.None) return error;
                }
                if (bits != 0)
                    for (int index = 1; index < 16; index++)
                    {
                        int coefficientIndex = CoefficientOrder[index];
                        int residual = (cbp & (1 << block)) != 0 ?
                            residuals[coefficientIndex] :
                            Math.Abs(coefficients[offset + coefficientIndex]) * 4 +
                            (coefficients[offset + coefficientIndex] < 0 ? 2 : 0) +
                            (coefficients[offset + coefficientIndex] != 0 ? 1 : 0);
                        writer.Write((uint)(residual >> 1), bits + (residual & 1));
                    }
            }
            error = state.Entropy.AcModel.UpdateForMacroblock(
                JxrCodecColorFormat.YOnly, 1, new int[] { nonzero, 0 });
            if (error != JxrError.None) return error;
            if (state.ResetContext)
            {
                error = state.HighpassCbp.Adapt();
                if (error != JxrError.None) return error;
                for (int table = 13; table < 21; table++)
                {
                    error = state.Huffman.Adapt(table);
                    if (error != JxrError.None) return error;
                }
            }
            return JxrError.None;
        }
    }
}
