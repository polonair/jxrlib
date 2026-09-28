using System;

namespace Jxr.Managed.Core
{
    // Complete managed decode for the intentionally narrow 16x16 Y_ONLY,
    // QP=1, spatial, one-tile, OL_NONE fixture. Unsupported JPEG XR variants
    // are rejected instead of silently following the minimal path.
    public static class JxrMinimalDecoder
    {
        private static readonly int[] LocalSampleOrder = {
            0, 1, 5, 4, 2, 3, 7, 6,
            10, 11, 15, 14, 8, 9, 13, 12
        };

        public static JxrError DecodeGrayBmp(byte[] source, out byte[] bitmap)
        {
            bitmap = null;
            JxrHeaders headers;
            JxrError error = JxrHeaders.Read(source, out headers);
            if (error != JxrError.None) return error;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            if (main.Width != 16 || main.Height != 16 || main.Overlap != 0 ||
                main.BitstreamFormat != 0 || main.Orientation != 0 ||
                main.CodedBitDepth != 1 || main.SourceColorFormat != 0 ||
                main.HasAlpha || main.BlackWhite ||
                main.VerticalSliceCountMinusOne != 0 ||
                main.HorizontalSliceCountMinusOne != 0 ||
                main.HasIndexTable || main.TrimFlexbits ||
                main.ExtraTop != 0 || main.ExtraLeft != 0 ||
                main.ExtraBottom != 0 || main.ExtraRight != 0 ||
                plane.ColorFormat != 0 || plane.ChannelCount != 1 ||
                plane.Subband != 0 || plane.ScaledArithmetic ||
                main.SourceBitDepth != 1 || q.Mode != 0x600 ||
                q.GetDcIndex(0) != 0 || q.GetLowpassIndex(0) != 0 ||
                q.GetHighpassIndex(0) != 0)
                return JxrError.UnsupportedFeature;

            int packetOffset;
            error = LocateSingleSpatialPacket(source, headers, out packetOffset);
            if (error != JxrError.None) return error;
            if (packetOffset > Int32.MaxValue / 8)
                return JxrError.UnsupportedFeature;
            JxrBitReader reader = new JxrBitReader(source);
            int remaining = packetOffset * 8;
            while (remaining > 0)
            {
                int count = remaining > 32 ? 32 : remaining;
                error = reader.ConsumeBits(count);
                if (error != JxrError.None) return error;
                remaining -= count;
            }
            JxrPacketHeader packet;
            error = JxrPacketReader.ReadHeader(reader, out packet);
            if (error != JxrError.None) return error;
            if (!packet.IsValid || packet.TileId != 0 || packet.PacketType != 0)
                return JxrError.InvalidBitstream;

            JxrSessionConfiguration sessionConfig;
            error = JxrSessionPlanner.FromHeaders(headers, out sessionConfig);
            if (error != JxrError.None) return error;
            using (JxrDecoderSession session =
                JxrDecoderSession.Create(sessionConfig, 0, 0, 0))
            {
                JxrCodecConfiguration format = new JxrCodecConfiguration(
                    JxrCodecColorFormat.YOnly, 1, true, false, true, true,
                    false, true, true, false, false, 0, 0, 1, 1,
                    new int[][] { new int[] { 1 } });
                JxrCodecState state = new JxrCodecState(format,
                    reader, reader, reader, reader);
                error = JxrDcCodec.Decode(state);
                if (error != JxrError.None) return error;
                error = JxrLpCodec.Decode(state);
                if (error != JxrError.None) return error;
                JxrCoefficientPredictionRows rows =
                    new JxrCoefficientPredictionRows(1, 1);
                error = JxrCoefficientPrediction.DecodeDcLp(state.Macroblock,
                    rows, JxrCodecColorFormat.YOnly, 0, true, true);
                if (error != JxrError.None) return error;
                error = JxrCoefficientPrediction.StoreCurrent(state.Macroblock,
                    rows, JxrCodecColorFormat.YOnly, 0);
                if (error != JxrError.None) return error;
                JxrQuantizer lossless = JxrQuantization.Remap(0, false, false);
                JxrQuantizerSet quantizers = new JxrQuantizerSet(
                    new JxrQuantizer[] { lossless.WithDcOffset() },
                    new JxrQuantizer[][] { new JxrQuantizer[] { lossless } },
                    new JxrQuantizer[][] { new JxrQuantizer[] { lossless } });
                error = JxrQuantization.DequantizeMacroblock(state.CoefficientPlanes,
                    state.Macroblock, quantizers, JxrCodecColorFormat.YOnly,
                    1, false);
                if (error != JxrError.None) return error;
                error = JxrHpCodec.Decode(state);
                if (error != JxrError.None) return error;
                error = JxrCoefficientPrediction.DecodeAc(state.Macroblock,
                    state.CoefficientPlanes, JxrCodecColorFormat.YOnly);
                if (error != JxrError.None) return error;
                int[] coefficients;
                error = state.CoefficientPlanes.GetPlane(0, out coefficients);
                if (error != JxrError.None) return error;
                int[] samples = session.GetPrimaryRow(0, 0);
                Array.Copy(coefficients, samples, 256);
                InverseMacroblock(samples);
                byte[] gray = new byte[256];
                for (int y = 0; y < 16; y++)
                    for (int x = 0; x < 16; x++)
                    {
                        int block = (x >> 2) * 64 + (y >> 2) * 16;
                        int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                        gray[y * 16 + x] =
                            JxrImagePipeline.ClipByte(unchecked(samples[block + local] + 128));
                    }
                bitmap = WriteGrayBmp(gray);
                return JxrError.None;
            }
        }

