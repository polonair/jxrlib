using System;

namespace Jxr.Managed.Core
{
    // Writes the eight TIFF-like IFD entries emitted by the native utility
    // for an 8-bit Gray or RGB image with no optional metadata or alpha plane.
    public static class JxrContainerWriter
    {
        private const int DirectoryOffset = 32;
        private const int EntryCount = 8;
        private const int CodestreamOffset = DirectoryOffset + 2 + 12 * EntryCount + 4;
        private static readonly Guid Gray8PixelFormat = new Guid(0x6fddc324,
            0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x08);
        private static readonly Guid Rgb24PixelFormat = new Guid(0x6fddc324,
            0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0c);

        public static JxrError WriteRgb24(byte[] codestream, int width,
            int height, float horizontalDpi, float verticalDpi, out byte[] container)
        {
            return WriteContainer(codestream, width, height, horizontalDpi,
                verticalDpi, Rgb24PixelFormat, out container);
        }

        public static JxrError WriteGray8(byte[] codestream, int width,
            int height, float horizontalDpi, float verticalDpi, out byte[] container)
        {
            return WriteContainer(codestream, width, height, horizontalDpi,
                verticalDpi, Gray8PixelFormat, out container);
        }

        private static JxrError WriteContainer(byte[] codestream, int width,
            int height, float horizontalDpi, float verticalDpi, Guid pixelFormat,
            out byte[] container)
        {
            container = null;
            if (codestream == null || width <= 0 || height <= 0 ||
                horizontalDpi <= 0 || verticalDpi <= 0 ||
                Single.IsInfinity(horizontalDpi) || Single.IsInfinity(verticalDpi) ||
                Single.IsNaN(horizontalDpi) || Single.IsNaN(verticalDpi))
                return JxrError.InvalidArgument;
            if ((long)CodestreamOffset + codestream.Length > Int32.MaxValue)
                return JxrError.UnsupportedFeature;
            JxrBitWriter writer = new JxrBitWriter();
            writer.Write((byte)'I', 8);
            writer.Write((byte)'I', 8);
            Write16(writer, 0x01bc);
            Write32(writer, DirectoryOffset);
            byte[] guid = pixelFormat.ToByteArray();
            for (int index = 0; index < guid.Length; index++)
                writer.Write(guid[index], 8);
            for (int index = 0; index < DirectoryOffset - 8 - guid.Length; index++)
                writer.Write(0, 8);
            Write16(writer, EntryCount);
            WriteEntry(writer, 0xbc01, 1, 16, 8);   // pixel format GUID
            WriteEntry(writer, 0xbc02, 4, 1, 0);    // identity orientation
            WriteEntry(writer, 0xbc80, 4, 1, (uint)width);
            WriteEntry(writer, 0xbc81, 4, 1, (uint)height);
            WriteEntry(writer, 0xbc82, 11, 1, FloatBits(horizontalDpi));
            WriteEntry(writer, 0xbc83, 11, 1, FloatBits(verticalDpi));
            WriteEntry(writer, 0xbcc0, 4, 1, CodestreamOffset);
            WriteEntry(writer, 0xbcc1, 4, 1, (uint)codestream.Length);
            Write32(writer, 0); // no next IFD
            byte[] prefix = writer.ToArray();
            if (prefix.Length != CodestreamOffset)
                return JxrError.InvalidBitstream;
            byte[] result = new byte[prefix.Length + codestream.Length];
            Array.Copy(prefix, result, prefix.Length);
            Array.Copy(codestream, 0, result, prefix.Length, codestream.Length);
            container = result;
            return JxrError.None;
        }

        private static uint FloatBits(float value)
        {
            return BitConverter.ToUInt32(BitConverter.GetBytes(value), 0);
        }

        private static void WriteEntry(JxrBitWriter writer, int tag,
            int type, uint count, uint value)
        {
            Write16(writer, tag);
            Write16(writer, type);
            Write32(writer, count);
            Write32(writer, value);
        }

        private static void Write16(JxrBitWriter writer, int value)
        {
            writer.Write((uint)(value & 255), 8);
            writer.Write((uint)((value >> 8) & 255), 8);
        }

        private static void Write32(JxrBitWriter writer, uint value)
        {
            writer.Write(value & 255, 8);
            writer.Write((value >> 8) & 255, 8);
            writer.Write((value >> 16) & 255, 8);
            writer.Write((value >> 24) & 255, 8);
        }
    }
}
