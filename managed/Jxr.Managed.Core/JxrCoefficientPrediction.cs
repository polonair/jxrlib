using System;

namespace Jxr.Managed.Core
{
    // Value stored for one channel and one macroblock in the two native PredInfo rows.
    public sealed class JxrPredictionInfo
    {
        private readonly int[] ad = new int[6];
        private int dc;
        private byte quantizerIndex;
        public int Dc { get { return dc; } set { dc = value; } }
        public byte QuantizerIndex { get { return quantizerIndex; } set { quantizerIndex = value; } }
        public int GetAd(int index) { return ad[index]; }
        public void SetAd(int index, int value) { ad[index] = value; }
    }

    // The caller owns row transitions and tile-boundary flags. No native pointers
    // or implicit codec column offsets cross this boundary.
    public sealed class JxrCoefficientPredictionRows
    {
        private JxrPredictionInfo[][] current;
        private JxrPredictionInfo[][] previous;
        private readonly int width;
        private readonly int channelCount;

        public JxrCoefficientPredictionRows(int width, int channelCount)
        {
            if (width < 1 || channelCount < 1 || channelCount > 16)
                throw new ArgumentException("Invalid prediction row geometry.");
            this.width = width;
            this.channelCount = channelCount;
            current = NewRow();
            previous = NewRow();
        }

        public int Width { get { return width; } }
        public int ChannelCount { get { return channelCount; } }
        public JxrPredictionInfo Current(int channel, int column) { return current[channel][column]; }
        public JxrPredictionInfo Previous(int channel, int column) { return previous[channel][column]; }

        public void AdvanceRow()
        {
            JxrPredictionInfo[][] old = previous;
            previous = current;
            current = old;
        }

        private JxrPredictionInfo[][] NewRow()
        {
            JxrPredictionInfo[][] row = new JxrPredictionInfo[channelCount][];
            for (int channel = 0; channel < channelCount; channel++)
            {
                row[channel] = new JxrPredictionInfo[width];
                for (int column = 0; column < width; column++)
                    row[channel][column] = new JxrPredictionInfo();
            }
            return row;
        }
    }

    public static class JxrCoefficientPrediction
    {
        private static readonly int[] Chroma422Offsets = { 0, 64, 16, 80, 32, 96, 48, 112 };
        private static readonly int[] FullTopBlocks = { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15 };

        // Native getACPredMode. 0 = left, 1 = top, 2 = none.
        public static int GetAcMode(JxrMacroblockState macroblock, JxrCodecColorFormat format)
        {
            int horizontal = Abs(Get(macroblock, 0, 1)) + Abs(Get(macroblock, 0, 2)) + Abs(Get(macroblock, 0, 3));
            int vertical = Abs(Get(macroblock, 0, 4)) + Abs(Get(macroblock, 0, 8)) + Abs(Get(macroblock, 0, 12));
            if (format != JxrCodecColorFormat.YOnly && format != JxrCodecColorFormat.NComponent)
            {
                horizontal += Abs(Get(macroblock, 1, 1)) + Abs(Get(macroblock, 2, 1));
                if (format == JxrCodecColorFormat.Yuv420)
                    vertical += Abs(Get(macroblock, 1, 2)) + Abs(Get(macroblock, 2, 2));
                else if (format == JxrCodecColorFormat.Yuv422)
                {
                    vertical += Abs(Get(macroblock, 1, 2)) + Abs(Get(macroblock, 2, 2)) +
                        Abs(Get(macroblock, 1, 6)) + Abs(Get(macroblock, 2, 6));
                    horizontal += Abs(Get(macroblock, 1, 5)) + Abs(Get(macroblock, 2, 5));
                }
                else
                    vertical += Abs(Get(macroblock, 1, 4)) + Abs(Get(macroblock, 2, 4));
            }
            return unchecked(horizontal * 4) < vertical ? 1 :
                (unchecked(vertical * 4) < horizontal ? 0 : 2);
        }

