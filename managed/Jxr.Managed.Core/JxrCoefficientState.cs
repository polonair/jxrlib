using System;

namespace Jxr.Managed.Core
{
    // Managed range over coefficients.  It replaces a PixelI* that is advanced
    // through a native block with one owned array and validated indexes.
    public sealed class JxrCoefficientBuffer
    {
        private readonly int[] values;
        private readonly int offset;
        private readonly int count;

        public JxrCoefficientBuffer(int[] values, int offset, int count)
        {
            if (values == null) throw new ArgumentNullException("values");
            if (offset < 0 || count < 0 || offset > values.Length - count)
                throw new ArgumentException("The coefficient range is outside its array.");
            this.values = values;
            this.offset = offset;
            this.count = count;
        }

        public int Count { get { return count; } }
        public int Offset { get { return offset; } }

        public JxrError Get(int index, out int value)
        {
            value = 0;
            if (index < 0 || index >= count) return JxrError.InvalidArgument;
            value = values[offset + index];
            return JxrError.None;
        }

        public JxrError Set(int index, int value)
        {
            if (index < 0 || index >= count) return JxrError.InvalidArgument;
            values[offset + index] = value;
            return JxrError.None;
        }

        public JxrError Add(int index, int value)
        {
            if (index < 0 || index >= count) return JxrError.InvalidArgument;
            values[offset + index] = unchecked(values[offset + index] + value);
            return JxrError.None;
        }

        public void Clear()
        {
            Array.Clear(values, offset, count);
        }
    }

    // This subset is all that coefficient storage needs from JPEG XR color
    // configuration.  Header parsing will map the full color-format enum here.
    public enum JxrCoefficientColorFormat
    {
        Other = 0,
        Yuv444,
        Yuv420,
        Yuv422
    }

    // Managed counterpart of JxrCoefficientPlaneState.  The plane arrays stay
    // with their caller; this object records their format-derived capacities.
    public sealed class JxrCoefficientPlaneState
    {
        private readonly int[][] planes;
        private readonly int[] lengths;
        private readonly int planeCount;

        public JxrCoefficientPlaneState(int[][] planes,
            JxrCoefficientColorFormat colorFormat, int channelCount)
        {
            int plane;
            if (planes == null) throw new ArgumentNullException("planes");
            if (channelCount < 0 || channelCount > planes.Length)
                throw new ArgumentException("Invalid channel count.");
            this.planes = new int[channelCount][];
            lengths = new int[channelCount];
            planeCount = channelCount;
            for (plane = 0; plane < channelCount; plane++)
            {
                if (planes[plane] == null) throw new ArgumentException("A coefficient plane is null.");
                this.planes[plane] = planes[plane];
                lengths[plane] = GetFormatLength(colorFormat, plane);
                if (planes[plane].Length < lengths[plane])
                    throw new ArgumentException("A coefficient plane is shorter than its format capacity.");
            }
        }

        public int PlaneCount { get { return planeCount; } }

        public JxrError GetPlane(int plane, out int[] values)
        {
            values = null;
            if (plane < 0 || plane >= planeCount) return JxrError.InvalidArgument;
            values = planes[plane];
            return JxrError.None;
        }

        public JxrError GetLength(int plane, out int length)
        {
            length = 0;
            if (plane < 0 || plane >= planeCount) return JxrError.InvalidArgument;
            length = lengths[plane];
            return JxrError.None;
        }

        public JxrError GetBlock(int plane, int offset, int count,
            out JxrCoefficientBuffer block)
        {
            block = null;
            if (plane < 0 || plane >= planeCount || offset < 0 || count < 0 ||
                offset > lengths[plane] - count) return JxrError.InvalidArgument;
            block = new JxrCoefficientBuffer(planes[plane], offset, count);
            return JxrError.None;
        }

