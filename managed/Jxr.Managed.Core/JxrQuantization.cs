using System;

namespace Jxr.Managed.Core
{
    // Value form of CWMIQuantizer. Multiplier holds the unsigned bit pattern
    // of native iMan; no floating-point approximation participates in coding.
    public sealed class JxrQuantizer
    {
        private readonly byte index;
        private readonly int parameter;
        private readonly int offset;
        private readonly uint multiplier;
        private readonly int exponent;

        internal JxrQuantizer(byte index, int parameter, int offset,
            uint multiplier, int exponent)
        {
            this.index = index;
            this.parameter = parameter;
            this.offset = offset;
            this.multiplier = multiplier;
            this.exponent = exponent;
        }

        public byte Index { get { return index; } }
        public int Parameter { get { return parameter; } }
        public int Offset { get { return offset; } }
        public uint Multiplier { get { return multiplier; } }
        public int Exponent { get { return exponent; } }

        // JxrEncoderQuantizerInitializer replaces the DC rounding offset.
        public JxrQuantizer WithDcOffset()
        {
            return new JxrQuantizer(index, parameter, parameter >> 1,
                multiplier, exponent);
        }
    }

    // Per-channel DC and per-channel/per-index LP/HP quantizers.  Each array
    // has an explicit owner instead of native channel-strided pointer tables.
    public sealed class JxrQuantizerSet
    {
        private readonly JxrQuantizer[] dc;
        private readonly JxrQuantizer[][] lp;
        private readonly JxrQuantizer[][] hp;

        public JxrQuantizerSet(JxrQuantizer[] dc,
            JxrQuantizer[][] lp, JxrQuantizer[][] hp)
        {
            if (dc == null || lp == null || hp == null ||
                dc.Length == 0 || dc.Length != lp.Length || dc.Length != hp.Length)
                throw new ArgumentException("Invalid quantizer channel arrays.");
            this.dc = (JxrQuantizer[])dc.Clone();
            this.lp = new JxrQuantizer[dc.Length][];
            this.hp = new JxrQuantizer[dc.Length][];
            for (int channel = 0; channel < dc.Length; channel++)
            {
                if (dc[channel] == null || lp[channel] == null || hp[channel] == null ||
                    lp[channel].Length == 0 || hp[channel].Length == 0)
                    throw new ArgumentException("A quantizer channel is empty.");
                this.lp[channel] = (JxrQuantizer[])lp[channel].Clone();
                this.hp[channel] = (JxrQuantizer[])hp[channel].Clone();
                for (int index = 0; index < this.lp[channel].Length; index++)
                    if (this.lp[channel][index] == null)
                        throw new ArgumentException("A lowpass quantizer is null.");
                for (int index = 0; index < this.hp[channel].Length; index++)
                    if (this.hp[channel][index] == null)
                        throw new ArgumentException("A highpass quantizer is null.");
            }
        }

        public int ChannelCount { get { return dc.Length; } }

        public JxrError GetDc(int channel, out JxrQuantizer quantizer)
        {
            quantizer = null;
            if (channel < 0 || channel >= dc.Length) return JxrError.InvalidArgument;
            quantizer = dc[channel];
            return JxrError.None;
        }

        public JxrError GetLp(int channel, int index, out JxrQuantizer quantizer)
        {
            quantizer = null;
            if (channel < 0 || channel >= lp.Length || index < 0 ||
                index >= lp[channel].Length) return JxrError.InvalidArgument;
            quantizer = lp[channel][index];
            return JxrError.None;
        }

        public JxrError GetHp(int channel, int index, out JxrQuantizer quantizer)
        {
            quantizer = null;
            if (channel < 0 || channel >= hp.Length || index < 0 ||
                index >= hp[channel].Length) return JxrError.InvalidArgument;
            quantizer = hp[channel][index];
            return JxrError.None;
        }
    }

