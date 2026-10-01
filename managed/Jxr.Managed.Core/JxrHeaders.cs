using System;

namespace Jxr.Managed.Core
{
    // The managed syntax values corresponding to JxrMainHeaderDescriptor,
    // JxrImagePlaneDescriptor and JxrImagePlaneQuantizerHeader. They are
    // populated only by JxrHeaders.Read, without a native codec object.
    public sealed class JxrMainHeader
    {
        internal int version, subversion, bitstreamFormat, orientation, overlap;
        internal int codedBitDepth, sourceColorFormat, sourceBitDepth;
        internal bool hardTiles, indexTable, trimFlexbits, redBlueSwapped, alpha, blackWhite;
        internal long width, height;
        internal int extraTop, extraLeft, extraBottom, extraRight;
        internal int verticalSlices, horizontalSlices;
        internal int[] tileX, tileY;
        public int Version { get { return version; } }
        public int Subversion { get { return subversion; } }
        public int BitstreamFormat { get { return bitstreamFormat; } }
        public int Orientation { get { return orientation; } }
        public int Overlap { get { return overlap; } }
        public int CodedBitDepth { get { return codedBitDepth; } }
        public int SourceColorFormat { get { return sourceColorFormat; } }
        public int SourceBitDepth { get { return sourceBitDepth; } }
        public bool HasHardTileBoundaries { get { return hardTiles; } }
        public bool HasIndexTable { get { return indexTable; } }
        public bool TrimFlexbits { get { return trimFlexbits; } }
        public bool RedBlueSwapped { get { return redBlueSwapped; } }
        public bool HasAlpha { get { return alpha; } }
        public bool BlackWhite { get { return blackWhite; } }
        public long Width { get { return width; } }
        public long Height { get { return height; } }
        public int ExtraTop { get { return extraTop; } }
        public int ExtraLeft { get { return extraLeft; } }
        public int ExtraBottom { get { return extraBottom; } }
        public int ExtraRight { get { return extraRight; } }
        public int VerticalSliceCountMinusOne { get { return verticalSlices; } }
        public int HorizontalSliceCountMinusOne { get { return horizontalSlices; } }
        public int GetTileX(int index) { return tileX[index]; }
        public int GetTileY(int index) { return tileY[index]; }
    }

    public sealed class JxrImagePlaneHeader
    {
        internal int colorFormat, subband, channelCount;
        internal bool scaledArithmetic, chromaX, chromaY, sampleConversion;
        internal int chromaCenterX, chromaCenterY, mantissaOrShift, exponentBias;
        public int ColorFormat { get { return colorFormat; } }
        public int Subband { get { return subband; } }
        public int ChannelCount { get { return channelCount; } }
        public bool ScaledArithmetic { get { return scaledArithmetic; } }
        public bool HasChromaCenteringX { get { return chromaX; } }
        public bool HasChromaCenteringY { get { return chromaY; } }
        public int ChromaCenteringX { get { return chromaCenterX; } }
        public int ChromaCenteringY { get { return chromaCenterY; } }
        public bool HasSampleConversion { get { return sampleConversion; } }
        public int MantissaOrShift { get { return mantissaOrShift; } }
        public int ExponentBias { get { return exponentBias; } }
    }

    public sealed class JxrImagePlaneQuantizerHeader
    {
        internal int mode, dcMode, lpMode, hpMode;
        internal bool hasDc, hasLp, hasHp;
        internal readonly byte[] dc = new byte[16];
        internal readonly byte[] lp = new byte[16];
        internal readonly byte[] hp = new byte[16];
        public int Mode { get { return mode; } }
        public int DcMode { get { return dcMode; } }
        public int LowpassMode { get { return lpMode; } }
        public int HighpassMode { get { return hpMode; } }
        public bool HasDc { get { return hasDc; } }
        public bool HasLowpass { get { return hasLp; } }
        public bool HasHighpass { get { return hasHp; } }
        public byte GetDcIndex(int channel) { return dc[channel]; }
        public byte GetLowpassIndex(int channel) { return lp[channel]; }
        public byte GetHighpassIndex(int channel) { return hp[channel]; }
    }

    public sealed class JxrHeaders
    {
        private readonly JxrMainHeader main;
        private readonly JxrImagePlaneHeader plane;
        private readonly JxrImagePlaneQuantizerHeader quantizers;
        private readonly int byteCount;
        private readonly int codestreamOffset;
        private readonly int codestreamLength;

        private JxrHeaders(JxrMainHeader main, JxrImagePlaneHeader plane,
            JxrImagePlaneQuantizerHeader quantizers, int byteCount,
            int codestreamOffset, int codestreamLength)
        {
            this.main = main;
            this.plane = plane;
            this.quantizers = quantizers;
            this.byteCount = byteCount;
            this.codestreamOffset = codestreamOffset;
            this.codestreamLength = codestreamLength;
        }