        private static JxrError LocateSingleSpatialPacket(byte[] source,
            JxrHeaders headers, out int offset)
        {
            offset = 0;
            int position = headers.CodestreamOffset + headers.ByteCount;
            if (position < 0 || position > source.Length - 2)
                return JxrError.UnexpectedEndOfStream;
            // JxrIndexTableReader always reads a variable-length word even in
            // streaming mode. This profile uses its two-byte form.
            int first = source[position];
            if (first >= 0xfb) return JxrError.UnsupportedFeature;
            int headerSize = (first << 8) | source[position + 1];
            long packetPosition = (long)position + 2 + headerSize;
            if (packetPosition + 4 > source.Length)
                return JxrError.UnexpectedEndOfStream;
            offset = (int)packetPosition;
            return JxrError.None;
        }

        private static void InverseMacroblock(int[] values)
        {
            // Stage 2: the 16 DC/LP values are embedded at native offsets.
            JxrInverseTransformMath.ApplyOdd(
                ref values[32], ref values[48], ref values[96], ref values[112]);
            JxrInverseTransformMath.ApplyOdd(
                ref values[128], ref values[192], ref values[144], ref values[208]);
            JxrInverseTransformMath.ApplyOddOdd(
                ref values[160], ref values[224], ref values[176], ref values[240]);
            JxrTransformMath.ApplyDct2x2Up(values, 0, 64, 16, 80);
            JxrTransformMath.ApplySecondStageFourButterfly(values);

            // Stage 1 is independent for each 4x4 coefficient block with no overlap.
            for (int block = 0; block < 16; block++)
            {
                int offset = block * 16;
                int[] work = new int[16];
                Array.Copy(values, offset, work, 0, 16);
                JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
                JxrInverseTransformMath.ApplyOdd(
                    ref work[5], ref work[4], ref work[7], ref work[6]);
                JxrInverseTransformMath.ApplyOdd(
                    ref work[10], ref work[8], ref work[11], ref work[9]);
                JxrInverseTransformMath.ApplyOddOdd(
                    ref work[15], ref work[14], ref work[13], ref work[12]);
                JxrTransformMath.ApplyFirstStageFourButterfly(work);
                Array.Copy(work, 0, values, offset, 16);
            }
        }

        private static byte[] WriteGrayBmp(byte[] pixels)
        {
            byte[] bmp = new byte[14 + 40 + 1024 + 256];
            bmp[0] = (byte)'B'; bmp[1] = (byte)'M';
            Write32(bmp, 2, bmp.Length);
            Write32(bmp, 10, 1078);
            Write32(bmp, 14, 40);
            Write32(bmp, 18, 16);
            Write32(bmp, 22, 16);
            Write16(bmp, 26, 1);
            Write16(bmp, 28, 8);
            Write32(bmp, 34, 256);
            Write32(bmp, 38, 3779);
            Write32(bmp, 42, 3779);
            for (int index = 0; index < 256; index++)
            {
                int entry = 54 + index * 4;
                bmp[entry] = bmp[entry + 1] = bmp[entry + 2] = (byte)index;
            }
            for (int row = 0; row < 16; row++)
                Array.Copy(pixels, row * 16, bmp, 1078 + (15 - row) * 16, 16);
            return bmp;
        }

        private static void Write16(byte[] buffer, int offset, int value)
        {
            buffer[offset] = (byte)value;
            buffer[offset + 1] = (byte)(value >> 8);
        }

        private static void Write32(byte[] buffer, int offset, int value)
        {
            buffer[offset] = (byte)value;
            buffer[offset + 1] = (byte)(value >> 8);
            buffer[offset + 2] = (byte)(value >> 16);
            buffer[offset + 3] = (byte)(value >> 24);
        }
    }
}
