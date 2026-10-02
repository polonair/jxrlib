using System;
using System.Collections.Generic;
using System.IO;

namespace Jxr.Managed.Core
{
    public sealed class JxrSourceProfile
    {
        private readonly JxrHeaders headers;
        private readonly JxrSourcePlaneProfile colorPlane;
        private readonly JxrSourcePlaneProfile alphaPlane;

        internal JxrSourceProfile(JxrHeaders headers,
            JxrSourcePlaneProfile colorPlane, JxrSourcePlaneProfile alphaPlane)
        {
            this.headers = headers;
            this.colorPlane = colorPlane;
            this.alphaPlane = alphaPlane;
        }

        public JxrHeaders Headers { get { return headers; } }
        public JxrSourcePlaneProfile ColorPlane { get { return colorPlane; } }
        public JxrSourcePlaneProfile AlphaPlane { get { return alphaPlane; } }
        public bool HasPlanarAlpha { get { return alphaPlane != null; } }
        public bool PacketSyntaxComplete
        {
            get { return colorPlane.SyntaxComplete &&
                (alphaPlane == null || alphaPlane.SyntaxComplete); }
        }
        public bool MacroblockQuantizerMapComplete
        {
            get { return colorPlane.MacroblockQuantizerMapComplete &&
                (alphaPlane == null || alphaPlane.MacroblockQuantizerMapComplete); }
        }
    }

    public sealed class JxrSourcePlaneProfile
    {
        private readonly JxrHeaders headers;
        private readonly JxrProfileTile[] tiles;
        private readonly bool syntaxComplete;
        private readonly bool macroblockQuantizerMapComplete;
        private readonly JxrError packetError;

        internal JxrSourcePlaneProfile(JxrHeaders headers, JxrProfileTile[] tiles,
            bool syntaxComplete, bool macroblockQuantizerMapComplete,
            JxrError packetError)
        {
            this.headers = headers;
            this.tiles = tiles;
            this.syntaxComplete = syntaxComplete;
            this.macroblockQuantizerMapComplete = macroblockQuantizerMapComplete;
            this.packetError = packetError;
        }

        public JxrHeaders Headers { get { return headers; } }
        public int TileCount { get { return tiles.Length; } }
        public JxrProfileTile GetTile(int index) { return tiles[index]; }
        public bool SyntaxComplete { get { return syntaxComplete; } }
        public bool MacroblockQuantizerMapComplete
        { get { return macroblockQuantizerMapComplete; } }
        public JxrError PacketError { get { return packetError; } }
    }

    public sealed class JxrProfileTile
    {
        private readonly int row, column, id;
        private readonly JxrProfilePacket[] packets;
        private readonly JxrProfileQuantizer[] dcQuantizers;
        private readonly JxrProfileQuantizer[] lpQuantizers;
        private readonly JxrProfileQuantizer[] hpQuantizers;
        private readonly bool hasTrimFlexbits;
        private readonly int trimFlexbits;

        internal JxrProfileTile(int row, int column, int id,
            JxrProfilePacket[] packets, JxrProfileQuantizer[] dcQuantizers,
            JxrProfileQuantizer[] lpQuantizers, JxrProfileQuantizer[] hpQuantizers,
            bool hasTrimFlexbits, int trimFlexbits)
        {
            this.row = row;
            this.column = column;
            this.id = id;
            this.packets = packets;
            this.dcQuantizers = dcQuantizers;
            this.lpQuantizers = lpQuantizers;
            this.hpQuantizers = hpQuantizers;
            this.hasTrimFlexbits = hasTrimFlexbits;
            this.trimFlexbits = trimFlexbits;
        }