        // Native getDCACPredMode. Low two bits are DC; bits 2-3 are AD.
        public static int GetDcAdMode(JxrCoefficientPredictionRows rows,
            JxrCodecColorFormat format, int column, bool leftBoundary,
            bool topBoundary, byte lowpassQuantizerIndex)
        {
            int dcMode;
            if (leftBoundary && topBoundary) dcMode = 3;
            else if (leftBoundary) dcMode = 1;
            else if (topBoundary) dcMode = 0;
            else
            {
                int scale = format == JxrCodecColorFormat.Yuv420 ? 8 :
                    (format == JxrCodecColorFormat.Yuv422 ? 4 : 2);
                int horizontal = Abs(rows.Previous(0, column - 1).Dc - rows.Current(0, column - 1).Dc);
                int vertical = Abs(rows.Previous(0, column - 1).Dc - rows.Previous(0, column).Dc);
                if (format != JxrCodecColorFormat.YOnly && format != JxrCodecColorFormat.NComponent)
                {
                    horizontal *= scale;
                    vertical *= scale;
                    for (int channel = 1; channel <= 2; channel++)
                    {
                        horizontal += Abs(rows.Previous(channel, column - 1).Dc - rows.Current(channel, column - 1).Dc);
                        vertical += Abs(rows.Previous(channel, column - 1).Dc - rows.Previous(channel, column).Dc);
                    }
                }
                dcMode = unchecked(horizontal * 4) < vertical ? 1 :
                    (unchecked(vertical * 4) < horizontal ? 0 : 2);
            }
            int adMode = 2;
            if (dcMode == 1 && rows.Previous(0, column).QuantizerIndex == lowpassQuantizerIndex) adMode = 1;
            if (dcMode == 0 && rows.Current(0, column - 1).QuantizerIndex == lowpassQuantizerIndex) adMode = 0;
            return dcMode + (adMode << 2);
        }

        // Native updatePredInfo. Call after decoder DC/LP prediction, or before
        // encoder subtraction; the latter must preserve original coefficients.
        public static JxrError StoreCurrent(JxrMacroblockState macroblock,
            JxrCoefficientPredictionRows rows, JxrCodecColorFormat format, int column)
        {
            if (!Valid(macroblock, null, rows, format, column, false)) return JxrError.InvalidArgument;
            for (int channel = 0; channel < rows.ChannelCount; channel++)
            {
                JxrPredictionInfo info = rows.Current(channel, column);
                info.Dc = Get(macroblock, channel, 0);
                info.QuantizerIndex = macroblock.LowpassQuantizerIndex;
                if ((format == JxrCodecColorFormat.Yuv420 || format == JxrCodecColorFormat.Yuv422) && channel != 0)
                {
                    info.SetAd(0, Get(macroblock, channel, 1));
                    info.SetAd(1, Get(macroblock, channel, 2));
                    if (format == JxrCodecColorFormat.Yuv422)
                    {
                        info.SetAd(2, Get(macroblock, channel, 5));
                        info.SetAd(3, Get(macroblock, channel, 6));
                        info.SetAd(4, Get(macroblock, channel, 4));
                    }
                }
                else
                {
                    int[] indexes = { 1, 2, 3, 4, 8, 12 };
                    for (int index = 0; index < indexes.Length; index++)
                        info.SetAd(index, Get(macroblock, channel, indexes[index]));
                }
            }
            return JxrError.None;
        }

        public static JxrError DecodeDcLp(JxrMacroblockState macroblock,
            JxrCoefficientPredictionRows rows, JxrCodecColorFormat format,
            int column, bool leftBoundary, bool topBoundary)
        {
            if (!Valid(macroblock, null, rows, format, column, false) ||
                (!leftBoundary && column == 0)) return JxrError.InvalidArgument;
            int mode = GetDcAdMode(rows, format, column, leftBoundary, topBoundary,
                macroblock.LowpassQuantizerIndex);
            for (int channel = 0; channel < rows.ChannelCount; channel++)
                PredictDcLp(macroblock, rows, format, column, channel, mode, false);
            macroblock.SetOrientation(2 - GetAcMode(macroblock, format));
            return JxrError.None;
        }