        private static int GetFormatLength(JxrCoefficientColorFormat colorFormat, int plane)
        {
            if (plane == 0 || colorFormat == JxrCoefficientColorFormat.Other ||
                colorFormat == JxrCoefficientColorFormat.Yuv444) return 256;
            return colorFormat == JxrCoefficientColorFormat.Yuv420 ? 64 : 128;
        }
    }

    // Explicit import/export form of CWMIMBInfo fields used by entropy code.
    // It keeps native interop out of the managed port while preserving the
    // former LoadFromNative/CommitToNative boundary for conformance vectors.
    public sealed class JxrMacroblockSnapshot
    {
        private readonly int[] dcCoefficients;
        private byte lowpassQuantizerIndex;
        private byte highpassQuantizerIndex;
        private int orientation;

        public JxrMacroblockSnapshot(int channelCapacity)
        {
            if (channelCapacity < 0) throw new ArgumentOutOfRangeException("channelCapacity");
            dcCoefficients = new int[channelCapacity * JxrMacroblockState.CoefficientsPerChannel];
        }

        public int ChannelCapacity { get { return dcCoefficients.Length / JxrMacroblockState.CoefficientsPerChannel; } }
        public byte LowpassQuantizerIndex { get { return lowpassQuantizerIndex; } set { lowpassQuantizerIndex = value; } }
        public byte HighpassQuantizerIndex { get { return highpassQuantizerIndex; } set { highpassQuantizerIndex = value; } }
        public int Orientation { get { return orientation; } set { orientation = value; } }

        public JxrError GetDcCoefficient(int channel, int index, out int value)
        {
            value = 0;
            if (!IsValid(channel, index)) return JxrError.InvalidArgument;
            value = dcCoefficients[channel * JxrMacroblockState.CoefficientsPerChannel + index];
            return JxrError.None;
        }

        public JxrError SetDcCoefficient(int channel, int index, int value)
        {
            if (!IsValid(channel, index)) return JxrError.InvalidArgument;
            dcCoefficients[channel * JxrMacroblockState.CoefficientsPerChannel + index] = value;
            return JxrError.None;
        }

        internal int[] GetValues() { return dcCoefficients; }

        private bool IsValid(int channel, int index)
        {
            return channel >= 0 && channel < ChannelCapacity && index >= 0 &&
                index < JxrMacroblockState.CoefficientsPerChannel;
        }
    }

    // Explicit mutable DC and quantizer state for one macroblock.
    public sealed class JxrMacroblockState
    {
        public const int CoefficientsPerChannel = 16;
        private readonly int[] dcCoefficients;
        private byte lowpassQuantizerIndex;
        private byte highpassQuantizerIndex;
        private int orientation;

        public JxrMacroblockState(int channelCapacity)
        {
            if (channelCapacity < 0) throw new ArgumentOutOfRangeException("channelCapacity");
            dcCoefficients = new int[channelCapacity * CoefficientsPerChannel];
        }

        public int ChannelCapacity { get { return dcCoefficients.Length / CoefficientsPerChannel; } }
        public int Orientation { get { return orientation; } }
        public byte LowpassQuantizerIndex { get { return lowpassQuantizerIndex; } }
        public byte HighpassQuantizerIndex { get { return highpassQuantizerIndex; } }

        public JxrError LoadFrom(JxrMacroblockSnapshot snapshot)
        {
            if (snapshot == null || snapshot.ChannelCapacity != ChannelCapacity)
                return JxrError.InvalidArgument;
            Array.Copy(snapshot.GetValues(), dcCoefficients, dcCoefficients.Length);
            lowpassQuantizerIndex = snapshot.LowpassQuantizerIndex;
            highpassQuantizerIndex = snapshot.HighpassQuantizerIndex;
            orientation = snapshot.Orientation;
            return JxrError.None;
        }

