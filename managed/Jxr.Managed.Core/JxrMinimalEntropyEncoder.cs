using System;

namespace Jxr.Managed.Core
{
    // One-channel, per-macroblock counterpart of segenc.c. This accepts the
    // already quantized and predicted coefficient layout used by JxrCodecState.
    public static class JxrMinimalEntropyEncoder
    {
        private static readonly int[] BlockOffsets =
            { 0,64,16,80,128,192,144,208,32,96,48,112,160,224,176,240 };
        private static readonly int[] Chroma420Offsets = { 0,32,16,48 };
        private static readonly int[] Chroma422Offsets =
            { 0,64,16,80,32,96,48,112 };
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
                0, 0, writer, out dcEnd, out lpEnd, out hpEnd);
        }

        internal static JxrError EncodeMacroblock(JxrCodecState state,
            int[] coefficients, int[] dc, int orientation, int subbands,
            int trimFlexbits, JxrBitWriter writer,
            out int dcEnd, out int lpEnd, out int hpEnd)
        {
            return EncodeMacroblock(state, coefficients, dc, orientation,
                subbands, trimFlexbits, writer, writer, writer, writer,
                out dcEnd, out lpEnd, out hpEnd);
        }

        // Frequency layout keeps the same macroblock visitation/adaptation
        // order, but routes each subband to its own packet writer.
        internal static JxrError EncodeMacroblock(JxrCodecState state,
            int[] coefficients, int[] dc, int orientation, int subbands,
            int trimFlexbits, JxrBitWriter dcWriter, JxrBitWriter lpWriter,
            JxrBitWriter hpWriter, JxrBitWriter flexWriter,
            out int dcEnd, out int lpEnd, out int hpEnd)
        {
            dcEnd = lpEnd = hpEnd = 0;
            if (state == null || coefficients == null || coefficients.Length != 256 ||
                dc == null || dc.Length != 16 || dcWriter == null ||
                lpWriter == null || hpWriter == null || flexWriter == null ||
                orientation < 0 || orientation > 2 || subbands < 0 || subbands > 3 ||
                trimFlexbits < 0 || trimFlexbits > 15) return JxrError.InvalidArgument;
            JxrError error = EncodeDc(state, dc, dcWriter);
            if (error != JxrError.None) return error;
            dcEnd = dcWriter.BitCount;
            if (subbands != (int)JxrGraySubbandMode.DcOnly)
            {
                error = EncodeLp(state, dc, lpWriter);
                if (error != JxrError.None) return error;
            }
            lpEnd = lpWriter.BitCount;
            if (subbands < (int)JxrGraySubbandMode.NoHighpass)
            {
                state.Entropy.TrimFlexBits = trimFlexbits;
                error = EncodeHp(state, coefficients, orientation, hpWriter,
                    flexWriter, subbands);
                if (error != JxrError.None) return error;
            }
            hpEnd = hpWriter.BitCount;
            return JxrError.None;
        }

        internal static JxrError EncodeYuv444Macroblock(JxrCodecState state,
            int[][] coefficients, int[][] dc, int orientation, int subbands,
            int trimFlexbits, JxrBitWriter writer)
        {
            return EncodeYuv444Macroblock(state, coefficients, dc, orientation,
                subbands, trimFlexbits, writer, writer, writer, writer);
        }

        internal static JxrError EncodeYuv444Macroblock(JxrCodecState state,
            int[][] coefficients, int[][] dc, int orientation, int subbands,
            int trimFlexbits, JxrBitWriter dcWriter, JxrBitWriter lpWriter,
            JxrBitWriter hpWriter, JxrBitWriter flexWriter)
        {
            if (state == null || state.Configuration.ColorFormat != JxrCodecColorFormat.Yuv444 ||
                coefficients == null || coefficients.Length != 3 || dc == null ||
                dc.Length != 3 || dcWriter == null || lpWriter == null ||
                hpWriter == null || flexWriter == null || orientation < 0 || orientation > 2 ||
                subbands < 0 || subbands > 3 || trimFlexbits < 0 || trimFlexbits > 15)
                return JxrError.InvalidArgument;
            for (int channel = 0; channel < 3; channel++)
                if (coefficients[channel] == null || coefficients[channel].Length != 256 ||
                    dc[channel] == null || dc[channel].Length != 16)
                    return JxrError.InvalidArgument;
            JxrError error = EncodeYuv444Dc(state, dc, dcWriter);
            if (error != JxrError.None) return error;
            if (subbands != 3)
            {
                error = EncodeYuv444Lp(state, dc, lpWriter);
                if (error != JxrError.None) return error;
            }
            if (subbands < 2)
            {
                state.Entropy.TrimFlexBits = trimFlexbits;
                error = EncodeYuv444Hp(state, coefficients, orientation,
                    subbands, hpWriter, flexWriter);
                if (error != JxrError.None) return error;
            }
            return JxrError.None;
        }

        internal static JxrError EncodeSubsampledMacroblock(JxrCodecState state,
            int[][] coefficients, int[][] dc, int orientation, int subbands,
            int trimFlexbits, JxrBitWriter writer)
        {
            return EncodeSubsampledMacroblock(state, coefficients, dc, orientation,
                subbands, trimFlexbits, writer, writer, writer, writer);
        }

        internal static JxrError EncodeSubsampledMacroblock(JxrCodecState state,
            int[][] coefficients, int[][] dc, int orientation, int subbands,
            int trimFlexbits, JxrBitWriter dcWriter, JxrBitWriter lpWriter,
            JxrBitWriter hpWriter, JxrBitWriter flexWriter)
        {
            JxrCodecColorFormat color = state.Configuration.ColorFormat;
            if (color != JxrCodecColorFormat.Yuv420 &&
                color != JxrCodecColorFormat.Yuv422)
                return JxrError.InvalidArgument;
            JxrError error = EncodeYuv444Dc(state, dc, dcWriter);
            if (error != JxrError.None) return error;
            if (subbands != 3)
            {
                error = EncodeSubsampledLp(state, dc, lpWriter);
                if (error != JxrError.None) return error;
            }
            if (subbands < 2)
            {
                state.Entropy.TrimFlexBits = trimFlexbits;
                error = EncodeSubsampledHp(state, coefficients, orientation,
                    subbands, hpWriter, flexWriter);
                if (error != JxrError.None) return error;
            }
            return JxrError.None;
        }

        private static JxrError EncodeYuv444Dc(JxrCodecState state,
            int[][] dc, JxrBitWriter writer)
        {
            int ignored, lumaBits, chromaBits;
            state.Entropy.DcModel.Get(0, out ignored, out lumaBits);
            state.Entropy.DcModel.Get(1, out ignored, out chromaBits);
            int flags = 0;
            int[] coarse = new int[3];
            for (int channel = 0; channel < 3; channel++)
            {
                int bits = channel == 0 ? lumaBits : chromaBits;
                coarse[channel] = Math.Abs(dc[channel][0]) >> bits;
                if (coarse[channel] != 0) flags |= 4 >> channel;
            }
            JxrError error = Code(state.Huffman.Get(2), flags, writer, false);
            if (error != JxrError.None) return error;
            int[] means = { 0, 0 };
            for (int channel = 0; channel < 3; channel++)
            {
                int value = dc[channel][0], magnitude = Math.Abs(value);
                int bits = channel == 0 ? lumaBits : chromaBits;
                if (coarse[channel] != 0)
                {
                    error = EncodeLevel(coarse[channel] + 1,
                        state.Huffman.Get(channel == 0 ? 3 : 4), writer);
                    if (error != JxrError.None) return error;
                    means[channel == 0 ? 0 : 1]++;
                }
                error = writer.Write((uint)magnitude, bits);
                if (error != JxrError.None) return error;
                if (magnitude != 0)
                {
                    error = writer.Write((uint)(value < 0 ? 1 : 0), 1);
                    if (error != JxrError.None) return error;
                }
            }
            error = state.Entropy.DcModel.UpdateForMacroblock(
                state.Configuration.ColorFormat, 3, means);
            if (error != JxrError.None) return error;
            if (state.ResetContext && state.Configuration.DcOnly)
                for (int table = 2; table <= 4; table++)
                {
                    error = state.Huffman.Adapt(table);
                    if (error != JxrError.None) return error;
                }
            return JxrError.None;
        }

        private static JxrError EncodeYuv444Lp(JxrCodecState state,
            int[][] dc, JxrBitWriter writer)
        {
            JxrAdaptiveScan scan = state.Entropy.LowpassScan;
            if (state.ResetScan) scan.ResetTotals(16);
            int ignored, lumaBits, chromaBits;
            state.Entropy.LpModel.Get(0, out ignored, out lumaBits);
            state.Entropy.LpModel.Get(1, out ignored, out chromaBits);
            int[][] residuals = { new int[16], new int[16], new int[16] };
            int[][] pairs = { new int[32], new int[32], new int[32] };
            int[] counts = new int[3];
            int cbp = 0;
            for (int channel = 0; channel < 3; channel++)
            {
                counts[channel] = Scan(dc[channel], 0, scan,
                    channel == 0 ? lumaBits : chromaBits, 0,
                    residuals[channel], pairs[channel]);
                if (counts[channel] != 0) cbp |= 1 << channel;
            }
            JxrLowpassCbpState cbpState = state.Entropy.LowpassCbp;
            int coded = cbp;
            if (cbpState.ZeroCount <= 0 || cbpState.MaxCount < 0)
            {
                if (cbpState.MaxCount < cbpState.ZeroCount) coded = 7 - cbp;
                if (coded == 0) writer.Write(0, 1);
                else if (coded == 1) writer.Write(4, 3);
                else writer.Write((uint)(coded + 8), 4);
            }
            else writer.Write((uint)cbp, 3);
            cbpState.Observe(cbp, 7);
            int[] means = { counts[0], counts[1] + counts[2] };
            for (int channel = 0; channel < 3; channel++)
            {
                JxrError error;
                if (counts[channel] != 0)
                {
                    error = EncodeBlock(state, pairs[channel], counts[channel],
                        1, 5, channel != 0, writer);
                    if (error != JxrError.None) return error;
                }
                int bits = channel == 0 ? lumaBits : chromaBits;
                if (bits != 0)
                    for (int index = 1; index < 16; index++)
                    {
                        int residual = residuals[channel][index];
                        error = writer.Write((uint)(residual >> 1),
                            bits + (residual & 1));
                        if (error != JxrError.None) return error;
                    }
            }
            JxrError update = state.Entropy.LpModel.UpdateForMacroblock(
                JxrCodecColorFormat.Yuv444, 3, means);
            if (update != JxrError.None) return update;
            if (state.ResetContext)
                for (int table = 0; table < 13; table++)
                {
                    update = state.Huffman.Adapt(table);
                    if (update != JxrError.None) return update;
                }
            return JxrError.None;
        }

        private static JxrError EncodeSubsampledLp(JxrCodecState state,
            int[][] dc, JxrBitWriter writer)
        {
            JxrCodecColorFormat color = state.Configuration.ColorFormat;
            bool is420 = color == JxrCodecColorFormat.Yuv420;
            JxrAdaptiveScan scan = state.Entropy.LowpassScan;
            if (state.ResetScan) scan.ResetTotals(16);
            int ignored, lumaBits, chromaBits;
            state.Entropy.LpModel.Get(0, out ignored, out lumaBits);
            state.Entropy.LpModel.Get(1, out ignored, out chromaBits);
            int[] lumaResiduals = new int[16], lumaPairs = new int[32];
            int lumaCount = Scan(dc[0], 0, scan, lumaBits, 0,
                lumaResiduals, lumaPairs);
            int[] remap = is420 ? new int[] { 1, 2, 3 } :
                new int[] { 4, 1, 2, 3, 5, 6, 7 };
            int[] chromaPairs = new int[32];
            int chromaCount = 0, run = 0;
            for (int index = 0; index < remap.Length * 2; index++)
            {
                int value = dc[1 + (index & 1)][remap[index >> 1]];
                int coarse = Math.Abs(value) >> chromaBits;
                if (coarse == 0) run++;
                else
                {
                    chromaPairs[2 * chromaCount] = run;
                    chromaPairs[2 * chromaCount + 1] = value < 0 ? -coarse : coarse;
                    chromaCount++; run = 0;
                }
            }
            int cbp = (lumaCount != 0 ? 1 : 0) +
                (chromaCount != 0 ? 2 : 0);
            JxrLowpassCbpState cbpState = state.Entropy.LowpassCbp;
            int coded = cbp;
            JxrError error;
            if (cbpState.ZeroCount <= 0 || cbpState.MaxCount < 0)
            {
                if (cbpState.MaxCount < cbpState.ZeroCount) coded = 3 - cbp;
                if (coded == 0) error = writer.Write(0, 1);
                else if (coded == 1) error = writer.Write(2, 2);
                else error = writer.Write((uint)(coded + 4), 3);
            }
            else error = writer.Write((uint)cbp, 2);
            if (error != JxrError.None) return error;
            cbpState.Observe(cbp, 3);
            if (lumaCount != 0)
            {
                error = EncodeBlock(state, lumaPairs, lumaCount, 1, 5,
                    false, writer);
                if (error != JxrError.None) return error;
            }
            if (lumaBits != 0)
                for (int index = 1; index < 16; index++)
                {
                    int residual = lumaResiduals[index];
                    error = writer.Write((uint)(residual >> 1),
                        lumaBits + (residual & 1));
                    if (error != JxrError.None) return error;
                }
            if (chromaCount != 0)
            {
                error = EncodeBlock(state, chromaPairs, chromaCount,
                    is420 ? 10 : 2, 5, true, writer);
                if (error != JxrError.None) return error;
            }
            if (chromaBits != 0)
                for (int index = 1; index < (is420 ? 4 : 8); index++)
                    for (int channel = 1; channel <= 2; channel++)
                    {
                        int value = dc[channel][index];
                        error = writer.Write((uint)Math.Abs(value), chromaBits);
                        if (error != JxrError.None) return error;
                        if ((Math.Abs(value) >> chromaBits) == 0 && value != 0)
                        {
                            error = writer.Write((uint)(value < 0 ? 1 : 0), 1);
                            if (error != JxrError.None) return error;
                        }
                    }
            error = state.Entropy.LpModel.UpdateForMacroblock(color, 3,
                new int[] { lumaCount, chromaCount });
            if (error != JxrError.None) return error;
            if (state.ResetContext)
                for (int table = 0; table < 13; table++)
                {
                    error = state.Huffman.Adapt(table);
                    if (error != JxrError.None) return error;
                }
            return JxrError.None;
        }

        private static int PredictColorCbp(JxrCodecState state,
            int channel, int cbp)
        {
            return PredictColorCbp(state, channel, cbp, 16);
        }

        private static int PredictColorCbp(JxrCodecState state,
            int channel, int cbp, int blockCount)
        {
            int model = channel == 0 ? 0 : 1;
            int zero, one, mode, top, left;
            state.HighpassCbp.PredictionModel.Get(model, out zero, out one, out mode);
            state.GetNeighborCbp(channel, out top, out left);
            int prediction = state.AtLeftBoundary ?
                (state.AtTopBoundary ? 1 :
                    (top >> (blockCount == 16 ? 10 : blockCount == 8 ? 6 : 2)) & 1) :
                (left >> (blockCount == 16 ? 5 : 1)) & 1;
            if (blockCount == 16)
            {
                prediction |= (cbp & 0x3300) << 2;
                prediction |= (cbp & 0xcc) << 6;
                prediction |= (cbp & 0x33) << 2;
                prediction |= (cbp & 0x11) << 1;
                prediction |= (cbp & 0x2) << 3;
            }
            else
            {
                prediction |= (cbp & 1) << 1;
                prediction |= (cbp & 3) << 2;
                if (blockCount == 8)
                {
                    prediction |= (cbp & 0xc) << 2;
                    prediction |= (cbp & 0x30) << 2;
                }
            }
            int differential = mode == 0 ? prediction ^ cbp :
                mode == 1 ? cbp : cbp ^ ((1 << blockCount) - 1);
            int ones = 0;
            for (int bit = 0; bit < blockCount; bit++) ones += (cbp >> bit) & 1;
            ones *= 16 / blockCount;
            zero = Math.Max(-16, Math.Min(15, zero + ones - 3));
            one = Math.Max(-16, Math.Min(15, one + 16 - ones - 3));
            mode = zero < 0 ? (zero < one ? 1 : 2) : one < 0 ? 2 : 0;
            state.HighpassCbp.PredictionModel.Set(model, zero, one, mode);
            state.SetCurrentCbp(channel, cbp);
            return differential;
        }

        private static JxrError EncodeYuv444Cbp(JxrCodecState state,
            int[] differential, JxrBitWriter writer)
        {
            int pattern = 0, union = differential[0] | differential[1] |
                differential[2];
            for (int group = 0; group < 4; group++)
            {
                pattern |= (union & 15) != 0 ? 16 : 0;
                union >>= 4; pattern >>= 1;
            }
            JxrError error = Code(state.HighpassCbp.CountHuffman,
                CountOnes[pattern], writer, true);
            if (error != JxrError.None) return error;
            error = writer.Write((uint)PatternCode[pattern], PatternLength[pattern]);
            if (error != JxrError.None) return error;
            for (int group = 0; group < 4; group++)
            {
                int y = (differential[0] >> (group * 4)) & 15;
                int u = (differential[1] >> (group * 4)) & 15;
                int v = (differential[2] >> (group * 4)) & 15;
                int chroma = (u != 0 ? 1 : 0) + (v != 0 ? 2 : 0);
                if ((y | u | v) == 0) continue;
                int blockClass = BlockClass[y];
                int symbol = chroma != 0 ?
                    (blockClass > 2 ? 8 : blockClass + 5) : blockClass - 1;
                error = Code(state.HighpassCbp.PatternHuffman, symbol,
                    writer, true);
                if (error != JxrError.None) return error;
                if (chroma != 0)
                {
                    error = writer.Write((uint)(chroma == 1 ? 1 : 3 - chroma),
                        chroma == 1 ? 1 : 2);
                    if (error != JxrError.None) return error;
                }
                if (symbol == 8)
                {
                    error = writer.Write((uint)(blockClass == 3 ? 1 : 5 - blockClass),
                        blockClass == 3 ? 1 : 2);
                    if (error != JxrError.None) return error;
                }
                error = writer.Write((uint)BlockCode[y], BlockLength[y]);
                if (error != JxrError.None) return error;
                int[] chromaNibbles = { u, v };
                for (int channel = 0; channel < 2; channel++)
                    if (chromaNibbles[channel] != 0)
                    {
                        int nibble = chromaNibbles[channel];
                        error = Code(state.Huffman.Get(1),
                            CountOnes[nibble] - 1, writer, false);
                        if (error != JxrError.None) return error;
                        error = writer.Write((uint)PatternCode[nibble],
                            PatternLength[nibble]);
                        if (error != JxrError.None) return error;
                    }
            }
            return JxrError.None;
        }

        private static JxrError EncodeSubsampledCbp(JxrCodecState state,
            int[] differential, JxrBitWriter writer)
        {
            bool is420 = state.Configuration.ColorFormat == JxrCodecColorFormat.Yuv420;
            uint packed = 0;
            for (int group = 0; group < 4; group++)
            {
                int y = (differential[0] >> (group * 4)) & 15;
                int u, v;
                if (is420)
                {
                    u = (differential[1] >> group) & 1;
                    v = (differential[2] >> group) & 1;
                    packed |= (uint)(y | (u << 4) | (v << 5)) << (group * 6);
                }
                else
                {
                    int shift = group == 0 ? 0 : group == 1 ? 1 :
                        group == 2 ? 4 : 5;
                    u = (differential[1] >> shift) & 1;
                    u |= ((differential[1] >> (shift + 2)) & 1) << 1;
                    v = (differential[2] >> shift) & 1;
                    v |= ((differential[2] >> (shift + 2)) & 1) << 1;
                    packed |= (uint)(y | (u << 4) | (v << 6)) << (group * 8);
                }
            }
            int groupWidth = is420 ? 6 : 8;
            int groupMask = (1 << groupWidth) - 1;
            int pattern = 0;
            for (int group = 0; group < 4; group++)
            {
                pattern |= ((packed >> (group * groupWidth)) &
                    (uint)groupMask) != 0 ? 1 << group : 0;
            }
            JxrError error = Code(state.HighpassCbp.CountHuffman,
                CountOnes[pattern], writer, true);
            if (error != JxrError.None) return error;
            error = writer.Write((uint)PatternCode[pattern], PatternLength[pattern]);
            if (error != JxrError.None) return error;
            for (int group = 0; group < 4; group++)
            {
                int code = (int)((packed >> (group * groupWidth)) &
                    (uint)groupMask);
                if (code == 0) continue;
                int y = code & 15, blockClass = BlockClass[y];
                int u = is420 ? (code >> 4) & 1 : (code >> 4) & 3;
                int v = is420 ? (code >> 5) & 1 : (code >> 6) & 3;
                int chroma = (u != 0 ? 1 : 0) + (v != 0 ? 2 : 0);
                int symbol = chroma == 0 ? blockClass - 1 :
                    blockClass > 2 ? 8 : blockClass + 5;
                error = Code(state.HighpassCbp.PatternHuffman,
                    symbol, writer, true);
                if (error != JxrError.None) return error;
                if (chroma != 0)
                {
                    error = writer.Write((uint)(chroma == 1 ? 1 : 3 - chroma),
                        chroma == 1 ? 1 : 2);
                    if (error != JxrError.None) return error;
                }
                if (symbol == 8)
                {
                    error = writer.Write((uint)(blockClass == 3 ? 1 : 5 - blockClass),
                        blockClass == 3 ? 1 : 2);
                    if (error != JxrError.None) return error;
                }
                error = writer.Write((uint)BlockCode[y], BlockLength[y]);
                if (error != JxrError.None) return error;
                if (!is420)
                {
                    int[] chromaCodes = { u, v };
                    for (int channel = 0; channel < 2; channel++)
                        if (chromaCodes[channel] != 0)
                        {
                            int value = chromaCodes[channel];
                            error = writer.Write((uint)(value == 1 ? 1 : 3 - value),
                                value == 1 ? 1 : 2);
                            if (error != JxrError.None) return error;
                        }
                }
            }
            return JxrError.None;
        }

        private static JxrError EncodeYuv444Hp(JxrCodecState state,
            int[][] coefficients, int orientation, int subbands,
            JxrBitWriter writer, JxrBitWriter flexWriter)
        {
            JxrAdaptiveScan scan = orientation == 1 ?
                state.Entropy.VerticalScan : state.Entropy.HorizontalScan;
            if (state.ResetScan)
            {
                state.Entropy.HorizontalScan.ResetTotals(16);
                state.Entropy.VerticalScan.ResetTotals(16);
            }
            int ignored, lumaBits, chromaBits;
            state.Entropy.AcModel.Get(0, out ignored, out lumaBits);
            state.Entropy.AcModel.Get(1, out ignored, out chromaBits);
            int[] cbp = new int[3], differential = new int[3];
            for (int channel = 0; channel < 3; channel++)
            {
                int threshold = (1 << (channel == 0 ? lumaBits : chromaBits)) - 1;
                for (int block = 0; block < 16; block++)
                    for (int index = 1; index < 16; index++)
                        if ((uint)(coefficients[channel][BlockOffsets[block] + index] +
                            threshold) >= (uint)(2 * threshold + 1))
                        { cbp[channel] |= 1 << block; break; }
                differential[channel] = PredictColorCbp(state, channel, cbp[channel]);
            }
            JxrError error = EncodeYuv444Cbp(state, differential, writer);
            if (error != JxrError.None) return error;
            int[] means = { 0, 0 };
            for (int channel = 0; channel < 3; channel++)
            {
                int bits = channel == 0 ? lumaBits : chromaBits;
                int trim = state.Entropy.TrimFlexBits;
                int flex = subbands != (int)JxrGraySubbandMode.NoFlexbits ?
                    Math.Max(0, bits - trim) : 0;
                int[] residuals = new int[16], pairs = new int[32];
                for (int block = 0; block < 16; block++)
                {
                    int offset = BlockOffsets[block];
                    Array.Clear(residuals, 0, residuals.Length);
                    if ((cbp[channel] & (1 << block)) != 0)
                    {
                        int count = Scan(coefficients[channel], offset,
                            scan, bits, trim, residuals, pairs);
                        means[channel == 0 ? 0 : 1] += count;
                        error = EncodeBlock(state, pairs, count, 1, 13,
                            channel != 0, writer);
                        if (error != JxrError.None) return error;
                    }
                    if (flex != 0)
                        for (int index = 1; index < 16; index++)
                        {
                            int coefficientIndex = CoefficientOrder[index];
                            int residual = (cbp[channel] & (1 << block)) != 0 ?
                                residuals[coefficientIndex] :
                                TrimmedResidual(coefficients[channel][offset +
                                    coefficientIndex], trim);
                            error = flexWriter.Write((uint)(residual >> 1),
                                flex + (residual & 1));
                            if (error != JxrError.None) return error;
                        }
                }
            }
            error = state.Entropy.AcModel.UpdateForMacroblock(
                JxrCodecColorFormat.Yuv444, 3, means);
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

        private static JxrError EncodeSubsampledHp(JxrCodecState state,
            int[][] coefficients, int orientation, int subbands,
            JxrBitWriter writer, JxrBitWriter flexWriter)
        {
            JxrCodecColorFormat color = state.Configuration.ColorFormat;
            bool is420 = color == JxrCodecColorFormat.Yuv420;
            JxrAdaptiveScan scan = orientation == 1 ?
                state.Entropy.VerticalScan : state.Entropy.HorizontalScan;
            if (state.ResetScan)
            {
                state.Entropy.HorizontalScan.ResetTotals(16);
                state.Entropy.VerticalScan.ResetTotals(16);
            }
            int ignored, lumaBits, chromaBits;
            state.Entropy.AcModel.Get(0, out ignored, out lumaBits);
            state.Entropy.AcModel.Get(1, out ignored, out chromaBits);
            int chromaBlocks = is420 ? 4 : 8;
            int[] cbp = new int[3], differential = new int[3];
            for (int channel = 0; channel < 3; channel++)
            {
                int[] offsets = channel == 0 ? BlockOffsets :
                    is420 ? Chroma420Offsets : Chroma422Offsets;
                int bits = channel == 0 ? lumaBits : chromaBits;
                int threshold = (1 << bits) - 1;
                int blocks = channel == 0 ? 16 : chromaBlocks;
                for (int block = 0; block < blocks; block++)
                    for (int index = 1; index < 16; index++)
                        if ((uint)(coefficients[channel][offsets[block] + index] +
                            threshold) >= (uint)(2 * threshold + 1))
                        { cbp[channel] |= 1 << block; break; }
                differential[channel] = PredictColorCbp(state, channel,
                    cbp[channel], blocks);
            }
            JxrError error = EncodeSubsampledCbp(state, differential, writer);
            if (error != JxrError.None) return error;
            int[] means = { 0, 0 };
            for (int channel = 0; channel < 3; channel++)
            {
                int bits = channel == 0 ? lumaBits : chromaBits;
                int trim = state.Entropy.TrimFlexBits;
                int flex = subbands != (int)JxrGraySubbandMode.NoFlexbits ?
                    Math.Max(0, bits - trim) : 0;
                int[] offsets = channel == 0 ? BlockOffsets :
                    is420 ? Chroma420Offsets : Chroma422Offsets;
                int blocks = channel == 0 ? 16 : chromaBlocks;
                int[] residuals = new int[16], pairs = new int[32];
                for (int block = 0; block < blocks; block++)
                {
                    int offset = offsets[block];
                    Array.Clear(residuals, 0, residuals.Length);
                    if ((cbp[channel] & (1 << block)) != 0)
                    {
                        int count = Scan(coefficients[channel], offset,
                            scan, bits, trim, residuals, pairs);
                        means[channel == 0 ? 0 : 1] += count;
                        error = EncodeBlock(state, pairs, count, 1, 13,
                            channel != 0, writer);
                        if (error != JxrError.None) return error;
                    }
                    if (flex != 0)
                        for (int index = 1; index < 16; index++)
                        {
                            int coefficientIndex = CoefficientOrder[index];
                            int residual = (cbp[channel] & (1 << block)) != 0 ?
                                residuals[coefficientIndex] :
                                TrimmedResidual(coefficients[channel][offset +
                                    coefficientIndex], trim);
                            error = flexWriter.Write((uint)(residual >> 1),
                                flex + (residual & 1));
                            if (error != JxrError.None) return error;
                        }
                }
            }
            error = state.Entropy.AcModel.UpdateForMacroblock(color, 3, means);
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
            int count, int firstPosition, int contextOffset, bool chroma,
            JxrBitWriter writer)
        {
            int level = pairs[1], significantRun = pairs[0] == 0 ? 1 : 0;
            int large = Math.Abs(level) > 1 ? 1 : 0;
            int remaining = count == 1 ? 0 : pairs[2] > 0 ? 2 : 1;
            int symbol = remaining * 4 + large * 2 + significantRun;
            int blockOffset = contextOffset + (chroma ? 3 : 0);
            JxrAdaptiveHuffman table = state.Huffman.Get(blockOffset);
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
                    table = state.Huffman.Get(blockOffset + continuation + 1);
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
            int modelBits, int trimBits, int[] residuals, int[] pairs)
        {
            // The native zero-model-bit path is distinct from AdaptiveScanTrim
            // even when trimBits is zero; keep its run syntax on the ordinary
            // scan path.  AdaptiveScanTrim applies only to positive model bits.
            bool fullyTrimmed = modelBits > 0 && modelBits <= trimBits;
            int run = fullyTrimmed ? 1 : 0, count = 0;
            int mask = (1 << modelBits) - 1;
            for (int position = 1; position < 16; position++)
            {
                if (fullyTrimmed && position > 1) run++;
                uint index;
                scan.GetCoefficientIndex(position, out index);
                int coefficient = values[offset + (int)index];
                int magnitude = Math.Abs(coefficient);
                int coarse = magnitude >> modelBits;
                if (coarse != 0)
                {
                    if (!fullyTrimmed)
                        residuals[index] = ((magnitude & mask) >> trimBits) * 2;
                    pairs[2 * count] = fullyTrimmed ? run - 1 : run;
                    pairs[2 * count + 1] = coefficient < 0 ? -coarse : coarse;
                    count++; run = 0;
                    scan.ObserveNonZero(position);
                }
                else
                {
                    if (fullyTrimmed)
                    {
                        // Trimmed scan counts the implicit first location
                        // differently; subsequent run increments happen at
                        // the start of each iteration, matching the C loop.
                    }
                    else
                    {
                        int sign = coefficient < 0 ? -1 : 0;
                        int trimmed = unchecked(((coefficient + sign) >> trimBits) - sign);
                        int trimmedSign = trimmed < 0 ? -1 : 0;
                        residuals[index] = unchecked((trimmed ^ trimmedSign) * 4 +
                            (6 & trimmedSign) + (trimmed != 0 ? 1 : 0));
                        run++;
                    }
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
            int count = Scan(dc, 0, scan, bits, 0, residuals, pairs);
            writer.Write((uint)(count > 0 ? 1 : 0), 1);
            if (count != 0)
            {
                JxrError error = EncodeBlock(state, pairs, count, 1, 5, false, writer);
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
            int orientation, JxrBitWriter writer, JxrBitWriter flexWriter,
            int subbands)
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
            int trimBits = state.Entropy.TrimFlexBits;
            int flexBits = Math.Max(0, bits - trimBits);
            bool writeFlexbits = subbands != (int)JxrGraySubbandMode.NoFlexbits &&
                flexBits != 0;
            int[] residuals = new int[16], pairs = new int[32];
            int nonzero = 0;
            for (int block = 0; block < 16; block++)
            {
                int offset = BlockOffsets[block];
                Array.Clear(residuals, 0, residuals.Length);
                int count = 0;
                if ((cbp & (1 << block)) != 0)
                {
                    count = Scan(coefficients, offset, scan, bits, trimBits, residuals, pairs);
                    nonzero += count;
                    error = EncodeBlock(state, pairs, count, 1, 13, false, writer);
                    if (error != JxrError.None) return error;
                }
                if (writeFlexbits)
                    for (int index = 1; index < 16; index++)
                    {
                        int coefficientIndex = CoefficientOrder[index];
                        int residual = (cbp & (1 << block)) != 0 ?
                            residuals[coefficientIndex] :
                            TrimmedResidual(coefficients[offset + coefficientIndex], trimBits);
                        flexWriter.Write((uint)(residual >> 1), flexBits + (residual & 1));
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

        private static int TrimmedResidual(int coefficient, int trimBits)
        {
            int sign = coefficient < 0 ? -1 : 0;
            int trimmed = unchecked(((coefficient + sign) >> trimBits) - sign);
            return unchecked((trimmed ^ sign) * 4 + (6 & sign) +
                (trimmed != 0 ? 1 : 0));
        }
    }
}
