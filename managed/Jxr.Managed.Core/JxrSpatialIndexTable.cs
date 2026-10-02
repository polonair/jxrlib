using System;

namespace Jxr.Managed.Core
{
    // Resolves packet positions from the JPEG XR index table. Spatial offsets
    // are tile-ordered; frequency offsets are mapped by packet tile/type.
    internal sealed class JxrPacketIndexTable
    {
        private readonly int packetBase;
        private readonly long[] offsets;
        private readonly byte[][] frequencyPackets;
        private readonly long[] frequencyPacketOffsets;
        private readonly long[] frequencyPacketLengths;

        private JxrPacketIndexTable(int packetBase, long[] offsets,
            byte[][] frequencyPackets, long[] frequencyPacketOffsets,
            long[] frequencyPacketLengths)
        {
            this.packetBase = packetBase;
            this.offsets = offsets;
            this.frequencyPackets = frequencyPackets;
            this.frequencyPacketOffsets = frequencyPacketOffsets;
            this.frequencyPacketLengths = frequencyPacketLengths;
        }

        internal int PacketCount { get { return offsets.Length; } }

        internal static JxrError Read(byte[] source, JxrHeaders headers,
            out JxrPacketIndexTable table)
        {
            table = null;
            if (source == null || headers == null) return JxrError.InvalidArgument;
            JxrMainHeader main = headers.Main;
            if (!main.HasIndexTable || main.BitstreamFormat > 1)
                return JxrError.UnsupportedFeature;

            long columns = (long)main.VerticalSliceCountMinusOne + 1;
            long rows = (long)main.HorizontalSliceCountMinusOne + 1;
            long tileCountLong = columns * rows;
            int bandCount = main.BitstreamFormat == 0 ? 1 :
                headers.Plane.Subband == 3 ? 1 : headers.Plane.Subband == 2 ? 2 :
                headers.Plane.Subband == 1 ? 3 : 4;
            long entryCountLong = tileCountLong * bandCount;
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
                    if ((escaped && main.BitstreamFormat == 0) ||
                        value > Int32.MaxValue)
                        return JxrError.UnsupportedFeature;
                    offsets[index] = escaped ? -1 : (long)value;
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

            if (main.BitstreamFormat == 1)
            {
                if (tileCountLong > Int32.MaxValue / 4)
                    return JxrError.UnsupportedFeature;
                byte[][] packets = new byte[(int)tileCountLong * 4][];
                long[] packetOffsets = new long[packets.Length];
                long[] packetLengths = new long[packets.Length];
                for (int packetIndex = 0; packetIndex < packetOffsets.Length; packetIndex++)
                    packetOffsets[packetIndex] = -1;
                long[] starts = new long[offsets.Length];
                int realCount = 0;
                for (int index = 0; index < offsets.Length; index++)
                {
                    if (offsets[index] < 0) continue;
                    long start = (long)packetBase + offsets[index];
                    if (start < packetBase || start > codestreamEnd - 4)
                        return JxrError.InvalidBitstream;
                    starts[realCount++] = start;
                }
                Array.Sort(starts, 0, realCount);
                for (int entry = 0; entry < offsets.Length; entry++)
                {
                    if (offsets[entry] < 0) continue;
                    long start = (long)packetBase + offsets[entry];
                    int next = 0;
                    while (next < realCount && starts[next] <= start) next++;
                    long end = next < realCount ? starts[next] : codestreamEnd;
                    if (end <= start || end - start > Int32.MaxValue)
                        return JxrError.InvalidBitstream;
                    int packetPosition = (int)start;
                    if (source[packetPosition] != 0 || source[packetPosition + 1] != 0 ||
                        source[packetPosition + 2] != 1) return JxrError.InvalidBitstream;
                    int tile = entry / bandCount;
                    int band = entry % bandCount;
                    int packetTileId = source[packetPosition + 3] >> 3;
                    int type = source[packetPosition + 3] & 7;
                    if (tile >= tileCountLong || type != band + 1 ||
                        packetTileId != (tile & 31))
                        return JxrError.InvalidBitstream;
                    int target = tile * 4 + band;
                    if (packets[target] != null) return JxrError.InvalidBitstream;
                    packets[target] = new byte[(int)(end - start)];
                    Array.Copy(source, packetPosition, packets[target], 0,
                        (int)(end - start));
                    packetOffsets[target] = start;
                    packetLengths[target] = end - start;
                }
                table = new JxrPacketIndexTable(packetBase, offsets, packets,
                    packetOffsets, packetLengths);
                return JxrError.None;
            }

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

            table = new JxrPacketIndexTable(packetBase, offsets, null, null, null);
            return JxrError.None;
        }

        internal JxrError GetPacketSpan(JxrHeaders headers, int tileRow,
            int tileColumn, int band, out int offset, out int length)
        {
            offset = length = 0;
            if (headers == null || tileRow < 0 || tileColumn < 0 || band < 0)
                return JxrError.InvalidArgument;
            int columns = headers.Main.VerticalSliceCountMinusOne + 1;
            int rows = headers.Main.HorizontalSliceCountMinusOne + 1;
            if (tileColumn >= columns || tileRow >= rows)
                return JxrError.InvalidArgument;
            int tile = tileRow * columns + tileColumn;
            long start, count;
            if (frequencyPackets != null)
            {
                if (band > 3) return JxrError.InvalidArgument;
                int index = tile * 4 + band;
                if (index < 0 || index >= frequencyPacketOffsets.Length ||
                    frequencyPacketOffsets[index] < 0)
                    return band == 3 ? JxrError.UnsupportedFeature :
                        JxrError.InvalidBitstream;
                start = frequencyPacketOffsets[index];
                count = frequencyPacketLengths[index];
            }
            else
            {
                if (band != 0 || tile < 0 || tile >= offsets.Length ||
                    offsets[tile] < 0) return JxrError.InvalidArgument;
                start = (long)packetBase + offsets[tile];
                long end = tile + 1 < offsets.Length ?
                    (long)packetBase + offsets[tile + 1] :
                    (long)headers.CodestreamOffset + headers.CodestreamLength;
                count = end - start;
            }
            if (start < 0 || count < 4 || start > Int32.MaxValue ||
                count > Int32.MaxValue || start + count > Int32.MaxValue)
                return JxrError.InvalidBitstream;
            offset = (int)start;
            length = (int)count;
            return JxrError.None;
        }

        internal JxrError ReadPacket(byte[] source, JxrHeaders headers,
            int tileRow, int tileColumn, out byte[] packet)
        {
            packet = null;
            if (frequencyPackets != null) return JxrError.UnsupportedFeature;
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

        internal JxrError ReadFrequencyPacket(byte[] source, JxrHeaders headers,
            int tileRow, int tileColumn, int band, out byte[] packet)
        {
            packet = null;
            if (frequencyPackets == null || source == null || headers == null ||
                tileRow < 0 || tileColumn < 0 || band < 0 || band > 3)
                return JxrError.InvalidArgument;
            int columns = headers.Main.VerticalSliceCountMinusOne + 1;
            int rows = headers.Main.HorizontalSliceCountMinusOne + 1;
            if (tileColumn >= columns || tileRow >= rows)
                return JxrError.InvalidArgument;
            int tile = tileRow * columns + tileColumn;
            if (tile < 0 || tile >= frequencyPackets.Length / 4)
                return JxrError.InvalidBitstream;
            packet = frequencyPackets[tile * 4 + band];
            if (packet == null && band == 3) return JxrError.None;
            return packet == null ? JxrError.InvalidBitstream : JxrError.None;
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
