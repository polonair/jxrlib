namespace Jxr.Managed.Core
{
    // Managed form of JxrHpDecoder: CBP syntax, inverse prediction, block
    // entropy and flexbits.  The caller owns packet refill and MB neighbours.
    public static class JxrHpCodec
    {
        private static readonly int[] blockOffsets =
            { 0,64,16,80,128,192,144,208,32,96,48,112,160,224,176,240 };
        private static readonly int[] uvOffsets = { 0,32,16,48 };
        private static readonly int[] uv422Offsets = { 0,64,16,80,32,96,48,112 };
        private static readonly int[] coefficientOrder =
            { 0,5,1,6,10,12,8,14,2,4,3,7,9,13,11,15 };
        private static readonly int[] flc0 = { 0,2,1,2,2,0 };
        private static readonly int[] off0 = { 0,4,2,8,12,1 };
        private static readonly int[] out0 =
            { 0,15,3,12,1,2,4,8,5,6,9,10,7,11,13,14 };

        public static JxrError Decode(JxrCodecState state)
        {
            if (state == null) return JxrError.InvalidArgument;
            JxrCodecConfiguration format = state.Configuration;
            JxrError error;
            byte index;
            if (state.ResetScan)
            {
                error = state.Entropy.HorizontalScan.ResetTotals(16);
                if (error != JxrError.None) return error;
                error = state.Entropy.VerticalScan.ResetTotals(16);
                if (error != JxrError.None) return error;
            }
            if (!format.Spatial && format.HighpassQuantizerBits > 0)
            {
                error = JxrEntropySyntax.ReadQuantizerIndex(state.HpReader,
                    format.HighpassQuantizerBits, out index);
                if (error != JxrError.None) return error;
                state.Macroblock.SetHighpassQuantizerIndex(index);
                if (index >= format.HighpassQuantizerCount) return JxrError.InvalidBitstream;
            }
            else if (format.HighpassQuantizerBits == 0 && format.HighpassQuantizerCount > 1)
                state.Macroblock.SetHighpassQuantizerIndex(state.Macroblock.LowpassQuantizerIndex);
            error = DecodeCbp(state);
            if (error != JxrError.None) return error;
            int predictionChannels = format.ColorFormat == JxrCodecColorFormat.Yuv420 ||
                format.ColorFormat == JxrCodecColorFormat.Yuv422 ? 1 : format.ChannelCount;
            for (int channel = 0; channel < predictionChannels; channel++)
            {
                int differential, cbp;
                error = state.MacroblockCbp.GetDifferential(channel, out differential);
                if (error != JxrError.None) return error;
                error = JxrCbpPrediction.Decode(state, channel, differential, 16, out cbp);
                if (error != JxrError.None) return error;
            }
            if (format.ColorFormat == JxrCodecColorFormat.Yuv420 ||
                format.ColorFormat == JxrCodecColorFormat.Yuv422)
                for (int channel = 1; channel <= 2; channel++)
                {
                    int differential, cbp;
                    error = state.MacroblockCbp.GetDifferential(channel, out differential);
                    if (error != JxrError.None) return error;
                    error = JxrCbpPrediction.Decode(state, channel, differential,
                        format.ColorFormat == JxrCodecColorFormat.Yuv420 ? 4 : 8, out cbp);
                    if (error != JxrError.None) return error;
                }
            error = DecodeCoefficients(state);
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

        private static JxrError DecodeCbp(JxrCodecState state)
        {
            JxrCodecConfiguration format = state.Configuration;
            JxrBitReader reader = state.HpReader;
            JxrAdaptiveHuffman pattern = state.HighpassCbp.PatternHuffman;
            JxrAdaptiveHuffman count = state.HighpassCbp.CountHuffman;
            int iterations = format.ColorFormat == JxrCodecColorFormat.NComponent ||
                format.ColorFormat == JxrCodecColorFormat.Cmyk ? format.ChannelCount : 1;
            for (int channel = 0; channel < iterations; channel++)
            {
                int cy = 0, cu = 0, cv = 0, number, bit;
                JxrError error = count.DecodeSymbol(reader, out number);
                if (error != JxrError.None) return error;
                error = DecodePattern(number, reader, out number);
                if (error != JxrError.None) return error;
                for (int block = 0; block < 4; block++)
                {
                    if ((number & (1 << block)) == 0) continue;
                    int symbol;
                    error = pattern.DecodeSymbol(reader, out symbol);
                    if (error != JxrError.None) return error;
                    int value = symbol + 1;
                    int chroma = 0;
                    if (value >= 6)
                    {
                        error = JxrEntropySyntax.Read(reader, 1, out bit);
                        if (error != JxrError.None) return error;
                        if (bit != 0) chroma = 0x10;
                        else
                        {
                            error = JxrEntropySyntax.Read(reader, 1, out bit);
                            if (error != JxrError.None) return error;
                            chroma = bit != 0 ? 0x20 : 0x30;
                        }
                        if (value == 9)
                        {
                            error = JxrEntropySyntax.Read(reader, 1, out bit);
                            if (error != JxrError.None) return error;
                            if (bit == 0)
                            {
                                error = JxrEntropySyntax.Read(reader, 1, out bit);
                                if (error != JxrError.None) return error;
                                value = bit != 0 ? 10 : 11;
                            }
                        }
                        value -= 6;
                    }
                    if (value < 0 || value >= off0.Length) return JxrError.InvalidBitstream;
                    int code = off0[value];
                    if (flc0[value] != 0)
                    {
                        error = JxrEntropySyntax.Read(reader, flc0[value], out bit);
                        if (error != JxrError.None) return error;
                        code += bit;
                    }
                    if (code < 0 || code >= out0.Length) return JxrError.InvalidBitstream;
                    int blockCbp = chroma + out0[code];
                    cy |= (blockCbp & 15) << (block * 4);
                    if (format.ColorFormat == JxrCodecColorFormat.Yuv444)
                    {
                        for (int c = 0; c < 2; c++)
                            if (((blockCbp >> (c + 4)) & 1) != 0)
                            {
                                int chromaCode;
                                error = JxrEntropySyntax.DecodeShortSymbol(state.Huffman.Get(1),
                                    reader, out chromaCode);
                                if (error != JxrError.None) return error;
                                error = DecodePattern444(chromaCode, reader, out chromaCode);
                                if (error != JxrError.None) return error;
                                if (c == 0) cu |= chromaCode << (block * 4);
                                else cv |= chromaCode << (block * 4);
                            }
                    }
                    else if (format.ColorFormat == JxrCodecColorFormat.Yuv420)
                    {
                        cu |= ((blockCbp >> 4) & 1) << block;
                        cv |= ((blockCbp >> 5) & 1) << block;
                    }
                    else if (format.ColorFormat == JxrCodecColorFormat.Yuv422)
                    {
                        for (int c = 0; c < 2; c++)
                            if (((blockCbp >> (c + 4)) & 1) != 0)
                            {
                                error = JxrEntropySyntax.Read(reader, 1, out bit);
                                if (error != JxrError.None) return error;
                                int chromaCode = 5;
                                if (bit != 0) chromaCode = 1;
                                else
                                {
                                    error = JxrEntropySyntax.Read(reader, 1, out bit);
                                    if (error != JxrError.None) return error;
                                    if (bit != 0) chromaCode = 4;
                                }
                                chromaCode <<= block == 0 ? 0 : block == 1 ? 1 : block == 2 ? 4 : 5;
                                if (c == 0) cu |= chromaCode;
                                else cv |= chromaCode;
                            }
                    }
                }
                error = state.MacroblockCbp.SetDifferential(channel, cy);
                if (error != JxrError.None) return error;
                if (format.ColorFormat == JxrCodecColorFormat.Yuv420 ||
                    format.ColorFormat == JxrCodecColorFormat.Yuv422 ||
                    format.ColorFormat == JxrCodecColorFormat.Yuv444)
                {
                    error = state.MacroblockCbp.SetDifferential(1, cu);
                    if (error != JxrError.None) return error;
                    error = state.MacroblockCbp.SetDifferential(2, cv);
                    if (error != JxrError.None) return error;
                }
            }
            return JxrError.None;
        }

        private static JxrError DecodePattern(int symbol, JxrBitReader reader, out int pattern)
        {
            int bit, value;
            JxrError error;
            pattern = symbol;
            switch (symbol)
            {
                case 2:
                    error = JxrEntropySyntax.Read(reader, 2, out value);
                    if (error != JxrError.None) return error;
                    if (value < 2) pattern = value == 0 ? 3 : 5;
                    else
                    {
                        error = JxrEntropySyntax.Read(reader, 1, out bit);
                        if (error != JxrError.None) return error;
                        pattern = new int[] { 6,9,10,12 }[value * 2 + bit - 4];
                    }
                    break;
                case 1:
                case 3:
                    error = JxrEntropySyntax.Read(reader, 2, out bit);
                    if (error != JxrError.None) return error;
                    pattern = symbol == 1 ? 1 << bit : 15 ^ (1 << bit);
                    break;
                case 4: pattern = 15; break;
            }
            return JxrError.None;
        }

        private static JxrError DecodePattern444(int symbol, JxrBitReader reader, out int pattern)
        {
            if (symbol == 0) return DecodePattern(1, reader, out pattern);
            if (symbol == 1) return DecodePattern(2, reader, out pattern);
            if (symbol == 2) return DecodePattern(3, reader, out pattern);
            return DecodePattern(4, reader, out pattern);
        }

        private static JxrError DecodeCoefficients(JxrCodecState state)
        {
            JxrCodecConfiguration format = state.Configuration;
            JxrCodecColorFormat color = format.ColorFormat;
            int planes = color == JxrCodecColorFormat.Yuv420 ||
                color == JxrCodecColorFormat.Yuv422 ? 1 : format.ChannelCount;
            int blocks = color == JxrCodecColorFormat.Yuv420 ? 6 :
                color == JxrCodecColorFormat.Yuv422 ? 8 : 4;
            JxrAdaptiveScan scan = state.Macroblock.Orientation == 1 ?
                state.Entropy.VerticalScan : state.Entropy.HorizontalScan;
            int cbpY, cbpU, cbpV, modelState, modelBits;
            JxrError error = state.MacroblockCbp.GetCbp(0, out cbpY);
            if (error != JxrError.None) return error;
            error = state.MacroblockCbp.GetCbp(1, out cbpU);
            if (error != JxrError.None) return error;
            error = state.MacroblockCbp.GetCbp(2, out cbpV);
            if (error != JxrError.None) return error;
            if (color == JxrCodecColorFormat.Yuv420)
                cbpY = unchecked(cbpY + (cbpU << 16) + (cbpV << 20));
            else if (color == JxrCodecColorFormat.Yuv422)
                cbpY = unchecked(cbpY + (cbpU << 16) + (cbpV << 24));
            error = state.Entropy.AcModel.Get(0, out modelState, out modelBits);
            if (error != JxrError.None) return error;
            int[] means = { 0, 0 };
            bool chroma = false;
            for (int plane = 0; plane < planes; plane++)
            {
                int blockIndex = 0;
                for (int block = 0; block < blocks; block++)
                {
                    int qpChannel = planes > 1 ? plane :
                        block > 3 ? color == JxrCodecColorFormat.Yuv420 ?
                            block - 3 : block / 2 - 1 : 0;
                    int quantizer;
                    error = format.GetHighpassQuantizer(qpChannel,
                        state.Macroblock.HighpassQuantizerIndex, out quantizer);
                    if (error != JxrError.None) return error;
                    if (format.Transcode) quantizer = 1;
                    for (int subblock = 0; subblock < 4; subblock++, blockIndex++, cbpY >>= 1)
                    {
                        int addressPlane = plane;
                        int offset = blockOffsets[blockIndex & 15];
                        if (block >= 4)
                        {
                            if (color == JxrCodecColorFormat.Yuv420)
                            { addressPlane = block - 3; offset = uvOffsets[subblock]; }
                            else
                            { addressPlane = 1 + (1 & (block >> 1));
                              offset = (block & 1) * 32 + uv422Offsets[subblock]; }
                        }
                        JxrCoefficientBuffer coefficients;
                        error = state.CoefficientPlanes.GetBlock(addressPlane, offset,
                            16, out coefficients);
                        if (error != JxrError.None) return error;
                        int count;
                        error = DecodeBlock(state, coefficients, scan, chroma,
                            (cbpY & 1) != 0, modelBits, quantizer, out count);
                        if (error != JxrError.None) return error;
                        means[chroma ? 1 : 0] += count;
                    }
                    if (block == 3)
                    {
                        error = state.Entropy.AcModel.Get(1, out modelState, out modelBits);
                        if (error != JxrError.None) return error;
                        chroma = true;
                    }
                }
                error = state.MacroblockCbp.GetCbp((plane + 1) & 15, out cbpY);
                if (error != JxrError.None) return error;
            }
            return state.Entropy.AcModel.UpdateForMacroblock(color,
                format.ChannelCount, means);
        }

        private static JxrError DecodeBlock(JxrCodecState state,
            JxrCoefficientBuffer coefficients, JxrAdaptiveScan scan,
            bool chroma, bool hasCoefficients, int modelBits, int quantizer,
            out int nonzeroCount)
        {
            nonzeroCount = 0;
            int flexbitCount = modelBits - state.Entropy.TrimFlexBits;
            if (flexbitCount < 0 || state.Configuration.SkipFlexbits) flexbitCount = 0;
            JxrError error;
            if (hasCoefficients)
            {
                int position = 1, symbol, sign, level, run;
                int baseIndex = 13 + (chroma ? 3 : 0);
                error = state.Huffman.Get(baseIndex).DecodeSymbol(state.HpReader, out symbol);
                if (error != JxrError.None) return error;
                int significant = symbol & 1;
                int remaining = symbol >> 2;
                int context = significant & remaining;
                error = JxrEntropySyntax.Read(state.HpReader, 1, out sign);
                if (error != JxrError.None) return error;
                level = sign != 0 ? -1 : 1;
                level = unchecked(level * (quantizer << modelBits));
                if ((symbol & 2) != 0)
                {
                    int magnitude;
                    error = JxrEntropySyntax.DecodeLevel(state.Huffman.Get(19 + context),
                        state.HpReader, out magnitude);
                    if (error != JxrError.None) return error;
                    level = unchecked(level * magnitude);
                }
                if (significant == 0)
                {
                    error = JxrEntropySyntax.DecodeRun(15 - position,
                        state.Huffman.Get(0), state.HpReader, out run);
                    if (error != JxrError.None) return error;
                    position += run;
                }
                position &= 15;
                uint coefficientIndex;
                error = scan.GetCoefficientIndex(position, out coefficientIndex);
                if (error != JxrError.None) return error;
                error = coefficients.Set((int)coefficientIndex, level);
                if (error != JxrError.None) return error;
                error = scan.ObserveNonZero(position);
                if (error != JxrError.None) return error;
                position = (position + 1) & 15;
                nonzeroCount = 1;
                while (remaining != 0)
                {
                    significant = remaining & 1;
                    if (significant == 0)
                    {
                        error = JxrEntropySyntax.DecodeRun(15 - position,
                            state.Huffman.Get(0), state.HpReader, out run);
                        if (error != JxrError.None) return error;
                        position += run;
                        if (position >= 16) { nonzeroCount = 16; break; }
                    }
                    error = JxrEntropySyntax.DecodeNextSymbol(position + 1,
                        state.Huffman.Get(baseIndex + context + 1),
                        state.HpReader, out symbol);
                    if (error != JxrError.None) return error;
                    remaining = symbol >> 1;
                    if (remaining < 0 || remaining >= 3) return JxrError.InvalidBitstream;
                    context &= remaining;
                    error = JxrEntropySyntax.Read(state.HpReader, 1, out sign);
                    if (error != JxrError.None) return error;
                    level = unchecked((sign != 0 ? -1 : 1) * (quantizer << modelBits));
                    if ((symbol & 1) != 0)
                    {
                        int magnitude;
                        error = JxrEntropySyntax.DecodeLevel(state.Huffman.Get(19 + context),
                            state.HpReader, out magnitude);
                        if (error != JxrError.None) return error;
                        level = unchecked(level * magnitude);
                    }
                    error = scan.GetCoefficientIndex(position, out coefficientIndex);
                    if (error != JxrError.None) return error;
                    error = coefficients.Set((int)coefficientIndex, level);
                    if (error != JxrError.None) return error;
                    error = scan.ObserveNonZero(position);
                    if (error != JxrError.None) return error;
                    position = (position + 1) & 15;
                    nonzeroCount++;
                }
            }
            if (flexbitCount != 0)
            {
                int multiplier = quantizer + state.Entropy.TrimFlexBits == 1 ? 1 :
                    quantizer << state.Entropy.TrimFlexBits;
                for (int index = 1; index < 16; index++)
                {
                    int coefficientIndex = coefficientOrder[index];
                    int coefficient, fine;
                    error = coefficients.Get(coefficientIndex, out coefficient);
                    if (error != JxrError.None) return error;
                    if (coefficient == 0)
                    {
                        error = JxrEntropySyntax.ReadSignedResidual(state.FlexReader,
                            flexbitCount, out fine);
                        if (error != JxrError.None) return error;
                        coefficient = unchecked(multiplier * fine);
                    }
                    else
                    {
                        error = JxrEntropySyntax.Read(state.FlexReader,
                            flexbitCount, out fine);
                        if (error != JxrError.None) return error;
                        coefficient = unchecked(coefficient +
                            multiplier * (coefficient < 0 ? -fine : fine));
                    }
                    error = coefficients.Set(coefficientIndex, coefficient);
                    if (error != JxrError.None) return error;
                }
            }
            return JxrError.None;
        }
    }
}