        public static JxrError Encode(JxrMacroblockState macroblock,
            JxrCoefficientPlaneState planes, JxrCoefficientPredictionRows rows,
            JxrCodecColorFormat format, int column, bool leftBoundary,
            bool topBoundary)
        {
            if (!Valid(macroblock, planes, rows, format, column, true) ||
                (!leftBoundary && column == 0)) return JxrError.InvalidArgument;
            int mode = GetDcAdMode(rows, format, column, leftBoundary, topBoundary,
                macroblock.LowpassQuantizerIndex);
            int acMode = GetAcMode(macroblock, format);
            macroblock.SetOrientation(2 - acMode);
            StoreCurrent(macroblock, rows, format, column);
            for (int channel = 0; channel < rows.ChannelCount; channel++)
            {
                PredictDcLp(macroblock, rows, format, column, channel, mode, true);
                int[] values;
                planes.GetPlane(channel, out values);
                PredictAc(values, format, channel, acMode, true);
            }
            return JxrError.None;
        }

        public static JxrError DecodeAc(JxrMacroblockState macroblock,
            JxrCoefficientPlaneState planes, JxrCodecColorFormat format)
        {
            if (!Valid(macroblock, planes, null, format, 0, true) ||
                macroblock.Orientation < 0 || macroblock.Orientation > 2)
                return JxrError.InvalidArgument;
            for (int channel = 0; channel < planes.PlaneCount; channel++)
            {
                int[] values;
                planes.GetPlane(channel, out values);
                PredictAc(values, format, channel, 2 - macroblock.Orientation, false);
            }
            return JxrError.None;
        }

        private static void PredictDcLp(JxrMacroblockState mb,
            JxrCoefficientPredictionRows rows, JxrCodecColorFormat format,
            int column, int channel, int mode, bool encode)
        {
            int dcMode = mode & 3;
            int adMode = mode & 12;
            bool chroma420 = format == JxrCodecColorFormat.Yuv420 && channel != 0;
            bool chroma422 = format == JxrCodecColorFormat.Yuv422 && channel != 0;
            JxrPredictionInfo left = column > 0 ? rows.Current(channel, column - 1) : null;
            JxrPredictionInfo top = rows.Previous(channel, column);
            int dc = dcMode == 0 ? left.Dc : (dcMode == 1 ? top.Dc :
                (dcMode == 2 ? unchecked(left.Dc + top.Dc +
                (chroma420 || chroma422 ? 1 : 0)) >> 1 : 0));
            Change(mb, channel, 0, dc, encode);
            if (chroma420)
            {
                if (adMode == 4) Change(mb, channel, 2, top.GetAd(1), encode);
                else if (adMode == 0) Change(mb, channel, 1, left.GetAd(0), encode);
            }
            else if (chroma422)
            {
                if (adMode == 4)
                {
                    Change(mb, channel, 4, top.GetAd(4), encode);
                    if (encode) Change(mb, channel, 6, Get(mb, channel, 2), true);
                    Change(mb, channel, 2, top.GetAd(3), encode);
                    if (!encode) Change(mb, channel, 6, Get(mb, channel, 2), false);
                }
                else if (adMode == 0)
                {
                    Change(mb, channel, 4, left.GetAd(4), encode);
                    Change(mb, channel, 1, left.GetAd(0), encode);
                    Change(mb, channel, 5, left.GetAd(2), encode);
                }
                else if (dcMode == 1) Change(mb, channel, 6, Get(mb, channel, 2), encode);
            }
            else if (adMode == 4)
            {
                Change(mb, channel, 4, top.GetAd(3), encode);
                Change(mb, channel, 8, top.GetAd(4), encode);
                Change(mb, channel, 12, top.GetAd(5), encode);
            }
            else if (adMode == 0)
            {
                for (int index = 1; index <= 3; index++)
                    Change(mb, channel, index, left.GetAd(index - 1), encode);
            }
        }