    // remapQP, JxrEncoderQuantizationPipeline and JxrDecoderDequantizer in
    // one explicit managed module. All arithmetic follows 32-bit C semantics.
    public static class JxrQuantization
    {
        private static readonly uint[] reciprocalMantissas = {
            0U, 0U, 0U, 0xaaaaaaabU, 0U, 0xcccccccdU,
            0xaaaaaaabU, 0x92492493U, 0U, 0xe38e38e4U,
            0xcccccccdU, 0xba2e8ba3U, 0xaaaaaaabU, 0x9d89d89eU,
            0x92492493U, 0x88888889U, 0U, 0xf0f0f0f1U,
            0xe38e38e4U, 0xd79435e6U, 0xcccccccdU, 0xc30c30c4U,
            0xba2e8ba3U, 0xb21642c9U, 0xaaaaaaabU, 0xa3d70a3eU,
            0x9d89d89eU, 0x97b425eeU, 0x92492493U, 0x8d3dcb09U,
            0x88888889U, 0x84210843U
        };
        private static readonly int[] reciprocalExponents = {
            0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
            4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
        };
        private static readonly int[] fullBlockOffsets = {
            0,64,16,80,128,192,144,208,32,96,48,112,160,224,176,240
        };
        private static readonly int[] chroma420Offsets = { 0,32,16,48 };
        private static readonly int[] chroma422Offsets = { 0,64,16,80,32,96,48,112 };
        private static readonly int[] dcFullOffsets = {
            0,128,64,208,32,240,48,224,16,192,80,144,112,176,96,160
        };

        public static JxrQuantizer Remap(byte index, bool scaledArithmetic,
            bool shiftedChroma)
        {
            if (index == 0) return new JxrQuantizer(0, 1, 0, 0, 0);
            int mantissa, exponent;
            if (scaledArithmetic)
            {
                int shift = shiftedChroma ? 0 : 1;
                if (index < 16) { mantissa = index; exponent = shift; }
                else
                {
                    mantissa = 16 + (index & 15);
                    exponent = (index >> 4) - 1 + shift;
                }
            }
            else
            {
                if (index < 32)
                { mantissa = (index + 3) >> 2; exponent = 0; }
                else if (index < 48)
                {
                    mantissa = (17 + (index & 15)) >> 1;
                    exponent = (index >> 4) - 2;
                }
                else
                {
                    mantissa = 16 + (index & 15);
                    exponent = (index >> 4) - 3;
                }
            }
            int parameter = unchecked(mantissa << exponent);
            int offset = unchecked((parameter * 3 + 1) >> 3);
            return new JxrQuantizer(index, parameter, offset,
                reciprocalMantissas[mantissa],
                reciprocalExponents[mantissa] + exponent);
        }

        // Native formatQuantizer channel modes: 0 uniform, 1 mixed, 2/3 independent.
        public static JxrError RemapChannels(byte[] indices, int channelMode,
            bool scaledArithmetic, bool shiftedChroma, bool dcBand,
            out JxrQuantizer[] result)
        {
            result = null;
            if (indices == null || indices.Length < 1 || indices.Length > 16 ||
                channelMode < 0 || channelMode > 3) return JxrError.InvalidArgument;
            JxrQuantizer[] quantizers = new JxrQuantizer[indices.Length];
            for (int channel = 0; channel < indices.Length; channel++)
            {
                int source = channel;
                if (channel > 0 && channelMode == 0) source = 0;
                else if (channel > 0 && channelMode == 1) source = 1;
                quantizers[channel] = Remap(indices[source], scaledArithmetic,
                    shiftedChroma && channel > 0);
                if (dcBand) quantizers[channel] = quantizers[channel].WithDcOffset();
            }
            result = quantizers;
            return JxrError.None;
        }

        public static int QuantizeCoefficient(int value, JxrQuantizer quantizer)
        {
            if (quantizer == null) throw new ArgumentNullException("quantizer");
            int signMask = value >> 31;
            int magnitudeWithOffset = unchecked(((value ^ signMask) - signMask) +
                quantizer.Offset);
            int positive = quantizer.Multiplier == 0 ?
                magnitudeWithOffset >> quantizer.Exponent :
                unchecked((int)(uint)(((ulong)(uint)magnitudeWithOffset *
                    quantizer.Multiplier >> 32) >> quantizer.Exponent));
            return unchecked((positive ^ signMask) - signMask);
        }

        public static int DequantizeCoefficient(int value, JxrQuantizer quantizer)
        {
            if (quantizer == null) throw new ArgumentNullException("quantizer");
            return unchecked(value * quantizer.Parameter);
        }