        public int Row { get { return row; } }
        public int Column { get { return column; } }
        public int Id { get { return id; } }
        public int PacketCount { get { return packets.Length; } }
        public JxrProfilePacket GetPacket(int index) { return packets[index]; }
        public JxrProfileQuantizer[] DcQuantizers { get { return dcQuantizers == null ? null : (JxrProfileQuantizer[])dcQuantizers.Clone(); } }
        public JxrProfileQuantizer[] LowpassQuantizers { get { return lpQuantizers == null ? null : (JxrProfileQuantizer[])lpQuantizers.Clone(); } }
        public JxrProfileQuantizer[] HighpassQuantizers { get { return hpQuantizers == null ? null : (JxrProfileQuantizer[])hpQuantizers.Clone(); } }
        public bool HasTrimFlexbits { get { return hasTrimFlexbits; } }
        public int TrimFlexbits { get { return trimFlexbits; } }
    }

    public sealed class JxrProfilePacket
    {
        private readonly int type, tileId, offset, length;
        internal JxrProfilePacket(int type, int tileId, int offset, int length)
        { this.type = type; this.tileId = tileId; this.offset = offset; this.length = length; }
        public int Type { get { return type; } }
        public int TileId { get { return tileId; } }
        public int Offset { get { return offset; } }
        public int Length { get { return length; } }
    }

    public sealed class JxrProfileQuantizer
    {
        private readonly bool copyPrevious;
        private readonly int channelMode;
        private readonly byte[] indices;
        internal JxrProfileQuantizer(bool copyPrevious, int channelMode, byte[] indices)
        {
            this.copyPrevious = copyPrevious;
            this.channelMode = channelMode;
            this.indices = (byte[])indices.Clone();
        }
        public bool CopyPrevious { get { return copyPrevious; } }
        public int ChannelMode { get { return channelMode; } }
        public int ChannelCount { get { return indices.Length; } }
        public byte GetIndex(int channel) { return indices[channel]; }
    }

    public static class JxrSourceProfileReader
    {
        private const int DefaultMaximumBytes = 536870912;

        public static JxrError Read(byte[] source, out JxrSourceProfile profile)
        {
            profile = null;
            if (source == null) return JxrError.InvalidArgument;
            JxrHeaders headers;
            JxrError error = JxrHeaders.Read(source, out headers);
            if (error != JxrError.None) return error;
            JxrSourcePlaneProfile color;
            error = ReadPlane(source, headers, 0, out color);
            if (error != JxrError.None) return error;
            JxrSourcePlaneProfile alpha = null;
            if (headers.HasPlanarAlpha)
            {
                byte[] alphaBytes = new byte[headers.AlphaByteCount];
                Array.Copy(source, headers.AlphaOffset, alphaBytes, 0, alphaBytes.Length);
                JxrHeaders alphaHeaders;
                error = JxrHeaders.Read(alphaBytes, out alphaHeaders);
                if (error != JxrError.None) return error;
                error = ReadPlane(alphaBytes, alphaHeaders, headers.AlphaOffset,
                    out alpha);
                if (error != JxrError.None) return error;
            }
            profile = new JxrSourceProfile(headers, color, alpha);
            return JxrError.None;
        }

        public static JxrError Read(Stream source, out JxrSourceProfile profile)
        { return Read(source, DefaultMaximumBytes, out profile); }

        public static JxrError Read(Stream source, int maximumBytes,
            out JxrSourceProfile profile)
        {
            profile = null;
            if (source == null || maximumBytes < 8) return JxrError.InvalidArgument;
            try
            {
                MemoryStream memory = new MemoryStream();
                byte[] buffer = new byte[32768];
                while (true)
                {
                    int count = source.Read(buffer, 0, buffer.Length);
                    if (count == 0) break;
                    if (memory.Length + count > maximumBytes)
                        return JxrError.UnsupportedFeature;
                    memory.Write(buffer, 0, count);
                }
                return Read(memory.ToArray(), out profile);
            }
            catch (IOException) { return JxrError.IoFailure; }
        }