        public JxrMainHeader Main { get { return main; } }
        public JxrImagePlaneHeader Plane { get { return plane; } }
        public JxrImagePlaneQuantizerHeader Quantizers { get { return quantizers; } }
        public int ByteCount { get { return byteCount; } }
        public int CodestreamOffset { get { return codestreamOffset; } }
        public int CodestreamLength { get { return codestreamLength; } }

        // Syntax-level entry points mirror the three native descriptor readers.
        // The reader remains at the first bit after the parsed descriptor.
        public static JxrError ReadMainHeader(JxrBitReader reader, out JxrMainHeader header)
        {
            header = null;
            if (reader == null) return JxrError.InvalidArgument;
            Cursor cursor = new Cursor(reader);
            JxrMainHeader parsed = ReadMain(cursor);
            if (cursor.Error != JxrError.None) return cursor.Error;
            if (parsed == null) return JxrError.InvalidBitstream;
            header = parsed;
            return JxrError.None;
        }

        public static JxrError ReadImagePlaneHeader(JxrBitReader reader,
            int bitDepth, out JxrImagePlaneHeader header)
        {
            header = null;
            if (reader == null) return JxrError.InvalidArgument;
            Cursor cursor = new Cursor(reader);
            JxrImagePlaneHeader parsed = ReadPlane(cursor, bitDepth);
            if (cursor.Error != JxrError.None) return cursor.Error;
            if (parsed == null) return JxrError.InvalidBitstream;
            header = parsed;
            return JxrError.None;
        }

        public static JxrError ReadImagePlaneQuantizerHeader(JxrBitReader reader,
            int channelCount, int subband, out JxrImagePlaneQuantizerHeader header)
        {
            header = null;
            if (reader == null || channelCount < 1 || channelCount >= 16)
                return JxrError.InvalidArgument;
            Cursor cursor = new Cursor(reader);
            JxrImagePlaneQuantizerHeader parsed = ReadQuantizers(cursor, channelCount, subband);
            if (cursor.Error != JxrError.None) return cursor.Error;
            header = parsed;
            return JxrError.None;
        }

        public static JxrError Read(byte[] source, out JxrHeaders result)
        {
            result = null;
            if (source == null) return JxrError.InvalidArgument;
            int codestreamOffset = 0;
            int codestreamLength = source.Length;
            if (source.Length >= 4 && source[0] == (byte)'I' &&
                source[1] == (byte)'I' && source[2] == 0xbc && source[3] == 1)
            {
                if (!LocateCodestream(source, out codestreamOffset, out codestreamLength))
                    return JxrError.InvalidBitstream;
            }
            else if (source.Length >= 1 && source[0] != (byte)'W')
                return JxrError.InvalidBitstream;
            byte[] payload = new byte[codestreamLength];
            Array.Copy(source, codestreamOffset, payload, 0, codestreamLength);
            Cursor cursor = new Cursor(new JxrBitReader(payload));
            byte[] signature = { (byte)'W', (byte)'M', (byte)'P', (byte)'H',
                (byte)'O', (byte)'T', (byte)'O', 0 };
            for (int index = 0; index < signature.Length; index++)
            {
                uint value = cursor.Read(8);
                if (cursor.Error != JxrError.None) return cursor.Error;
                if (value != signature[index]) return JxrError.InvalidBitstream;
            }

            JxrMainHeader main = ReadMain(cursor);
            if (cursor.Error != JxrError.None) return cursor.Error;
            if (main == null) return JxrError.InvalidBitstream;
            cursor.Align();
            if (cursor.Error != JxrError.None) return cursor.Error;
            JxrImagePlaneHeader plane = ReadPlane(cursor, main.sourceBitDepth);
            if (cursor.Error != JxrError.None) return cursor.Error;
            if (plane == null || plane.channelCount < 1 || plane.channelCount >= 16)
                return JxrError.InvalidBitstream;
            JxrImagePlaneQuantizerHeader quantizers = ReadQuantizers(cursor,
                plane.channelCount, plane.subband);
            if (cursor.Error != JxrError.None) return cursor.Error;
            if ((quantizers.mode & 0x600) == 0) return JxrError.InvalidBitstream;
            if ((main.sourceBitDepth == 8 || main.sourceBitDepth == 9 ||
                main.sourceBitDepth == 10) && plane.colorFormat > 3)
                return JxrError.UnsupportedFeature;
            cursor.Align();
            if (cursor.Error != JxrError.None) return cursor.Error;
            result = new JxrHeaders(main, plane, quantizers,
                cursor.BitPosition / 8, codestreamOffset, codestreamLength);
            return JxrError.None;
        }