        public JxrError CopyTo(JxrMacroblockSnapshot snapshot)
        {
            if (snapshot == null || snapshot.ChannelCapacity != ChannelCapacity)
                return JxrError.InvalidArgument;
            Array.Copy(dcCoefficients, snapshot.GetValues(), dcCoefficients.Length);
            snapshot.LowpassQuantizerIndex = lowpassQuantizerIndex;
            snapshot.HighpassQuantizerIndex = highpassQuantizerIndex;
            snapshot.Orientation = orientation;
            return JxrError.None;
        }

        public JxrError ClearDc(int channelCount)
        {
            int channel;
            if (channelCount < 0 || channelCount > ChannelCapacity) return JxrError.InvalidArgument;
            for (channel = 0; channel < channelCount; channel++)
                Array.Clear(dcCoefficients, channel * CoefficientsPerChannel, CoefficientsPerChannel);
            return JxrError.None;
        }

        public JxrError GetDcCoefficient(int channel, int index, out int value)
        {
            value = 0;
            if (!IsValid(channel, index)) return JxrError.InvalidArgument;
            value = dcCoefficients[channel * CoefficientsPerChannel + index];
            return JxrError.None;
        }

        public JxrError SetDcCoefficient(int channel, int index, int value)
        {
            if (!IsValid(channel, index)) return JxrError.InvalidArgument;
            dcCoefficients[channel * CoefficientsPerChannel + index] = value;
            return JxrError.None;
        }

        public void ResetQuantizerIndices()
        {
            lowpassQuantizerIndex = 0;
            highpassQuantizerIndex = 0;
        }

        public void SetLowpassQuantizerIndex(byte value) { lowpassQuantizerIndex = value; }
        public void SetHighpassQuantizerIndex(byte value) { highpassQuantizerIndex = value; }

        private bool IsValid(int channel, int index)
        {
            return channel >= 0 && channel < ChannelCapacity && index >= 0 &&
                index < CoefficientsPerChannel;
        }
    }

    // Explicit per-plane CBP values.  Arrays are copied at the compatibility
    // boundary so updates are never implicit aliases as they were in C.
    public sealed class JxrMacroblockCbpState
    {
        private readonly int[] cbpValues;
        private readonly int[] differentialValues;

        public JxrMacroblockCbpState(int channelCapacity)
        {
            if (channelCapacity < 0) throw new ArgumentOutOfRangeException("channelCapacity");
            cbpValues = new int[channelCapacity];
            differentialValues = new int[channelCapacity];
        }

        public int ChannelCapacity { get { return cbpValues.Length; } }

        public JxrError LoadFrom(int[] cbp, int[] differential)
        {
            if (cbp == null || differential == null || cbp.Length < ChannelCapacity ||
                differential.Length < ChannelCapacity) return JxrError.InvalidArgument;
            Array.Copy(cbp, cbpValues, ChannelCapacity);
            Array.Copy(differential, differentialValues, ChannelCapacity);
            return JxrError.None;
        }

        public JxrError CopyTo(int[] cbp, int[] differential)
        {
            if (cbp == null || differential == null || cbp.Length < ChannelCapacity ||
                differential.Length < ChannelCapacity) return JxrError.InvalidArgument;
            Array.Copy(cbpValues, cbp, ChannelCapacity);
            Array.Copy(differentialValues, differential, ChannelCapacity);
            return JxrError.None;
        }

        public JxrError GetCbp(int plane, out int value)
        {
            value = 0;
            if (!IsValidPlane(plane)) return JxrError.InvalidArgument;
            value = cbpValues[plane];
            return JxrError.None;
        }

        public JxrError SetCbp(int plane, int value)
        {
            if (!IsValidPlane(plane)) return JxrError.InvalidArgument;
            cbpValues[plane] = value;
            return JxrError.None;
        }

        public JxrError GetDifferential(int plane, out int value)
        {
            value = 0;
            if (!IsValidPlane(plane)) return JxrError.InvalidArgument;
            value = differentialValues[plane];
            return JxrError.None;
        }

