namespace Jxr.Managed.Core
{
    // Managed counterpart of JxrLpDecoderDecodeSubband.  Refill is performed
    // by the caller, which supplies a positioned lowpass reader.
    public static class JxrLpCodec
    {
        private static readonly int[] chromaRemap = { 4, 1, 2, 3, 5, 6, 7 };

        public static JxrError Decode(JxrCodecState state)
        {
            if (state == null) return JxrError.InvalidArgument;
            JxrCodecConfiguration format = state.Configuration;
            JxrCodecColorFormat color = format.ColorFormat;
            JxrBitReader reader = state.LpReader;
            JxrMacroblockState macroblock = state.Macroblock;
            JxrAdaptiveScan scan = state.Entropy.LowpassScan;
            int fullPlanes = color == JxrCodecColorFormat.Yuv420 ||
                color == JxrCodecColorFormat.Yuv422 ? 2 : format.ChannelCount;
            int[] means = { 0, 0 };
            int[] pairs = new int[32];
            int modelState, modelBits, cbp = 0, bit;
            byte index;
            JxrError error = state.Entropy.LpModel.Get(0, out modelState, out modelBits);
            if (error != JxrError.None) return error;
            if (!format.Spatial && format.LowpassQuantizerBits > 0)
            {
                error = JxrEntropySyntax.ReadQuantizerIndex(reader,
                    format.LowpassQuantizerBits, out index);
                if (error != JxrError.None) return error;
                macroblock.SetLowpassQuantizerIndex(index);
            }
            if (state.ResetScan)
            {
                error = scan.ResetTotals(16);
                if (error != JxrError.None) return error;
            }

            if (color == JxrCodecColorFormat.Yuv420 ||
                color == JxrCodecColorFormat.Yuv422 ||
                color == JxrCodecColorFormat.Yuv444)
            {
                JxrLowpassCbpState cbpState = state.Entropy.LowpassCbp;
                int maximum = fullPlanes * 4 - 5;
                if (cbpState.ZeroCount <= 0 || cbpState.MaxCount < 0)
                {
                    error = JxrEntropySyntax.Read(reader, 1, out bit);
                    if (error != JxrError.None) return error;
                    if (bit != 0)
                    {
                        cbp = 1;
                        error = JxrEntropySyntax.Read(reader, fullPlanes - 1, out bit);
                        if (error != JxrError.None) return error;
                        if (bit != 0)
                        {
                            cbp = bit * 2;
                            error = JxrEntropySyntax.Read(reader, 1, out bit);
                            if (error != JxrError.None) return error;
                            cbp += bit;
                        }
                    }
                    if (cbpState.MaxCount < cbpState.ZeroCount) cbp = maximum - cbp;
                }
                else
                {
                    error = JxrEntropySyntax.Read(reader, fullPlanes, out cbp);
                    if (error != JxrError.None) return error;
                }
                cbpState.Observe(cbp, maximum);
            }
            else
            {
                for (int channel = 0; channel < format.ChannelCount; channel++)
                {
                    error = JxrEntropySyntax.Read(reader, 1, out bit);
                    if (error != JxrError.None) return error;
                    cbp |= bit << channel;
                }
            }

            for (int channel = 0; channel < fullPlanes; channel++)
            {
                bool subsampledChroma = channel == 1 &&
                    (color == JxrCodecColorFormat.Yuv420 || color == JxrCodecColorFormat.Yuv422);
                if ((cbp & 1) != 0)
                {
                    int nonzero;
                    int start = 1 + (color == JxrCodecColorFormat.Yuv420 && channel == 1 ? 9 : 0) +
                        (color == JxrCodecColorFormat.Yuv422 && channel == 1 ? 1 : 0);
                    error = JxrEntropySyntax.DecodeLowpassBlock(channel > 0,
                        pairs, state.Huffman, reader, start, out nonzero);
                    if (error != JxrError.None) return error;
                    means[channel == 0 ? 0 : 1] += nonzero;
                    int position = subsampledChroma ? 0 : 1;
                    if (subsampledChroma)
                    {
                        int[] temporary = new int[16];
                        for (int k = 0; k < nonzero; k++)
                        {
                            position += pairs[2 * k];
                            temporary[position & 15] = pairs[2 * k + 1];
                            position++;
                        }
                        int count = color == JxrCodecColorFormat.Yuv420 ? 6 : 14;
                        int remapOffset = color == JxrCodecColorFormat.Yuv420 ? 1 : 0;
                        for (int k = 0; k < count; k++)
                        {
                            error = macroblock.SetDcCoefficient((k & 1) + 1,
                                chromaRemap[remapOffset + (k >> 1)], temporary[k]);
                            if (error != JxrError.None) return error;
                        }
                    }
                    else
                    {
                        for (int k = 0; k < nonzero; k++)
                        {
                            position += pairs[2 * k];
                            uint coefficientIndex;
                            error = scan.GetCoefficientIndex(position, out coefficientIndex);
                            if (error != JxrError.None) return error;
                            error = macroblock.SetDcCoefficient(channel,
                                (int)coefficientIndex, pairs[2 * k + 1]);
                            if (error != JxrError.None) return error;
                            error = scan.ObserveNonZero(position);
                            if (error != JxrError.None) return error;
                            position++;
                        }
                    }
                }

                if (modelBits != 0)
                {
                    if (subsampledChroma)
                    {
                        int count = color == JxrCodecColorFormat.Yuv420 ? 4 : 8;
                        for (int k = 1; k < count; k++)
                        {
                            for (int chroma = 1; chroma <= 2; chroma++)
                            {
                                int coefficient, residual;
                                error = macroblock.GetDcCoefficient(chroma, k, out coefficient);
                                if (error != JxrError.None) return error;
                                error = JxrEntropySyntax.Read(reader, modelBits, out residual);
                                if (error != JxrError.None) return error;
                                if (coefficient != 0)
                                    coefficient = JxrEntropySyntax.CombineSignedMagnitude(
                                        coefficient, unchecked((uint)residual), modelBits);
                                else
                                {
                                    coefficient = residual;
                                    if (coefficient != 0)
                                    {
                                        error = JxrEntropySyntax.Read(reader, 1, out bit);
                                        if (error != JxrError.None) return error;
                                        if (bit != 0) coefficient = -coefficient;
                                    }
                                }
                                error = macroblock.SetDcCoefficient(chroma, k, coefficient);
                                if (error != JxrError.None) return error;
                            }
                        }
                    }
                    else
                    {
                        for (int k = 1; k < 16; k++)
                        {
                            int coefficient, residual;
                            error = macroblock.GetDcCoefficient(channel, k, out coefficient);
                            if (error != JxrError.None) return error;
                            if (coefficient == 0)
                                error = JxrEntropySyntax.ReadSignedResidual(reader, modelBits, out coefficient);
                            else
                            {
                                error = JxrEntropySyntax.Read(reader, modelBits, out residual);
                                if (error == JxrError.None)
                                    coefficient = JxrEntropySyntax.CombineNonZero(
                                        coefficient, unchecked((uint)residual), modelBits);
                            }
                            if (error != JxrError.None) return error;
                            error = macroblock.SetDcCoefficient(channel, k, coefficient);
                            if (error != JxrError.None) return error;
                        }
                    }
                }
                error = state.Entropy.LpModel.Get(1, out modelState, out modelBits);
                if (error != JxrError.None) return error;
                cbp >>= 1;
            }
            error = state.Entropy.LpModel.UpdateForMacroblock(color,
                format.ChannelCount, means);
            if (error != JxrError.None) return error;
            if (state.ResetContext)
                for (int k = 0; k < 13; k++)
                {
                    error = state.Huffman.Adapt(k);
                    if (error != JxrError.None) return error;
                }
            return JxrError.None;
        }
    }
}