        private static bool LocateCodestream(byte[] source, out int offset, out int length)
        {
            offset = 0;
            length = 0;
            if (source.Length < 10) return false;
            uint directoryOffset = ReadU32(source, 4);
            if (directoryOffset > (uint)(source.Length - 2)) return false;
            int directory = (int)directoryOffset;
            int entryCount = ReadU16(source, directory);
            if ((long)directory + 2 + (long)entryCount * 12 > source.Length)
                return false;
            bool foundOffset = false, foundLength = false;
            for (int index = 0; index < entryCount; index++)
            {
                int entry = directory + 2 + index * 12;
                int tag = ReadU16(source, entry);
                if ((tag == 0xbcc0 || tag == 0xbcc1) &&
                    ReadU16(source, entry + 2) == 4 && ReadU32(source, entry + 4) == 1)
                {
                    uint value = ReadU32(source, entry + 8);
                    if (value > Int32.MaxValue) return false;
                    if (tag == 0xbcc0) { offset = (int)value; foundOffset = true; }
                    else { length = (int)value; foundLength = true; }
                }
            }
            return foundOffset && foundLength && offset >= 0 && length >= 8 &&
                offset <= source.Length - length;
        }

        private static int ReadU16(byte[] data, int index)
        {
            return data[index] | (data[index + 1] << 8);
        }

        private static uint ReadU32(byte[] data, int index)
        {
            return (uint)data[index] | ((uint)data[index + 1] << 8) |
                ((uint)data[index + 2] << 16) | ((uint)data[index + 3] << 24);
        }

        private static JxrMainHeader ReadMain(Cursor input)
        {
            JxrMainHeader header = new JxrMainHeader();
            header.version = (int)input.Read(4);
            header.subversion = (int)input.Read(4);
            if (header.version != 1 || (header.subversion != 0 &&
                header.subversion != 1 && header.subversion != 9)) return null;
            header.hardTiles = header.subversion == 9;
            bool tiled = input.Read(1) != 0;
            header.bitstreamFormat = (int)input.Read(1);
            header.orientation = (int)input.Read(3);
            header.indexTable = input.Read(1) != 0;
            header.overlap = (int)input.Read(2);
            if (header.overlap == 3) return null;
            bool abbreviated = input.Read(1) != 0;
            header.codedBitDepth = (int)input.Read(1);
            bool inscribed = input.Read(1) != 0;
            header.trimFlexbits = input.Read(1) != 0;
            bool tileStretch = input.Read(1) != 0;
            header.redBlueSwapped = input.Read(1) != 0;
            input.Read(1); // reserved
            header.alpha = input.Read(1) != 0;
            header.sourceColorFormat = (int)input.Read(4);
            int depth = (int)input.Read(4);
            header.blackWhite = depth == 15;
            header.sourceBitDepth = header.blackWhite ? 0 : depth;
            header.width = (long)input.Read(abbreviated ? 16 : 32) + 1;
            header.height = (long)input.Read(abbreviated ? 16 : 32) + 1;
            header.extraBottom = inscribed ? 0 : (int)((16 - (header.height & 15)) & 15);
            header.extraRight = inscribed ? 0 : (int)((16 - (header.width & 15)) & 15);
            if (tiled)
            {
                header.verticalSlices = (int)input.Read(12);
                header.horizontalSlices = (int)input.Read(12);
            }
            if ((!header.indexTable && (header.bitstreamFormat == 1 ||
                header.verticalSlices + header.horizontalSlices > 0)) ||
                header.verticalSlices >= 4096 || header.horizontalSlices >= 4096)
                return null;
            header.tileX = new int[header.verticalSlices + 1];
            header.tileY = new int[header.horizontalSlices + 1];
            for (int index = 0; index < header.verticalSlices; index++)
                header.tileX[index + 1] = unchecked(header.tileX[index] +
                    (int)input.Read(abbreviated ? 8 : 16));
            for (int index = 0; index < header.horizontalSlices; index++)
                header.tileY[index + 1] = unchecked(header.tileY[index] +
                    (int)input.Read(abbreviated ? 8 : 16));
            if (tileStretch)
                for (int index = 0; index < (header.verticalSlices + 1) *
                    (header.horizontalSlices + 1) && input.Error == JxrError.None;
                    index++) input.Read(8);
            if (inscribed)
            {
                header.extraTop = (int)input.Read(6);
                header.extraLeft = (int)input.Read(6);
                header.extraBottom = (int)input.Read(6);
                header.extraRight = (int)input.Read(6);
            }
            if (((header.width + header.extraLeft + header.extraRight) & 15) +
                ((header.height + header.extraTop + header.extraBottom) & 15) != 0)
            {
                if ((header.width & 15) + (header.height & 15) +
                    header.extraLeft + header.extraTop != 0 ||
                    header.width <= header.extraRight || header.height <= header.extraBottom)
                    return null;
                header.width -= header.extraRight;
                header.height -= header.extraBottom;
            }
            return header;
        }

