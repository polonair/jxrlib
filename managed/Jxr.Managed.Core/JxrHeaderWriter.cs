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
            return WriteGraySpatial(writer, width, height, quantizerIndex,
                quantizerIndex, quantizerIndex, JxrGraySubbandMode.All, false, 0);
        }

        public static JxrError WriteGraySpatial(JxrBitWriter writer,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic, int trimFlexbits)
        {
            if (writer == null || width < 1 || height < 1 ||
                writer.BitCount != 0 || (int)subbands < 0 || (int)subbands > 3 ||
                trimFlexbits < 0 || trimFlexbits > 15)
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
            writer.Write(trimFlexbits != 0 ? 1U : 0U, 1);
            writer.Write(0, 1);  // no tile stretching
            writer.Write(0, 2);  // reserved and red/blue swap
            writer.Write(0, 1);  // no alpha
            writer.Write(0, 4);  // Y_ONLY source color format
            writer.Write(1, 4);  // 8-bit source depth
            writer.Write((uint)(width - 1), abbreviated ? 16 : 32);
            writer.Write((uint)(height - 1), abbreviated ? 16 : 32);
            writer.AlignByte();

            writer.Write(0, 3);  // Y_ONLY plane
            writer.Write(scaledArithmetic ? 1U : 0U, 1);
            writer.Write((uint)subbands, 4);
            writer.Write(1, 1); writer.Write(dcQuantizerIndex, 8); // DC frame QP
            if (subbands != JxrGraySubbandMode.DcOnly)
            {
                writer.Write(0, 1); writer.Write(1, 1);          // LP own frame QP
                writer.Write(lowpassQuantizerIndex, 8);
            }
            if ((int)subbands < (int)JxrGraySubbandMode.NoHighpass)
            {
                writer.Write(0, 1); writer.Write(1, 1);          // HP own frame QP
                writer.Write(highpassQuantizerIndex, 8);
            }
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
            return WriteGraySpatial(entropyPacket, width, height, quantizerIndex,
                quantizerIndex, quantizerIndex, JxrGraySubbandMode.All, false, 0,
                out codestream);
        }

        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, out byte[] codestream)
        {
            return WriteGraySpatial(entropyPacket, width, height,
                dcQuantizerIndex, lowpassQuantizerIndex, highpassQuantizerIndex,
                subbands, scaledArithmetic, trimFlexbits,
                entropyPacket == null ? 0 : entropyPacket.Length * 8,
                out codestream);
        }

        public static JxrError WriteGraySpatial(byte[] entropyPacket,
            int width, int height, byte dcQuantizerIndex,
            byte lowpassQuantizerIndex, byte highpassQuantizerIndex,
            JxrGraySubbandMode subbands, bool scaledArithmetic,
            int trimFlexbits, int entropyBitCount, out byte[] codestream)
        {
            codestream = null;
            if (entropyPacket == null || entropyBitCount < 0 ||
                entropyBitCount > (long)entropyPacket.Length * 8)
                return JxrError.InvalidArgument;
            JxrBitWriter writer = new JxrBitWriter();
            JxrError error = JxrHeaderWriter.WriteGraySpatial(writer,
                width, height, dcQuantizerIndex, lowpassQuantizerIndex,
                highpassQuantizerIndex, subbands, scaledArithmetic, trimFlexbits);
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
            if (trimFlexbits != 0) writer.Write((uint)trimFlexbits, 4);
            JxrBitReader entropyReader = new JxrBitReader(entropyPacket);
            int remainingBits = entropyBitCount;
            while (remainingBits > 0)
            {
                uint bit;
                error = entropyReader.ReadBits(1, out bit);
                if (error != JxrError.None) return error;
                error = writer.Write(bit, 1);
                if (error != JxrError.None) return error;
                remainingBits--;
            }
            writer.AlignByte();
            codestream = writer.ToArray();
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