        public JxrError SetDifferential(int plane, int value)
        {
            if (!IsValidPlane(plane)) return JxrError.InvalidArgument;
            differentialValues[plane] = value;
            return JxrError.None;
        }

        private bool IsValidPlane(int plane) { return plane >= 0 && plane < ChannelCapacity; }
    }

    // Managed value form of CCBPModel.  HP CBP prediction will populate these
    // three two-element arrays without a native struct or an addressable field.
    public sealed class JxrCbpPredictionModel
    {
        private readonly int[] zeroCounts = new int[2];
        private readonly int[] oneCounts = new int[2];
        private readonly int[] states = new int[2];

        public void Reset()
        {
            int context;
            for (context = 0; context < 2; context++)
            {
                zeroCounts[context] = -4;
                oneCounts[context] = 4;
                states[context] = 0;
            }
        }

        public JxrError Get(int context, out int zeroCount, out int oneCount, out int state)
        {
            zeroCount = 0; oneCount = 0; state = 0;
            if (context < 0 || context >= 2) return JxrError.InvalidArgument;
            zeroCount = zeroCounts[context]; oneCount = oneCounts[context]; state = states[context];
            return JxrError.None;
        }

        public JxrError Set(int context, int zeroCount, int oneCount, int state)
        {
            if (context < 0 || context >= 2) return JxrError.InvalidArgument;
            zeroCounts[context] = zeroCount; oneCounts[context] = oneCount; states[context] = state;
            return JxrError.None;
        }
    }

    // Groups the HP-CBP entropy and prediction dependencies formerly passed as
    // three pointers.  The objects remain explicit managed dependencies.
    public sealed class JxrHighpassCbpState
    {
        private readonly JxrAdaptiveHuffman patternHuffman;
        private readonly JxrAdaptiveHuffman countHuffman;
        private readonly JxrCbpPredictionModel predictionModel;

        public JxrHighpassCbpState(JxrAdaptiveHuffman patternHuffman,
            JxrAdaptiveHuffman countHuffman, JxrCbpPredictionModel predictionModel)
        {
            if (patternHuffman == null) throw new ArgumentNullException("patternHuffman");
            if (countHuffman == null) throw new ArgumentNullException("countHuffman");
            if (predictionModel == null) throw new ArgumentNullException("predictionModel");
            this.patternHuffman = patternHuffman;
            this.countHuffman = countHuffman;
            this.predictionModel = predictionModel;
        }

        public JxrAdaptiveHuffman PatternHuffman { get { return patternHuffman; } }
        public JxrAdaptiveHuffman CountHuffman { get { return countHuffman; } }
        public JxrCbpPredictionModel PredictionModel { get { return predictionModel; } }

        public JxrError Adapt()
        {
            JxrError error = patternHuffman.Adapt();
            if (error != JxrError.None) return error;
            return countHuffman.Adapt();
        }
    }

    // LP-CBP counters are self-owned numbers rather than pointers into a
    // CCodingContext.  Observe retains the exact native clamp rules.
    public sealed class JxrLowpassCbpState
    {
        private int zeroCount;
        private int maxCount;

        public JxrLowpassCbpState(int zeroCount, int maxCount)
        {
            this.zeroCount = zeroCount;
            this.maxCount = maxCount;
        }

        public int ZeroCount { get { return zeroCount; } }
        public int MaxCount { get { return maxCount; } }

        public void Reset()
        {
            zeroCount = 1;
            maxCount = 1;
        }

        public void Observe(int cbp, int maximumCbp)
        {
            maxCount = Clamp(maxCount + 1 - (cbp == maximumCbp ? 4 : 0));
            zeroCount = Clamp(zeroCount + 1 - (cbp == 0 ? 4 : 0));
        }

        private static int Clamp(int value)
        {
            if (value < -8) return -8;
            if (value > 7) return 7;
            return value;
        }
    }
}
