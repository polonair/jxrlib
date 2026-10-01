using System;

namespace Jxr.Managed.Core
{
    // Resolves the byte positions of spatial packets from the JPEG XR index
    // table. Offsets are relative to the byte immediately after the table.
    internal sealed class JxrSpatialIndexTable
    {
        private readonly int packetBase;
        private readonly long[] offsets;

        private JxrSpatialIndexTable(int packetBase, long[] offsets)
        {
            this.packetBase = packetBase;
            this.offsets = offsets;
        }

        internal int PacketCount { get { return offsets.Length; } }

        internal static JxrError Read(byte[] source, JxrHeaders headers,
            out JxrSpatialIndexTable table)
        {
            table = null;
            if (source == null || headers == null) return JxrError.InvalidArgument;
            JxrMainHeader main = headers.Main;
            if (!main.HasIndexTable || main.BitstreamFormat != 0)
                return JxrError.UnsupportedFeature;

            long columns = (long)main.VerticalSliceCountMinusOne + 1;
            long rows = (long)main.HorizontalSliceCountMinusOne + 1;
            long entryCountLong = columns * rows;
            long positionLong = (long)headers.CodestreamOffset + headers.ByteCount;
            long codestreamEndLong = (long)headers.CodestreamOffset +
                headers.CodestreamLength;
            if (entryCountLong < 1 || entryCountLong > Int32.MaxValue ||
                positionLong < 0 || positionLong > codestreamEndLong ||
                codestreamEndLong > source.Length)
                return JxrError.InvalidBitstream;
            // Every entry needs at least one byte, in addition to the marker
            // and final length word. Reject impossible dimensions before
            // allocating from untrusted header counts.
            if (entryCountLong > codestreamEndLong - positionLong - 3)
                return JxrError.InvalidBitstream;

            int position = (int)positionLong;
            int codestreamEnd = (int)codestreamEndLong;
            int entryCount = (int)entryCountLong;
            long[] offsets = new long[entryCount];
            if (entryCount != 0)
            {
                if (position > codestreamEnd - 2 ||
                    source[position] != 0 || source[position + 1] != 1)
                    return JxrError.InvalidBitstream;
                position += 2;
                for (int index = 0; index < entryCount; index++)
                {
                    ulong value;
                    bool escaped;
                    JxrError error = ReadVariableLengthWord(source,
                        codestreamEnd, ref position, out value, out escaped);
                    if (error != JxrError.None) return error;
                    if (escaped || value > Int32.MaxValue)
                        return JxrError.UnsupportedFeature;
                    offsets[index] = (long)value;
                }
            }

            ulong trailingValue;
            bool trailingEscape;
            JxrError trailingError = ReadVariableLengthWord(source,
                codestreamEnd, ref position, out trailingValue, out trailingEscape);
            if (trailingError != JxrError.None) return trailingError;
            if (trailingEscape) trailingValue = 0;
            if (trailingValue > Int32.MaxValue)
                return JxrError.UnsupportedFeature;
            long packetBaseLong = (long)position + (long)trailingValue;
            if (packetBaseLong < position || packetBaseLong > codestreamEnd)
                return JxrError.InvalidBitstream;
            int packetBase = (int)packetBaseLong;

            long previous = -1;
            for (int index = 0; index < offsets.Length; index++)
            {
                long packetStart = (long)packetBase + offsets[index];
                if (offsets[index] < 0 || packetStart < packetBase ||
                    packetStart > codestreamEnd - 4 ||
                    (index != 0 && offsets[index] <= previous))
                    return JxrError.InvalidBitstream;
                previous = offsets[index];
            }

            table = new JxrSpatialIndexTable(packetBase, offsets);
            return JxrError.None;
        }

        internal JxrError ReadPacket(byte[] source, JxrHeaders headers,
            int tileRow, int tileColumn, out byte[] packet)
        {
            packet = null;
            if (source == null || headers == null || tileRow < 0 || tileColumn < 0)
                return JxrError.InvalidArgument;
            int tileColumns = headers.Main.VerticalSliceCountMinusOne + 1;
            int tileRows = headers.Main.HorizontalSliceCountMinusOne + 1;
            if (tileColumn >= tileColumns || tileRow >= tileRows)
                return JxrError.InvalidArgument;
            int index = tileRow * tileColumns + tileColumn;
            if (index < 0 || index >= offsets.Length) return JxrError.InvalidBitstream;
            long startLong = (long)packetBase + offsets[index];
            long endLong = index + 1 < offsets.Length ?
                (long)packetBase + offsets[index + 1] :
                (long)headers.CodestreamOffset + headers.CodestreamLength;
            if (startLong < packetBase || endLong <= startLong ||
                endLong > source.Length || startLong > Int32.MaxValue ||
                endLong - startLong > Int32.MaxValue)
                return JxrError.InvalidBitstream;
            int start = (int)startLong;
            int length = (int)(endLong - startLong);
            packet = new byte[length];
            Array.Copy(source, start, packet, 0, length);
            return JxrError.None;
        }

        private static JxrError ReadVariableLengthWord(byte[] source,
            int limit, ref int position, out ulong value, out bool escaped)
        {
            value = 0;
            escaped = false;
            if (position < 0 || position >= limit)
                return JxrError.UnexpectedEndOfStream;
            int first = source[position++];
            if (first >= 0xfd)
            {
                escaped = true;
                return JxrError.None;
            }
            if (first < 0xfb)
            {
                if (position >= limit) return JxrError.UnexpectedEndOfStream;
                value = (ulong)((first << 8) | source[position++]);
                return JxrError.None;
            }
            int wordCount = first == 0xfb ? 2 : 4;
            if ((long)position + wordCount * 2 > limit)
                return JxrError.UnexpectedEndOfStream;
            for (int index = 0; index < wordCount; index++)
            {
                ulong high = source[position++];
                ulong low = source[position++];
                value = (value << 16) | (high << 8) | low;
            }
            return JxrError.None;
        }
    }
}