        // Native JxrEncoderQuantizationPipelineQuantize. Transcode bypasses
        // coefficient quantization but still imports the DC/LP block values.
        public static JxrError QuantizeMacroblock(JxrCoefficientPlaneState planes,
            JxrMacroblockState macroblock, JxrQuantizerSet quantizers,
            JxrCodecColorFormat color, int channelCount, bool dcOnly,
            bool noHighpass, bool transcode)
        {
            if (!ValidInputs(planes, macroblock, quantizers, channelCount,
                !transcode))
                return JxrError.InvalidArgument;
            for (int channel = 0; channel < channelCount; channel++)
            {
                int[] coefficients;
                JxrError error = planes.GetPlane(channel, out coefficients);
                if (error != JxrError.None) return error;
                bool chroma = channel > 0 && IsYuv(color);
                int[] offsets = chroma && color == JxrCodecColorFormat.Yuv420 ?
                    chroma420Offsets : chroma && color == JxrCodecColorFormat.Yuv422 ?
                    chroma422Offsets : fullBlockOffsets;
                if (!transcode)
                {
                    JxrQuantizer dc, lp, hp;
                    error = quantizers.GetDc(channel, out dc);
                    if (error != JxrError.None) return error;
                    error = quantizers.GetLp(channel,
                        macroblock.LowpassQuantizerIndex, out lp);
                    if (error != JxrError.None) return error;
                    error = quantizers.GetHp(channel,
                        macroblock.HighpassQuantizerIndex, out hp);
                    if (error != JxrError.None) return error;
                    for (int block = 0; block < offsets.Length; block++)
                    {
                        int offset = offsets[block];
                        if (block == 0)
                            coefficients[offset] = QuantizeCoefficient(coefficients[offset], dc);
                        else if (!dcOnly)
                            coefficients[offset] = QuantizeCoefficient(coefficients[offset], lp);
                        if (!dcOnly && !noHighpass)
                            for (int k = 1; k < 16; k++)
                                coefficients[offset + k] =
                                    QuantizeCoefficient(coefficients[offset + k], hp);
                    }
                }
                int[] dcOffsets = chroma && color == JxrCodecColorFormat.Yuv420 ?
                    chroma420Offsets : chroma && color == JxrCodecColorFormat.Yuv422 ?
                    chroma422Offsets : dcFullOffsets;
                for (int block = 0; block < dcOffsets.Length; block++)
                {
                    error = macroblock.SetDcCoefficient(channel, block,
                        coefficients[dcOffsets[block]]);
                    if (error != JxrError.None) return error;
                }
            }
            return JxrError.None;
        }

        // Native JxrDecoderDequantizerDequantizeMacroblock. Caller supplies
        // the current tile's resolved quantizers and decoded macroblock DCs.
        public static JxrError DequantizeMacroblock(JxrCoefficientPlaneState planes,
            JxrMacroblockState macroblock, JxrQuantizerSet quantizers,
            JxrCodecColorFormat color, int channelCount, bool dcOnly)
        {
            if (!ValidInputs(planes, macroblock, quantizers, channelCount, true))
                return JxrError.InvalidArgument;
            for (int channel = 0; channel < channelCount; channel++)
            {
                int[] coefficients;
                int value;
                JxrQuantizer dc, lp;
                JxrError error = planes.GetPlane(channel, out coefficients);
                if (error != JxrError.None) return error;
                error = quantizers.GetDc(channel, out dc);
                if (error != JxrError.None) return error;
                error = macroblock.GetDcCoefficient(channel, 0, out value);
                if (error != JxrError.None) return error;
                coefficients[0] = DequantizeCoefficient(value, dc);
                if (dcOnly) continue;
                error = quantizers.GetLp(channel,
                    macroblock.LowpassQuantizerIndex, out lp);
                if (error != JxrError.None) return error;
                int[] offsets = channel > 0 && color == JxrCodecColorFormat.Yuv420 ?
                    chroma420Offsets : channel > 0 && color == JxrCodecColorFormat.Yuv422 ?
                    chroma422Offsets : dcFullOffsets;
                for (int block = 1; block < offsets.Length; block++)
                {
                    error = macroblock.GetDcCoefficient(channel, block, out value);
                    if (error != JxrError.None) return error;
                    coefficients[offsets[block]] = DequantizeCoefficient(value, lp);
                }
            }
            return JxrError.None;
        }

        private static bool ValidInputs(JxrCoefficientPlaneState planes,
            JxrMacroblockState macroblock, JxrQuantizerSet quantizers,
            int channelCount, bool requireQuantizers)
        {
            return planes != null && macroblock != null &&
                channelCount > 0 && channelCount <= planes.PlaneCount &&
                channelCount <= macroblock.ChannelCapacity &&
                (!requireQuantizers || quantizers != null) &&
                (quantizers == null || channelCount <= quantizers.ChannelCount);
        }

        private static bool IsYuv(JxrCodecColorFormat color)
        {
            return color == JxrCodecColorFormat.Yuv420 ||
                color == JxrCodecColorFormat.Yuv422 ||
                color == JxrCodecColorFormat.Yuv444;
        }
    }
}