        private static JxrError ReadPlane(byte[] source, JxrHeaders headers,
            int globalOffset, out JxrSourcePlaneProfile profile)
        {
            JxrError error = ReadPlanePackets(source, headers, globalOffset,
                out profile);
            if (error == JxrError.None) return JxrError.None;
            int columns = headers.Main.VerticalSliceCountMinusOne + 1;
            int rows = headers.Main.HorizontalSliceCountMinusOne + 1;
            long count = (long)columns * rows;
            if (columns < 1 || rows < 1 || count < 1 || count > 65536)
                return error;
            JxrProfileTile[] tiles = new JxrProfileTile[(int)count];
            for (int index = 0; index < tiles.Length; index++)
                tiles[index] = new JxrProfileTile(index / columns,
                    index % columns, index & 31, new JxrProfilePacket[0],
                    null, null, null, false, 0);
            profile = new JxrSourcePlaneProfile(headers, tiles, false, false,
                error);
            return JxrError.None;
        }

        private static JxrError ReadPlanePackets(byte[] source, JxrHeaders headers,
            int globalOffset, out JxrSourcePlaneProfile profile)
        {
            profile = null;
            JxrMainHeader main = headers.Main;
            int tileColumns = main.VerticalSliceCountMinusOne + 1;
            int tileRows = main.HorizontalSliceCountMinusOne + 1;
            long tileCountLong = (long)tileColumns * tileRows;
            if (tileCountLong < 1 || tileCountLong > 65536)
                return JxrError.UnsupportedFeature;
            int tileCount = (int)tileCountLong;
            JxrPacketIndexTable table = null;
            if (main.HasIndexTable)
            {
                JxrError tableError = JxrPacketIndexTable.Read(source, headers,
                    out table);
                if (tableError != JxrError.None) return tableError;
            }
            else if (main.BitstreamFormat != 0 || tileCount != 1)
                return JxrError.UnsupportedFeature;

            int bandCount = main.BitstreamFormat == 0 ? 1 :
                headers.Plane.Subband == 3 ? 1 : headers.Plane.Subband == 2 ? 2 :
                headers.Plane.Subband == 1 ? 3 : 4;
            List<JxrProfileTile> tiles = new List<JxrProfileTile>(tileCount);
            bool syntaxComplete = true;
            bool quantizerMapComplete = true;
            int singlePacketOffset = 0;
            int singlePacketLength = 0;
            if (!main.HasIndexTable)
            {
                JxrError packetError = LocateSingleSpatialPacket(source, headers,
                    out singlePacketOffset, out singlePacketLength);
                if (packetError != JxrError.None) return packetError;
            }
            for (int row = 0; row < tileRows; row++)
                for (int column = 0; column < tileColumns; column++)
                {
                    int tile = row * tileColumns + column;
                    List<JxrProfilePacket> packets = new List<JxrProfilePacket>();
                    JxrProfileQuantizer[] dc = null, lp = null, hp = null;
                    bool hasTrim = false;
                    int trim = 0;
                    int lowpassCount = 1;
                    for (int band = 0; band < bandCount; band++)
                    {
                        int packetOffset, packetLength;
                        if (main.HasIndexTable)
                        {
                            JxrError spanError = table.GetPacketSpan(headers, row,
                                column, band, out packetOffset, out packetLength);
                            if (spanError != JxrError.None)
                            {
                                if (band == 3 && spanError == JxrError.UnsupportedFeature)
                                    continue;
                                return spanError;
                            }
                        }
                        else
                        {
                            packetOffset = singlePacketOffset;
                            packetLength = singlePacketLength;
                        }
                        if (packetOffset < 0 || packetLength < 4 ||
                            packetOffset > source.Length - packetLength)
                            return JxrError.InvalidBitstream;
                        byte[] packetBytes = new byte[packetLength];
                        Array.Copy(source, packetOffset, packetBytes, 0, packetLength);
                        JxrBitReader reader = new JxrBitReader(packetBytes);
                        JxrPacketHeader packetHeader;
                        JxrError error = JxrPacketReader.ReadHeader(reader,
                            out packetHeader);
                        if (error != JxrError.None) return error;
                        int expectedType = main.BitstreamFormat == 0 ? 0 : band + 1;
                        if (!packetHeader.IsValid || packetHeader.TileId != (tile & 31) ||
                            packetHeader.PacketType != expectedType)
                            return JxrError.InvalidBitstream;
                        packets.Add(new JxrProfilePacket(expectedType,
                            packetHeader.TileId, globalOffset + packetOffset,
                            packetLength));

                        if (main.BitstreamFormat == 0)
                        {
                            if (main.TrimFlexbits)
                            {
                                uint trimValue;
                                error = reader.ReadBits(4, out trimValue);
                                if (error != JxrError.None) return error;
                                trim = (int)trimValue;
                                hasTrim = true;
                            }
                            error = ReadTileQuantizers(reader, headers, ref dc,
                                ref lp, ref hp, ref lowpassCount);
                            if (error != JxrError.None) return error;
                            break;
                        }
                        if (band == 0 && (headers.Quantizers.Mode & 1) != 0)
                        {
                            JxrProfileQuantizer quantizer;
                            error = ReadQuantizer(reader, headers.Plane.ChannelCount,
                                out quantizer);
                            if (error != JxrError.None) return error;
                            dc = new JxrProfileQuantizer[] { quantizer };
                        }
                        else if (band == 1 && (headers.Quantizers.Mode & 2) != 0)
                        {
                            error = ReadQuantizerSet(reader,
                                headers.Plane.ChannelCount, 1, out lp);
                            if (error != JxrError.None) return error;
                            lowpassCount = lp.Length;
                            if (lowpassCount > 1) quantizerMapComplete = false;
                        }
                        else if (band == 2 && (headers.Quantizers.Mode & 4) != 0)
                        {
                            error = ReadQuantizerSet(reader,
                                headers.Plane.ChannelCount, lowpassCount, out hp);
                            if (error != JxrError.None) return error;
                            if (hp.Length > 1) quantizerMapComplete = false;
                        }
                        else if (band == 3 && main.TrimFlexbits)
                        {
                            uint trimValue;
                            error = reader.ReadBits(4, out trimValue);
                            if (error != JxrError.None) return error;
                            trim = (int)trimValue;
                            hasTrim = true;
                        }
                    }
                    if (packets.Count == 0) syntaxComplete = false;
                    tiles.Add(new JxrProfileTile(row, column, tile & 31,
                        packets.ToArray(), dc, lp, hp, hasTrim, trim));
                }
            profile = new JxrSourcePlaneProfile(headers, tiles.ToArray(),
                syntaxComplete, quantizerMapComplete, JxrError.None);
            return JxrError.None;
        }

