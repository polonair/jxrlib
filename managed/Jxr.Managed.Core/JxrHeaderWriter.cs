using System;

namespace Jxr.Managed.Core
{
    // Field-by-field counterpart of the native main/image-plane writers.
    // This first writer supports one Y_ONLY spatial tile and frame quantizers.
    public static class JxrHeaderWriter
    {
        public static JxrError WriteGraySpatial(JxrBitWriter writer,
            int width, int height, byte quantizerIndex)
        {
            if (writer == null || width < 1 || height < 1 ||
                writer.BitCount != 0)
                return JxrError.InvalidArgument;
            bool abbreviated = ((long)width + 15) / 16 <= 255 &&
                ((long)height + 15) / 16 <= 255;
            byte[] signature = { (byte)'W', (byte)'M', (byte)'P', (byte)'H',
                (byte)'O', (byte)'T', (byte)'O', 0 };
            for (int index = 0; index < signature.Length; index++)
                writer.Write(signature[index], 8);

            writer.Write(1, 4);  // version
            writer.Write(1, 4);  // new-scaling, soft-tile subversion
            writer.Write(0, 1);  // no tiling
            writer.Write(0, 1);  // spatial layout
            writer.Write(0, 3);  // orientation
            writer.Write(0, 1);  // no index table
            writer.Write(0, 2);  // no overlap
            writer.Write(abbreviated ? 1U : 0U, 1);
            writer.Write(1, 1);  // short header flag used by native encoder
            writer.Write(0, 1);  // no windowing
            writer.Write(0, 1);  // no flexbit trimming
            writer.Write(0, 1);  // no tile stretching
            writer.Write(0, 2);  // reserved and red/blue swap
            writer.Write(0, 1);  // no alpha
            writer.Write(0, 4);  // Y_ONLY source color format
            writer.Write(1, 4);  // 8-bit source depth
            writer.Write((uint)(width - 1), abbreviated ? 16 : 32);
            writer.Write((uint)(height - 1), abbreviated ? 16 : 32);
            writer.AlignByte();

            writer.Write(0, 3);  // Y_ONLY plane
            writer.Write(0, 1);  // unscaled arithmetic
            writer.Write(0, 4);  // all subbands
            writer.Write(1, 1); writer.Write(quantizerIndex, 8); // DC frame QP
            writer.Write(0, 1); writer.Write(1, 1);              // LP own frame QP
            writer.Write(quantizerIndex, 8);
            writer.Write(0, 1); writer.Write(1, 1);              // HP own frame QP
            writer.Write(quantizerIndex, 8);
            writer.AlignByte();
            return JxrError.None;
        }
    }

    public static class JxrPacketWriter
    {
        public static JxrError WriteHeader(JxrBitWriter writer,
            int tileId, int packetType)
        {
            if (writer == null || (writer.BitCount & 7) != 0 ||
                tileId < 0 || tileId > 31 || packetType < 0 || packetType > 4)
                return JxrError.InvalidArgument;
            writer.Write(0, 8);
            writer.Write(0, 8);
            writer.Write(1, 8);
            writer.Write((uint)((tileId << 3) | packetType), 8);
            return JxrError.None;
        }
    }

    public static class JxrCodestreamWriter
    {
        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte quantizerIndex, out byte[] codestream)
        {
            codestream = null;
            if (entropyPacket == null) return JxrError.InvalidArgument;
            JxrBitWriter writer = new JxrBitWriter();
            JxrError error = JxrHeaderWriter.WriteGraySpatial(writer,
                width, height, quantizerIndex);
            if (error != JxrError.None) return error;

            // No index table in this spatial profile. The length word covers
            // the marker record, not the entropy packet; its actual length is
            // carried by the container's ImageByteCount field.
            JxrBitWriter marker = new JxrBitWriter();
            marker.Write(0x6f, 8);
            marker.Write(0xff, 8);
            marker.Write(1, 16);
            byte[] markerBytes = marker.ToArray();
            error = JxrVariableLengthWordWriter.Write(writer,
                (uint)markerBytes.Length);
            if (error != JxrError.None) return error;
            for (int index = 0; index < markerBytes.Length; index++)
                writer.Write(markerBytes[index], 8);
            error = JxrPacketWriter.WriteHeader(writer, 0, 0);
            if (error != JxrError.None) return error;
            byte[] prefix = writer.ToArray();
            long total = (long)prefix.Length + entropyPacket.Length;
            if (total > Int32.MaxValue) return JxrError.UnsupportedFeature;
            byte[] result = new byte[(int)total];
            Array.Copy(prefix, result, prefix.Length);
            Array.Copy(entropyPacket, 0, result, prefix.Length,
                entropyPacket.Length);
            codestream = result;
            return JxrError.None;
        }
    }

    public static class JxrVariableLengthWordWriter
    {
        // The native 16/40-bit forms needed for 32-bit managed buffers.
        public static JxrError Write(JxrBitWriter writer, uint value)
        {
            if (writer == null || (writer.BitCount & 7) != 0)
                return JxrError.InvalidArgument;
            if (value < 0xfb00)
                writer.Write(value, 16);
            else
            {
                writer.Write(0xfb, 8);
                writer.Write(value >> 16, 16);
                writer.Write(value & 0xffff, 16);
            }
            return JxrError.None;
        }
    }
}
