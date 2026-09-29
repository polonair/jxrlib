namespace Jxr.Managed.Core
{
    // Managed counterpart of JxrDcDecoderDecodeSubband.  Packet refill is
    // supplied by the caller through JxrBitReader before this method runs.
    public static class JxrDcCodec
    {
        public static JxrError Decode(JxrCodecState state)
        {
            int channel, bits, flcState, level, sign, flag, symbol;
            int[] laplacianMean = { 0, 0 };
            JxrError error;
            byte index;
            JxrCodecConfiguration format;
            JxrMacroblockState macroblock;
            JxrBitReader reader;
            if (state == null) return JxrError.InvalidArgument;
            format = state.Configuration;
            macroblock = state.Macroblock;
            reader = state.DcReader;
            error = macroblock.ClearDc(format.ChannelCount);
            if (error != JxrError.None) return error;
            macroblock.ResetQuantizerIndices();
            if (format.Spatial && !format.DcOnly)
            {
                if (format.LowpassQuantizerBits > 0)
                {
                    error = JxrEntropySyntax.ReadQuantizerIndex(reader,
                        format.LowpassQuantizerBits, out index);
                    if (error != JxrError.None) return error;
                    macroblock.SetLowpassQuantizerIndex(index);
                }
                if (format.HasHighpass && format.HighpassQuantizerBits > 0)
                {
                    error = JxrEntropySyntax.ReadQuantizerIndex(reader,
                        format.HighpassQuantizerBits, out index);
                    if (error != JxrError.None) return error;
                    macroblock.SetHighpassQuantizerIndex(index);
                }
            }
            if (format.HighpassQuantizerBits == 0 && format.HighpassQuantizerCount > 1)
                macroblock.SetHighpassQuantizerIndex(macroblock.LowpassQuantizerIndex);
            if (macroblock.LowpassQuantizerIndex >= format.LowpassQuantizerCount ||
                macroblock.HighpassQuantizerIndex >= format.HighpassQuantizerCount)
                return JxrError.InvalidBitstream;

            error = state.Entropy.DcModel.Get(0, out flcState, out bits);
            if (error != JxrError.None) return error;
            if (format.ColorFormat == JxrCodecColorFormat.YOnly ||
                format.ColorFormat == JxrCodecColorFormat.Cmyk ||
                format.ColorFormat == JxrCodecColorFormat.NComponent)
            {
                for (channel = 0; channel < format.ChannelCount; channel++)
                {
                    int value = 0;
                    error = JxrEntropySyntax.Read(reader, 1, out flag);
                    if (error != JxrError.None) return error;
                    if (flag != 0)
                    {
                        error = JxrEntropySyntax.DecodeLevel(state.Huffman.Get(3),
                            reader, out level);
                        if (error != JxrError.None) return error;
                        value = level - 1;
                        laplacianMean[channel == 0 ? 0 : 1]++;
                    }
                    if (bits != 0)
                    {
                        error = JxrEntropySyntax.Read(reader, bits, out level);
                        if (error != JxrError.None) return error;
                        value = unchecked((value << bits) | level);
                    }
                    if (value != 0)
                    {
                        error = JxrEntropySyntax.Read(reader, 1, out sign);
                        if (error != JxrError.None) return error;
                        if (sign != 0) value = unchecked(-value);
                    }
                    macroblock.SetDcCoefficient(channel, 0, value);
                    error = state.Entropy.DcModel.Get(1, out flcState, out bits);
                    if (error != JxrError.None) return error;
                }
            }
            else
            {
                int[] values = new int[3];
                JxrAdaptiveHuffman significantFlags = state.Huffman.Get(2);
                error = JxrHuffmanDecoder.DecodeSymbol(significantFlags.DecoderTable,
                    reader, out symbol);
                if (error != JxrError.None) return error;
                values[0] = symbol >> 2;
                values[1] = (symbol >> 1) & 1;
                values[2] = symbol & 1;
                for (channel = 0; channel < 3; channel++)
                {
                    int value = values[channel];
                    if (channel > 0)
                    {
                        error = state.Entropy.DcModel.Get(1, out flcState, out bits);
                        if (error != JxrError.None) return error;
                    }
                    if (value != 0)
                    {
                        error = JxrEntropySyntax.DecodeLevel(state.Huffman.Get(channel == 0 ? 3 : 4),
                            reader, out level);
                        if (error != JxrError.None) return error;
                        value = level - 1;
                        laplacianMean[channel == 0 ? 0 : 1]++;
                    }
                    if (bits != 0)
                    {
                        error = JxrEntropySyntax.Read(reader, bits, out level);
                        if (error != JxrError.None) return error;
                        value = unchecked((value << bits) | level);
                    }
                    if (value != 0)
                    {
                        error = JxrEntropySyntax.Read(reader, 1, out sign);
                        if (error != JxrError.None) return error;
                        if (sign != 0) value = unchecked(-value);
                    }
                    macroblock.SetDcCoefficient(channel, 0, value);
                }
            }
            error = state.Entropy.DcModel.UpdateForMacroblock(format.ColorFormat,
                format.ChannelCount, laplacianMean);
            if (error != JxrError.None) return error;
            if (format.AdaptDcHuffman && state.ResetContext)
            {
                for (channel = 2; channel < 5; channel++)
                {
                    error = state.Huffman.Adapt(channel);
                    if (error != JxrError.None) return error;
                }
            }
            return JxrError.None;
        }
    }
}