        private static void PredictAc(int[] values, JxrCodecColorFormat format,
            int channel, int mode, bool encode)
        {
            bool c420 = format == JxrCodecColorFormat.Yuv420 && channel != 0;
            bool c422 = format == JxrCodecColorFormat.Yuv422 && channel != 0;
            if (mode == 1)
            {
                if (c420)
                    for (int block = 1; block <= 3; block += 2)
                        ChangeAc(values, block * 16, 16, true, encode);
                else if (c422)
                {
                    for (int block = 2; block < 8; block++)
                        ChangeAc(values, Chroma422Offsets[block], 16, true, encode);
                }
                else if (encode)
                {
                    for (int block = FullTopBlocks.Length - 1; block >= 0; block--)
                        ChangeAc(values, FullTopBlocks[block] * 16, 16, true, true);
                }
                else
                    for (int block = 0; block < FullTopBlocks.Length; block++)
                        ChangeAc(values, FullTopBlocks[block] * 16, 16, true, false);
            }
            else if (mode == 0)
            {
                if (c420)
                {
                    if (encode)
                        for (int block = 3; block >= 2; block--)
                            ChangeAc(values, block * 16, 32, false, true);
                    else
                        for (int block = 2; block <= 3; block++)
                            ChangeAc(values, block * 16, 32, false, false);
                }
                else if (c422)
                {
                    for (int block = 1; block < 8; block += 2)
                        ChangeAc(values, Chroma422Offsets[block], 64, false, encode);
                }
                else if (encode)
                {
                    for (int block = 15; block >= 4; block--)
                        ChangeAc(values, block * 16, 64, false, true);
                }
                else
                    for (int block = 4; block < 16; block++)
                        ChangeAc(values, block * 16, 64, false, false);
            }
        }

        private static void ChangeAc(int[] values, int offset, int stride,
            bool vertical, bool encode)
        {
            int[] indexes = vertical ? new int[] { 2, 10, 9 } : new int[] { 1, 5, 6 };
            for (int index = 0; index < indexes.Length; index++)
            {
                int target = offset + indexes[index];
                values[target] = encode ? unchecked(values[target] - values[target - stride]) :
                    unchecked(values[target] + values[target - stride]);
            }
        }

        private static int Get(JxrMacroblockState mb, int channel, int index)
        {
            int value;
            mb.GetDcCoefficient(channel, index, out value);
            return value;
        }

        private static void Change(JxrMacroblockState mb, int channel, int index,
            int prediction, bool encode)
        {
            mb.SetDcCoefficient(channel, index, encode ?
                unchecked(Get(mb, channel, index) - prediction) :
                unchecked(Get(mb, channel, index) + prediction));
        }

        private static int Abs(int value) { return value < 0 ? unchecked(-value) : value; }

        private static bool Valid(JxrMacroblockState mb, JxrCoefficientPlaneState planes,
            JxrCoefficientPredictionRows rows, JxrCodecColorFormat format,
            int column, bool requirePlanes)
        {
            if (mb == null || (requirePlanes && planes == null)) return false;
            int count = format == JxrCodecColorFormat.YOnly ? 1 :
                ((format == JxrCodecColorFormat.Yuv420 || format == JxrCodecColorFormat.Yuv422 ||
                  format == JxrCodecColorFormat.Yuv444) ? 3 :
                 (rows != null ? rows.ChannelCount :
                 (planes != null ? planes.PlaneCount : mb.ChannelCapacity)));
            if (mb.ChannelCapacity < count || (rows != null &&
                (rows.ChannelCount != count || column < 0 || column >= rows.Width)) ||
                (requirePlanes && planes.PlaneCount != count)) return false;
            return true;
        }
    }
}
