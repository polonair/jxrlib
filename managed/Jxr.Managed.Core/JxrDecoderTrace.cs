using System;
using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    // Opt-in, bounded trace for comparing a single decoded macroblock with
    // the native decoder. This is diagnostic data, not part of the codec API.
    public sealed class JxrDecoderTrace
    {
        private readonly int macroblockX;
        private readonly int macroblockY;
        private readonly List<JxrDecoderTraceStage> stages =
            new List<JxrDecoderTraceStage>();
        private readonly List<JxrDecoderTraceBitRange> bitRanges =
            new List<JxrDecoderTraceBitRange>();

        public JxrDecoderTrace(int macroblockX, int macroblockY)
        {
            if (macroblockX < 0 || macroblockY < 0)
                throw new ArgumentOutOfRangeException("macroblockX");
            this.macroblockX = macroblockX;
            this.macroblockY = macroblockY;
        }

        public int MacroblockX { get { return macroblockX; } }
        public int MacroblockY { get { return macroblockY; } }
        public int StageCount { get { return stages.Count; } }
        public int BitRangeCount { get { return bitRanges.Count; } }
        public JxrDecoderTraceStage GetStage(int index) { return stages[index]; }
        public JxrDecoderTraceBitRange GetBitRange(int index) { return bitRanges[index]; }

        internal bool Selects(int x, int y)
        { return x == macroblockX && y == macroblockY; }

        internal void RecordStage(string name, int x, int y, string channel,
            int[] values)
        {
            if (!Selects(x, y) || values == null) return;
            stages.Add(new JxrDecoderTraceStage(name, x, y, channel, values));
        }

        internal void RecordBitRange(string name, int x, int y, int tileRow,
            int tileColumn, int packetOffset, int startBit, int endBit)
        {
            if (!Selects(x, y)) return;
            bitRanges.Add(new JxrDecoderTraceBitRange(name, x, y, tileRow,
                tileColumn, packetOffset, startBit, endBit));
        }
    }

    public sealed class JxrDecoderTraceStage
    {
        private readonly string name;
        private readonly int x, y;
        private readonly string channel;
        private readonly int[] values;

        internal JxrDecoderTraceStage(string name, int x, int y,
            string channel, int[] values)
        {
            this.name = name;
            this.x = x;
            this.y = y;
            this.channel = channel;
            this.values = (int[])values.Clone();
        }

        public string Name { get { return name; } }
        public int X { get { return x; } }
        public int Y { get { return y; } }
        public string Channel { get { return channel; } }
        public int[] Values { get { return (int[])values.Clone(); } }
    }

    public sealed class JxrDecoderTraceBitRange
    {
        private readonly string name;
        private readonly int x, y, tileRow, tileColumn, packetOffset;
        private readonly int startBit, endBit;

        internal JxrDecoderTraceBitRange(string name, int x, int y,
            int tileRow, int tileColumn, int packetOffset, int startBit,
            int endBit)
        {
            this.name = name;
            this.x = x;
            this.y = y;
            this.tileRow = tileRow;
            this.tileColumn = tileColumn;
            this.packetOffset = packetOffset;
            this.startBit = startBit;
            this.endBit = endBit;
        }

        public string Name { get { return name; } }
        public int X { get { return x; } }
        public int Y { get { return y; } }
        public int TileRow { get { return tileRow; } }
        public int TileColumn { get { return tileColumn; } }
        public int PacketOffset { get { return packetOffset; } }
        public int StartBit { get { return startBit; } }
        public int EndBit { get { return endBit; } }
    }
}
