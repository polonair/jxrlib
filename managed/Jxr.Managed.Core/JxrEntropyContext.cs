using System;

namespace Jxr.Managed.Core
{
    public enum JxrAdaptiveBand
    {
        Dc = 1,
        Lowpass = 2,
        Highpass = 3
    }

    // Managed form of CAdaptiveModel.  Channel 0 is luma and channel 1 is
    // chroma, as in the two-element native FLC state and bit arrays.
    public sealed class JxrAdaptiveModel
    {
        private static readonly int[] lumaWeights = { 240, 12, 1 };
        private static readonly int[][] channelWeights = {
            new int[] { 0,240,120,80,60,48,40,34,30,27,24,22,20,18,17,16 },
            new int[] { 0,12,6,4,3,2,2,2,2,1,1,1,1,1,1,1 },
            new int[] { 0,16,8,5,4,3,3,2,2,2,2,1,1,1,1,1 }
        };
        private static readonly int[] subsampledWeights = { 120,37,2,120,18,1 };
        private readonly int[] flcStates = new int[2];
        private readonly int[] flcBits = new int[2];
        private JxrAdaptiveBand band;

        public JxrAdaptiveBand Band { get { return band; } }

        public void Reset(JxrAdaptiveBand newBand)
        {
            int initialBits = newBand == JxrAdaptiveBand.Dc ? 8 :
                newBand == JxrAdaptiveBand.Lowpass ? 4 : 0;
            band = newBand;
            for (int channel = 0; channel < 2; channel++)
            {
                flcStates[channel] = 0;
                flcBits[channel] = initialBits;
            }
        }

        public JxrError Get(int channel, out int flcState, out int bitCount)
        {
            flcState = 0;
            bitCount = 0;
            if (channel < 0 || channel >= 2) return JxrError.InvalidArgument;
            flcState = flcStates[channel];
            bitCount = flcBits[channel];
            return JxrError.None;
        }

        public JxrError Set(int channel, int flcState, int bitCount)
        {
            if (channel < 0 || channel >= 2) return JxrError.InvalidArgument;
            flcStates[channel] = flcState;
            flcBits[channel] = bitCount;
            return JxrError.None;
        }

        // Exact managed form of UpdateModelMB in image/sys/image.c.  The
        // laplacian mean array is scaled in place, like the native input.
        public JxrError UpdateForMacroblock(JxrCodecColorFormat colorFormat,
            int channelCount, int[] laplacianMean)
        {
            int bandIndex = (int)band - (int)JxrAdaptiveBand.Dc;
            int channel;
            if (laplacianMean == null || laplacianMean.Length < 2 ||
                channelCount < 1 || channelCount > 16 || bandIndex < 0 || bandIndex > 2)
                return JxrError.InvalidArgument;
            laplacianMean[0] = unchecked(laplacianMean[0] * lumaWeights[bandIndex]);
            if (colorFormat == JxrCodecColorFormat.Yuv420)
                laplacianMean[1] = unchecked(laplacianMean[1] * subsampledWeights[bandIndex]);
            else if (colorFormat == JxrCodecColorFormat.Yuv422)
                laplacianMean[1] = unchecked(laplacianMean[1] * subsampledWeights[3 + bandIndex]);
            else
            {
                laplacianMean[1] = unchecked(laplacianMean[1] * channelWeights[bandIndex][channelCount - 1]);
                if (band == JxrAdaptiveBand.Highpass) laplacianMean[1] >>= 4;
            }
            for (channel = 0; channel < 2; channel++)
            {
                int mean = laplacianMean[channel];
                int modelState = flcStates[channel];
                int delta = unchecked(mean - 70) >> 2;
                if (delta <= -8)
                {
                    delta += 4;
                    if (delta < -16) delta = -16;
                    modelState += delta;
                    if (modelState < -8)
                    {
                        if (flcBits[channel] == 0) modelState = -8;
                        else { modelState = 0; flcBits[channel]--; }
                    }
                }
                else if (delta >= 8)
                {
                    delta -= 4;
                    if (delta > 15) delta = 15;
                    modelState += delta;
                    if (modelState > 8)
                    {
                        if (flcBits[channel] >= 15) { flcBits[channel] = 15; modelState = 8; }
                        else { modelState = 0; flcBits[channel]++; }
                    }
                }
                flcStates[channel] = modelState;
                if (colorFormat == JxrCodecColorFormat.YOnly) break;
            }
            return JxrError.None;
        }
    }

    // Owns the mutable fields reset by JxrEntropyContextReset in the native
    // reference.  The six referenced objects keep their identity across a
    // reset, just as the C view keeps pointers into one CCodingContext.
    public sealed class JxrEntropyContext
    {
        private static readonly uint[] lowpassIndexes =
            { 0, 1, 4, 5, 2, 8, 6, 9, 3, 12, 10, 7, 13, 11, 14, 15 };
        private static readonly uint[] horizontalIndexes =
            { 0, 5, 10, 12, 1, 2, 8, 4, 6, 9, 3, 14, 13, 7, 11, 15 };
        private static readonly uint[] verticalIndexes =
            { 0, 10, 2, 12, 5, 9, 4, 8, 1, 13, 6, 15, 14, 3, 11, 7 };

        private readonly JxrAdaptiveModel dcModel = new JxrAdaptiveModel();
        private readonly JxrAdaptiveModel lpModel = new JxrAdaptiveModel();
        private readonly JxrAdaptiveModel acModel = new JxrAdaptiveModel();
        private readonly JxrAdaptiveScanSet scans = JxrAdaptiveScanSet.CreateDefault();
        private readonly JxrLowpassCbpState lowpassCbp = new JxrLowpassCbpState(1, 1);
        private readonly JxrCbpPredictionModel highpassCbp = new JxrCbpPredictionModel();
        private int trimFlexBits;
        private bool inRoi;

        public JxrEntropyContext() { Reset(); }

        public JxrAdaptiveModel DcModel { get { return dcModel; } }
        public JxrAdaptiveModel LpModel { get { return lpModel; } }
        public JxrAdaptiveModel AcModel { get { return acModel; } }
        public JxrAdaptiveScan LowpassScan { get { return scans.Lowpass; } }
        public JxrAdaptiveScan HorizontalScan { get { return scans.Horizontal; } }
        public JxrAdaptiveScan VerticalScan { get { return scans.Vertical; } }
        public JxrLowpassCbpState LowpassCbp { get { return lowpassCbp; } }
        public JxrCbpPredictionModel HighpassCbp { get { return highpassCbp; } }
        public int TrimFlexBits { get { return trimFlexBits; } set { trimFlexBits = value; } }
        public bool InRoi { get { return inRoi; } set { inRoi = value; } }

        public void Reset()
        {
            dcModel.Reset(JxrAdaptiveBand.Dc);
            lpModel.Reset(JxrAdaptiveBand.Lowpass);
            acModel.Reset(JxrAdaptiveBand.Highpass);
            lowpassCbp.Reset();
            highpassCbp.Reset();
            // Native InitZigzagScan overwrites uScan only, not uTotal.
            scans.Lowpass.ResetIndexes(lowpassIndexes);
            scans.Horizontal.ResetIndexes(horizontalIndexes);
            scans.Vertical.ResetIndexes(verticalIndexes);
        }
    }
}