        private static JxrError ReadTileQuantizers(JxrBitReader reader,
            JxrHeaders headers, ref JxrProfileQuantizer[] dc,
            ref JxrProfileQuantizer[] lp, ref JxrProfileQuantizer[] hp,
            ref int lowpassCount)
        {
            JxrError error;
            if ((headers.Quantizers.Mode & 1) != 0)
            {
                JxrProfileQuantizer value;
                error = ReadQuantizer(reader, headers.Plane.ChannelCount,
                    out value);
                if (error != JxrError.None) return error;
                dc = new JxrProfileQuantizer[] { value };
            }
            if (headers.Plane.Subband != 3 &&
                (headers.Quantizers.Mode & 2) != 0)
            {
                error = ReadQuantizerSet(reader, headers.Plane.ChannelCount,
                    1, out lp);
                if (error != JxrError.None) return error;
                lowpassCount = lp.Length;
            }
            if (headers.Plane.Subband < 2 &&
                (headers.Quantizers.Mode & 4) != 0)
            {
                error = ReadQuantizerSet(reader, headers.Plane.ChannelCount,
                    lowpassCount, out hp);
                if (error != JxrError.None) return error;
            }
            return JxrError.None;
        }

        private static JxrError ReadQuantizerSet(JxrBitReader reader,
            int channelCount, int copyCount, out JxrProfileQuantizer[] quantizers)
        {
            quantizers = null;
            uint value;
            JxrError error = reader.ReadBits(1, out value);
            if (error != JxrError.None) return error;
            if (value != 0)
            {
                quantizers = new JxrProfileQuantizer[copyCount];
                for (int index = 0; index < copyCount; index++)
                quantizers[index] = new JxrProfileQuantizer(true, 0,
                        new byte[channelCount]);
                return JxrError.None;
            }
            error = reader.ReadBits(4, out value);
            if (error != JxrError.None) return error;
            int count = (int)value + 1;
            quantizers = new JxrProfileQuantizer[count];
            for (int index = 0; index < count; index++)
            {
                error = ReadQuantizer(reader, channelCount, out quantizers[index]);
                if (error != JxrError.None) return error;
            }
            return JxrError.None;
        }

