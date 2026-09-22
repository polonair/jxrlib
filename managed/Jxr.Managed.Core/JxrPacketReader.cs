using System;

namespace Jxr.Managed.Core
{
    // Managed value form of JxrPacketHeaderSyntax.  The four wire bytes are
    // retained so validation never loses information from the input packet.
    public sealed class JxrPacketHeader
    {
        private byte prefix0;
        private byte prefix1;
        private byte marker;
        private byte tileAndType;

        public byte Prefix0 { get { return prefix0; } }
        public byte Prefix1 { get { return prefix1; } }
        public byte Marker { get { return marker; } }
        public byte TileAndType { get { return tileAndType; } }
        public byte TileId { get { return (byte)(tileAndType >> 3); } }
        public byte PacketType { get { return (byte)(tileAndType & 7); } }
        public bool IsValid { get { return prefix0 == 0 && prefix1 == 0 && marker == 1; } }

        internal void SetPrefix0(byte value) { prefix0 = value; }
        internal void SetPrefix1(byte value) { prefix1 = value; }
        internal void SetMarker(byte value) { marker = value; }
        internal void SetTileAndType(byte value) { tileAndType = value; }
    }

    // Managed counterpart of JxrPacketHeaderSyntaxReader.c.  This layer only
    // interprets header syntax; packet transport and refill stay outside it.
    public static class JxrPacketReader
    {
        public static JxrError ReadHeader(JxrBitReader reader, out JxrPacketHeader header)
        {
            uint value;
            JxrError error;
            header = null;
            if (reader == null) return JxrError.InvalidArgument;
            header = new JxrPacketHeader();

            error = reader.ReadBits(8, out value);
            if (error != JxrError.None) return error;
            header.SetPrefix0((byte)value);
            error = reader.ReadBits(8, out value);
            if (error != JxrError.None) return error;
            header.SetPrefix1((byte)value);
            error = reader.ReadBits(8, out value);
            if (error != JxrError.None) return error;
            header.SetMarker((byte)value);
            error = reader.ReadBits(8, out value);
            if (error != JxrError.None) return error;
            header.SetTileAndType((byte)value);
            return JxrError.None;
        }
    }
}