        private static JxrImagePlaneHeader ReadPlane(Cursor input, int bitDepth)
        {
            JxrImagePlaneHeader plane = new JxrImagePlaneHeader();
            plane.colorFormat = (int)input.Read(3);
            if (plane.colorFormat == 5 || plane.colorFormat > 6) return null;
            plane.scaledArithmetic = input.Read(1) != 0;
            plane.subband = (int)input.Read(4);
            if (plane.colorFormat == 0) plane.channelCount = 1;
            else if (plane.colorFormat == 1)
            {
                plane.channelCount = 3;
                input.Read(1); plane.chromaCenterX = (int)input.Read(3);
                input.Read(1); plane.chromaCenterY = (int)input.Read(3);
                plane.chromaX = plane.chromaY = true;
            }
            else if (plane.colorFormat == 2)
            {
                plane.channelCount = 3;
                input.Read(1); plane.chromaCenterX = (int)input.Read(3);
                input.Read(4); plane.chromaX = true;
            }
            else if (plane.colorFormat == 3)
            {
                plane.channelCount = 3;
                input.Read(4); input.Read(4);
            }
            else if (plane.colorFormat == 4) plane.channelCount = 4;
            else
            {
                plane.channelCount = (int)input.Read(4) + 1;
                input.Read(4);
            }
            if (bitDepth == 2 || bitDepth == 3 || bitDepth == 5 || bitDepth == 6 || bitDepth == 7)
            {
                plane.sampleConversion = true;
                plane.mantissaOrShift = (int)input.Read(8);
                if (bitDepth == 7) plane.exponentBias = (sbyte)input.Read(8);
            }
            return plane;
        }

        private static JxrImagePlaneQuantizerHeader ReadQuantizers(Cursor input,
            int channelCount, int subband)
        {
            JxrImagePlaneQuantizerHeader q = new JxrImagePlaneQuantizerHeader();
            if (input.Read(1) != 0)
            {
                q.hasDc = true;
                q.dcMode = ReadQuantizer(input, channelCount, q.dc);
                q.mode += q.dcMode << 3;
            }
            else q.mode++;
            if (subband != 3)
            {
                if (input.Read(1) == 0)
                {
                    q.mode += 0x200;
                    if (input.Read(1) != 0)
                    {
                        q.hasLp = true;
                        q.lpMode = ReadQuantizer(input, channelCount, q.lp);
                        q.mode += q.lpMode << 5;
                    }
                    else q.mode += 2;
                }
                else q.mode += ((q.mode & 1) << 1) + ((q.mode & 0x18) << 2);
                if (subband != 2)
                {
                    if (input.Read(1) == 0)
                    {
                        q.mode += 0x400;
                        if (input.Read(1) != 0)
                        {
                            q.hasHp = true;
                            q.hpMode = ReadQuantizer(input, channelCount, q.hp);
                            q.mode += q.hpMode << 7;
                        }
                        else q.mode += 4;
                    }
                    else q.mode += ((q.mode & 2) << 1) + ((q.mode & 0x60) << 2);
                }
            }
            if (subband == 3) q.mode |= 0x200;
            else if (subband == 2) q.mode |= 0x400;
            return q;
        }

        private static int ReadQuantizer(Cursor input, int channels, byte[] indices)
        {
            int mode = channels > 1 ? (int)input.Read(2) : 0;
            indices[0] = (byte)input.Read(8);
            if (mode == 1) indices[1] = (byte)input.Read(8);
            else if (mode > 0)
                for (int channel = 1; channel < channels; channel++)
                    indices[channel] = (byte)input.Read(8);
            return mode;
        }

        private sealed class Cursor
        {
            private readonly JxrBitReader reader;
            private JxrError error;
            internal Cursor(JxrBitReader reader) { this.reader = reader; }
            internal JxrError Error { get { return error; } }
            internal int BitPosition { get { return reader.BitPosition; } }
            internal uint Read(int count)
            {
                uint value;
                if (error != JxrError.None) return 0;
                error = reader.ReadBits(count, out value);
                return value;
            }
            internal void Align()
            {
                if (error == JxrError.None) error = reader.AlignToByte();
            }
        }
    }
}