        private static JxrError ReadQuantizer(JxrBitReader reader,
            int channelCount, out JxrProfileQuantizer quantizer)
        {
            quantizer = null;
            uint value;
            int mode = 0;
            byte[] indices = new byte[channelCount];
            JxrError error;
            if (channelCount > 1)
            {
                error = reader.ReadBits(2, out value);
                if (error != JxrError.None) return error;
                mode = (int)value;
            }
            error = reader.ReadBits(8, out value);
            if (error != JxrError.None) return error;
            indices[0] = (byte)value;
            if (mode == 1)
            {
                error = reader.ReadBits(8, out value);
                if (error != JxrError.None) return error;
                indices[1] = (byte)value;
            }
            else if (mode > 1)
                for (int channel = 1; channel < channelCount; channel++)
                {
                    error = reader.ReadBits(8, out value);
                    if (error != JxrError.None) return error;
                    indices[channel] = (byte)value;
                }
            quantizer = new JxrProfileQuantizer(false, mode, indices);
            return JxrError.None;
        }

        private static JxrError LocateSingleSpatialPacket(byte[] source,
            JxrHeaders headers, out int packetOffset, out int packetLength)
        {
            packetOffset = packetLength = 0;
            int position = headers.CodestreamOffset + headers.ByteCount;
            int end = headers.CodestreamOffset + headers.CodestreamLength;
            if (position < 0 || position > end - 2) return JxrError.InvalidBitstream;
            // Mirror the native variable-length-word reader: 0xFD..0xFF is
            // an escape marker whose one-byte form ends the size prefix.
            int first = source[position];
            int wordCount, prefixBytes;
            if (first < 0xfb) { wordCount = 1; prefixBytes = 2; }
            else if (first == 0xfb) { wordCount = 2; prefixBytes = 5; }
            else if (first == 0xfc) { wordCount = 4; prefixBytes = 9; }
            else { wordCount = 0; prefixBytes = 1; }
            if (position > end - prefixBytes) return JxrError.InvalidBitstream;
            long headerSize = 0;
            if (wordCount == 1)
                headerSize = (first << 8) | source[position + 1];
            else if (wordCount > 1)
                for (int index = 0; index < wordCount; index++)
                {
                    int wordOffset = position + 1 + index * 2;
                    headerSize = (headerSize << 16) |
                        ((long)source[wordOffset] << 8) | source[wordOffset + 1];
                    if (headerSize > Int32.MaxValue) return JxrError.UnsupportedFeature;
                }
            long packetStart = (long)position + prefixBytes + headerSize;
            if (packetStart < position || packetStart > end - 4)
                return JxrError.InvalidBitstream;
            packetOffset = (int)packetStart;
            packetLength = end - packetOffset;
            return JxrError.None;
        }
    }
}
