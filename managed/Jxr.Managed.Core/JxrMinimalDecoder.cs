using System;

namespace Jxr.Managed.Core
{
    // Managed pixel decode for Y_ONLY, QP=1, spatial, one-tile, OL_NONE.
    // Macroblocks share one entropy context and two prediction rows.
    // Unsupported JPEG XR variants
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
            JxrImage image;
            JxrError error = JxrCodec.Decode(source,
                new JxrDecoderOptions(), out image);
            if (error != JxrError.None) return error;
            return JxrBmpAdapter.WriteGray8(image, out bitmap);
        }

        internal static JxrError DecodeGrayPixels(byte[] source, out byte[] pixels,
            out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            JxrHeaders headers;
            JxrError error = JxrHeaders.Read(source, out headers);
            if (error != JxrError.None) return error;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            if (main.Width < 1 || main.Height < 1 ||
                main.Width > Int32.MaxValue - 15 ||
                main.Height > Int32.MaxValue - 15 ||
                (long)main.Width * main.Height > Int32.MaxValue ||
                main.Overlap > 2 ||
                main.BitstreamFormat > 1 || main.Orientation != 0 ||
                main.CodedBitDepth != 1 || main.SourceColorFormat != 0 ||
                main.HasAlpha || main.BlackWhite ||
                main.ExtraTop != 0 || main.ExtraLeft != 0 ||
                main.ExtraBottom != ((16 - ((int)main.Height & 15)) & 15) ||
                main.ExtraRight != ((16 - ((int)main.Width & 15)) & 15) ||
                plane.ColorFormat != 0 || plane.ChannelCount != 1 ||
                plane.Subband < 0 || plane.Subband > 3 ||
                main.SourceBitDepth != 1 ||
                q.Mode != (plane.Subband == (int)JxrGraySubbandMode.DcOnly ? 0x200 : 0x600) ||
                !q.HasDc || (plane.Subband != 3 && !q.HasLowpass) ||
                (plane.Subband < 2 && !q.HasHighpass))
                return JxrError.UnsupportedFeature;

            if (main.HasIndexTable || main.VerticalSliceCountMinusOne != 0 ||
                main.HorizontalSliceCountMinusOne != 0)
                return DecodeGrayTiles(source, headers, out pixels, out width,
                    out height);

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
            int trimFlexbits = 0;
            if (main.TrimFlexbits)
            {
                uint trim;
                error = reader.ReadBits(4, out trim);
                if (error != JxrError.None) return error;
                trimFlexbits = (int)trim;
            }

            JxrSessionConfiguration sessionConfig;
            error = JxrSessionPlanner.FromHeaders(headers, out sessionConfig);
            if (error != JxrError.None) return error;
            using (JxrDecoderSession session =
                JxrDecoderSession.Create(sessionConfig, 0, 0, 0))
            {
                bool dcOnly = plane.Subband == (int)JxrGraySubbandMode.DcOnly;
                bool scaledArithmetic = plane.ScaledArithmetic;
                bool hasHighpass = plane.Subband < (int)JxrGraySubbandMode.NoHighpass;
                bool skipFlexbits = plane.Subband == (int)JxrGraySubbandMode.NoFlexbits;
                JxrCodecConfiguration format = new JxrCodecConfiguration(
                    JxrCodecColorFormat.YOnly, 1, true, dcOnly, hasHighpass,
                    !skipFlexbits, skipFlexbits, true, true, false, false,
                    0, 0, 1, 1,
                    new int[][] { new int[] { JxrQuantization.Remap(
                        q.GetHighpassIndex(0), plane.ScaledArithmetic, false).Parameter } });
                JxrCodecState state = new JxrCodecState(format,
                    reader, reader, reader, reader);
                state.Entropy.TrimFlexBits = trimFlexbits;
                int imageWidth = (int)main.Width, imageHeight = (int)main.Height;
                int columns = (imageWidth + 15) / 16;
                int rowsCount = (imageHeight + 15) / 16;
                JxrCoefficientPredictionRows rows =
                    new JxrCoefficientPredictionRows(columns, 1);
                int[] topCbp = new int[columns];
                byte[] gray = new byte[imageWidth * imageHeight];
                int[][] overlapCoefficients = main.Overlap == 0 ? null :
                    new int[columns * rowsCount][];
                JxrQuantizer dcQuantizer = JxrQuantization.Remap(
                    q.GetDcIndex(0), plane.ScaledArithmetic, false);
                JxrQuantizer lpQuantizer = JxrQuantization.Remap(
                    plane.Subband == 3 ? q.GetDcIndex(0) : q.GetLowpassIndex(0),
                    plane.ScaledArithmetic, false);
                JxrQuantizer hpQuantizer = JxrQuantization.Remap(
                    plane.Subband < 2 ? q.GetHighpassIndex(0) : q.GetDcIndex(0),
                    plane.ScaledArithmetic, false);
                JxrQuantizerSet quantizers = new JxrQuantizerSet(
                    new JxrQuantizer[] { dcQuantizer.WithDcOffset() },
                    new JxrQuantizer[][] { new JxrQuantizer[] { lpQuantizer } },
                    new JxrQuantizer[][] { new JxrQuantizer[] { hpQuantizer } });
                for (int mbY = 0; mbY < rowsCount; mbY++)
                {
                    if (mbY != 0) rows.AdvanceRow();
                    int leftCbp = 0;
                    for (int mbX = 0; mbX < columns; mbX++)
                    {
                        state.SetMacroblockPosition(mbX, mbY, columns);
                        state.SetNeighborCbp(0, topCbp[mbX], leftCbp);
                        int[] coefficients;
                        error = state.CoefficientPlanes.GetPlane(0, out coefficients);
                        if (error != JxrError.None) return error;
                        Array.Clear(coefficients, 0, coefficients.Length);
                        error = JxrDcCodec.Decode(state);
                        if (error != JxrError.None) return error;
                        if (!dcOnly)
                        {
                            error = JxrLpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                        }
                        error = JxrCoefficientPrediction.DecodeDcLp(state.Macroblock,
                            rows, JxrCodecColorFormat.YOnly, mbX, mbX == 0, mbY == 0);
                        if (error != JxrError.None) return error;
                        error = JxrCoefficientPrediction.StoreCurrent(state.Macroblock,
                            rows, JxrCodecColorFormat.YOnly, mbX);
                        if (error != JxrError.None) return error;
                        error = JxrQuantization.DequantizeMacroblock(state.CoefficientPlanes,
                            state.Macroblock, quantizers, JxrCodecColorFormat.YOnly,
                            1, dcOnly);
                        if (error != JxrError.None) return error;
                        if (hasHighpass)
                        {
                            error = JxrHpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                            int cbp;
                            error = state.MacroblockCbp.GetCbp(0, out cbp);
                            if (error != JxrError.None) return error;
                            leftCbp = topCbp[mbX] = cbp;
                        }
                        error = JxrCoefficientPrediction.DecodeAc(state.Macroblock,
                            state.CoefficientPlanes, JxrCodecColorFormat.YOnly);
                        if (error != JxrError.None) return error;
                        int[] samples = new int[256];
                        Array.Copy(coefficients, samples, 256);
                        if (overlapCoefficients != null)
                            overlapCoefficients[mbY * columns + mbX] = samples;
                        else
                        {
                            InverseMacroblock(samples);
                            CopyGrayMacroblock(samples, gray, imageWidth,
                                imageHeight, mbX, mbY, scaledArithmetic);
                        }
                    }
                }
                if (overlapCoefficients != null)
                {
                    int[][] overlapSamples = JxrOverlapInverse.Transform(
                        overlapCoefficients, imageWidth, imageHeight,
                        main.Overlap, false, hpQuantizer.Parameter, !hasHighpass);
                    for (int mbY = 0; mbY < rowsCount; mbY++)
                        for (int mbX = 0; mbX < columns; mbX++)
                            CopyGrayMacroblock(overlapSamples[mbY * columns + mbX],
                                gray, imageWidth, imageHeight, mbX, mbY,
                                scaledArithmetic);
                }
                pixels = gray;
                width = imageWidth; height = imageHeight;
                return JxrError.None;
            }
        }

        private static JxrError DecodeGrayTiles(byte[] source, JxrHeaders headers,
            out byte[] pixels, out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            JxrPacketIndexTable index;
            JxrError error = JxrPacketIndexTable.Read(source, headers, out index);
            if (error != JxrError.None) return error;
            int imageWidth = (int)main.Width;
            int imageHeight = (int)main.Height;
            int columns = (imageWidth + 15) / 16;
            int rowsCount = (imageHeight + 15) / 16;
            int tileColumns = main.VerticalSliceCountMinusOne + 1;
            int tileRows = main.HorizontalSliceCountMinusOne + 1;
            int[] tileX, tileY;
            error = BuildTileBoundaries(main, columns, rowsCount,
                out tileX, out tileY);
            if (error != JxrError.None) return error;
            bool frequency = main.BitstreamFormat == 1;
            int packetBandCount = plane.Subband == 3 ? 1 :
                plane.Subband == 2 ? 2 : plane.Subband == 1 ? 3 : 4;
            if (index.PacketCount != tileColumns * tileRows *
                (frequency ? packetBandCount : 1))
                return JxrError.InvalidBitstream;

            bool dcOnly = plane.Subband == (int)JxrGraySubbandMode.DcOnly;
            bool scaled = plane.ScaledArithmetic;
            bool hasHighpass = plane.Subband < (int)JxrGraySubbandMode.NoHighpass;
            bool skipFlexbits = plane.Subband == (int)JxrGraySubbandMode.NoFlexbits;
            JxrQuantizer dcQuantizer = JxrQuantization.Remap(
                q.GetDcIndex(0), scaled, false);
            JxrQuantizer lpQuantizer = JxrQuantization.Remap(
                plane.Subband == 3 ? q.GetDcIndex(0) : q.GetLowpassIndex(0),
                scaled, false);
            JxrQuantizer hpQuantizer = JxrQuantization.Remap(
                plane.Subband < 2 ? q.GetHighpassIndex(0) : q.GetDcIndex(0),
                scaled, false);
            JxrQuantizerSet quantizers = new JxrQuantizerSet(
                new JxrQuantizer[] { dcQuantizer.WithDcOffset() },
                new JxrQuantizer[][] { new JxrQuantizer[] { lpQuantizer } },
                new JxrQuantizer[][] { new JxrQuantizer[] { hpQuantizer } });
            byte[] gray = new byte[imageWidth * imageHeight];
            int[][] globalCoefficients = main.Overlap == 0 ||
                main.HasHardTileBoundaries ? null : new int[columns * rowsCount][];

            for (int tileRow = 0; tileRow < tileRows; tileRow++)
                for (int tileColumn = 0; tileColumn < tileColumns; tileColumn++)
                {
                    int startMbX = tileX[tileColumn];
                    int startMbY = tileY[tileRow];
                    int tileWidth = tileX[tileColumn + 1] - startMbX;
                    int tileHeight = tileY[tileRow + 1] - startMbY;
                    int tileId = (tileRow * tileColumns + tileColumn) & 31;
                    int trimFlexbits = 0;
                    JxrBitReader dcReader, lpReader, hpReader, flexReader;
                    if (frequency)
                    {
                        error = ReadFrequencyReader(index, source, headers,
                            tileRow, tileColumn, 0, tileId, out dcReader);
                        if (error != JxrError.None) return error;
                        lpReader = dcReader; hpReader = dcReader; flexReader = dcReader;
                        if (packetBandCount > 1)
                        {
                            error = ReadFrequencyReader(index, source, headers,
                                tileRow, tileColumn, 1, tileId, out lpReader);
                            if (error != JxrError.None) return error;
                        }
                        if (packetBandCount > 2)
                        {
                            error = ReadFrequencyReader(index, source, headers,
                                tileRow, tileColumn, 2, tileId, out hpReader);
                            if (error != JxrError.None) return error;
                        }
                        if (packetBandCount > 3)
                        {
                            error = ReadFrequencyReader(index, source, headers,
                                tileRow, tileColumn, 3, tileId, out flexReader);
                            if (error != JxrError.None) return error;
                            if (main.TrimFlexbits)
                            {
                                uint trim;
                                error = flexReader.ReadBits(4, out trim);
                                if (error != JxrError.None) return error;
                                trimFlexbits = (int)trim;
                            }
                        }
                    }
                    else
                    {
                        byte[] packetBytes;
                        error = index.ReadPacket(source, headers, tileRow,
                            tileColumn, out packetBytes);
                        if (error != JxrError.None) return error;
                        dcReader = new JxrBitReader(packetBytes);
                        JxrPacketHeader packet;
                        error = JxrPacketReader.ReadHeader(dcReader, out packet);
                        if (error != JxrError.None) return error;
                        if (!packet.IsValid || packet.TileId != tileId ||
                            packet.PacketType != 0) return JxrError.InvalidBitstream;
                        lpReader = hpReader = flexReader = dcReader;
                        if (main.TrimFlexbits)
                        {
                            uint trim;
                            error = dcReader.ReadBits(4, out trim);
                            if (error != JxrError.None) return error;
                            trimFlexbits = (int)trim;
                        }
                    }

                    JxrCodecConfiguration format = new JxrCodecConfiguration(
                        JxrCodecColorFormat.YOnly, 1, !frequency, dcOnly, hasHighpass,
                        !skipFlexbits, skipFlexbits, true, true, false, false,
                        0, 0, 1, 1,
                        new int[][] { new int[] { hpQuantizer.Parameter } });
                    JxrCodecState state = new JxrCodecState(format,
                        dcReader, lpReader, hpReader, flexReader);
                    state.Entropy.TrimFlexBits = trimFlexbits;
                    JxrCoefficientPredictionRows coefficientRows =
                        new JxrCoefficientPredictionRows(tileWidth, 1);
                    int[] topCbp = new int[tileWidth];
                    int[][] tileCoefficients = main.Overlap != 0 &&
                        main.HasHardTileBoundaries ?
                        new int[tileWidth * tileHeight][] : null;
                    int tilePixelWidth = Math.Min(imageWidth - startMbX * 16,
                        tileWidth * 16);
                    int tilePixelHeight = Math.Min(imageHeight - startMbY * 16,
                        tileHeight * 16);

                    for (int localY = 0; localY < tileHeight; localY++)
                    {
                        if (localY != 0) coefficientRows.AdvanceRow();
                        int leftCbp = 0;
                        for (int localX = 0; localX < tileWidth; localX++)
                        {
                            state.SetMacroblockPosition(localX, localY, tileWidth);
                            state.SetNeighborCbp(0, topCbp[localX], leftCbp);
                            int[] coefficients;
                            error = state.CoefficientPlanes.GetPlane(0,
                                out coefficients);
                            if (error != JxrError.None) return error;
                            Array.Clear(coefficients, 0, coefficients.Length);
                            error = JxrDcCodec.Decode(state);
                            if (error != JxrError.None) return error;
                            if (!dcOnly)
                            {
                                error = JxrLpCodec.Decode(state);
                                if (error != JxrError.None) return error;
                            }
                            error = JxrCoefficientPrediction.DecodeDcLp(
                                state.Macroblock, coefficientRows,
                                JxrCodecColorFormat.YOnly, localX,
                                localX == 0, localY == 0);
                            if (error != JxrError.None) return error;
                            error = JxrCoefficientPrediction.StoreCurrent(
                                state.Macroblock, coefficientRows,
                                JxrCodecColorFormat.YOnly, localX);
                            if (error != JxrError.None) return error;
                            error = JxrQuantization.DequantizeMacroblock(
                                state.CoefficientPlanes, state.Macroblock,
                                quantizers, JxrCodecColorFormat.YOnly, 1,
                                dcOnly);
                            if (error != JxrError.None) return error;
                            if (hasHighpass)
                            {
                                error = JxrHpCodec.Decode(state);
                                if (error != JxrError.None) return error;
                                error = state.MacroblockCbp.GetCbp(0, out leftCbp);
                                if (error != JxrError.None) return error;
                                topCbp[localX] = leftCbp;
                            }
                            error = JxrCoefficientPrediction.DecodeAc(
                                state.Macroblock, state.CoefficientPlanes,
                                JxrCodecColorFormat.YOnly);
                            if (error != JxrError.None) return error;
                            int[] samples = new int[256];
                            Array.Copy(coefficients, samples, 256);
                            int globalX = startMbX + localX;
                            int globalY = startMbY + localY;
                            if (main.Overlap == 0)
                            {
                                InverseMacroblock(samples);
                                CopyGrayMacroblock(samples, gray, imageWidth,
                                    imageHeight, globalX, globalY, scaled);
                            }
                            else if (main.HasHardTileBoundaries)
                                tileCoefficients[localY * tileWidth + localX] = samples;
                            else
                                globalCoefficients[globalY * columns + globalX] = samples;
                        }
                    }
                    if (tileCoefficients != null)
                    {
                        int[][] tileSamples = JxrOverlapInverse.Transform(
                            tileCoefficients, tilePixelWidth, tilePixelHeight,
                            main.Overlap, false, hpQuantizer.Parameter,
                            !hasHighpass);
                        for (int localY = 0; localY < tileHeight; localY++)
                            for (int localX = 0; localX < tileWidth; localX++)
                                CopyGrayMacroblock(
                                    tileSamples[localY * tileWidth + localX],
                                    gray, imageWidth, imageHeight,
                                    startMbX + localX, startMbY + localY, scaled);
                    }
                }

            if (globalCoefficients != null)
            {
                int[][] overlapSamples = JxrOverlapInverse.Transform(
                    globalCoefficients, imageWidth, imageHeight, main.Overlap,
                    false, hpQuantizer.Parameter, !hasHighpass);
                for (int mbY = 0; mbY < rowsCount; mbY++)
                    for (int mbX = 0; mbX < columns; mbX++)
                        CopyGrayMacroblock(overlapSamples[mbY * columns + mbX],
                            gray, imageWidth, imageHeight, mbX, mbY, scaled);
            }
            pixels = gray;
            width = imageWidth;
            height = imageHeight;
            return JxrError.None;
        }

        private static JxrError ReadFrequencyReader(JxrPacketIndexTable index,
            byte[] source, JxrHeaders headers, int tileRow, int tileColumn,
            int band, int expectedTileId, out JxrBitReader reader)
        {
            reader = null;
            byte[] packet;
            JxrError error = index.ReadFrequencyPacket(source, headers,
                tileRow, tileColumn, band, out packet);
            if (error != JxrError.None) return error;
            if (packet == null && band == 3)
            {
                reader = new JxrBitReader(new byte[8]);
                return JxrError.None;
            }
            if (packet.Length < 4) return JxrError.InvalidBitstream;
            if (packet.Length == Int32.MaxValue) return JxrError.UnsupportedFeature;
            byte[] paddedPacket = new byte[packet.Length + 1];
            Array.Copy(packet, paddedPacket, packet.Length);
            reader = new JxrBitReader(paddedPacket);
            JxrPacketHeader header;
            error = JxrPacketReader.ReadHeader(reader, out header);
            if (error != JxrError.None) return error;
            if (!header.IsValid || header.TileId != expectedTileId ||
                header.PacketType != band + 1) return JxrError.InvalidBitstream;
            return JxrError.None;
        }

        // Full-resolution three-plane path used by RGB24 images encoded as
        // YUV444.  All three component planes share one spatial entropy
        // stream, prediction state and macroblock position.
        internal static JxrError DecodeRgbPixels(byte[] source, bool rgbOrder,
            out byte[] pixels,
            out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            JxrHeaders headers;
            JxrError error = JxrHeaders.Read(source, out headers);
            if (error != JxrError.None) return error;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            if (main.Width < 1 || main.Height < 1 ||
                main.Width > Int32.MaxValue - 15 || main.Height > Int32.MaxValue - 15 ||
                (long)main.Width * main.Height > Int32.MaxValue ||
                main.Overlap > 2 || main.BitstreamFormat > 1 || main.Orientation != 0 ||
                main.CodedBitDepth != 1 || main.HasAlpha || main.BlackWhite ||
                main.ExtraTop != 0 || main.ExtraLeft != 0 ||
                main.ExtraBottom != ((16 - ((int)main.Height & 15)) & 15) ||
                main.ExtraRight != ((16 - ((int)main.Width & 15)) & 15) ||
                (plane.ColorFormat != (int)JxrCodecColorFormat.Yuv444 &&
                 plane.ColorFormat != (int)JxrCodecColorFormat.Yuv422 &&
                 plane.ColorFormat != (int)JxrCodecColorFormat.Yuv420) ||
                plane.ChannelCount != 3 || plane.Subband < 0 || plane.Subband > 3 ||
                main.SourceBitDepth != 1 ||
                !q.HasDc || (plane.Subband != 3 && !q.HasLowpass) ||
                (plane.Subband < 2 && !q.HasHighpass))
                return JxrError.UnsupportedFeature;

            if (main.HasIndexTable || main.VerticalSliceCountMinusOne != 0 ||
                main.HorizontalSliceCountMinusOne != 0)
                return DecodeRgbTiles(source, headers, rgbOrder, out pixels,
                    out width, out height);

            int packetOffset;
            error = LocateSingleSpatialPacket(source, headers, out packetOffset);
            if (error != JxrError.None) return error;
            if (packetOffset > Int32.MaxValue / 8) return JxrError.UnsupportedFeature;
            // Native packet bit I/O keeps a zero-filled lookahead word after
            // the final byte. The color HP decoder may need that lookahead
            // to resolve a short final Huffman symbol, but may not consume
            // beyond the physical input. Validate that limit after decoding.
            if (source.Length == Int32.MaxValue) return JxrError.UnsupportedFeature;
            byte[] paddedSource = new byte[source.Length + 1];
            Array.Copy(source, paddedSource, source.Length);
            JxrBitReader reader = new JxrBitReader(paddedSource);
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
            int trimFlexbits = 0;
            if (main.TrimFlexbits)
            {
                uint trim;
                error = reader.ReadBits(4, out trim);
                if (error != JxrError.None) return error;
                trimFlexbits = (int)trim;
            }

            JxrSessionConfiguration sessionConfig;
            error = JxrSessionPlanner.FromHeaders(headers, out sessionConfig);
            if (error != JxrError.None) return error;
            using (JxrDecoderSession session = JxrDecoderSession.Create(
                sessionConfig, 0, 0, 0))
            {
                JxrCodecColorFormat colorFormat =
                    (JxrCodecColorFormat)plane.ColorFormat;
                bool subsampled = colorFormat == JxrCodecColorFormat.Yuv422 ||
                    colorFormat == JxrCodecColorFormat.Yuv420;
                bool dcOnly = plane.Subband == (int)JxrGraySubbandMode.DcOnly;
                bool scaledArithmetic = plane.ScaledArithmetic;
                bool hasHighpass = plane.Subband < (int)JxrGraySubbandMode.NoHighpass;
                bool skipFlexbits = plane.Subband == (int)JxrGraySubbandMode.NoFlexbits;
                int[][] hpParameters = new int[3][];
                JxrQuantizer[] dc = new JxrQuantizer[3];
                JxrQuantizer[][] lp = new JxrQuantizer[3][];
                JxrQuantizer[][] hp = new JxrQuantizer[3][];
                for (int channel = 0; channel < 3; channel++)
                {
                    byte dcIndex = q.GetDcIndex(channel);
                    byte lpIndex = plane.Subband == 3 ? dcIndex : q.GetLowpassIndex(channel);
                    byte hpIndex = plane.Subband < 2 ? q.GetHighpassIndex(channel) : dcIndex;
                    dc[channel] = JxrQuantization.Remap(dcIndex, scaledArithmetic, channel != 0).WithDcOffset();
                    lp[channel] = new JxrQuantizer[] {
                        JxrQuantization.Remap(lpIndex, scaledArithmetic, channel != 0) };
                    hp[channel] = new JxrQuantizer[] {
                        JxrQuantization.Remap(hpIndex, scaledArithmetic, false) };
                    hpParameters[channel] = new int[] { hp[channel][0].Parameter };
                }
                JxrQuantizerSet quantizers = new JxrQuantizerSet(dc, lp, hp);
                JxrCodecConfiguration format = new JxrCodecConfiguration(
                    colorFormat, 3, true, dcOnly, hasHighpass,
                    !skipFlexbits, skipFlexbits, true, true, false, false,
                    0, 0, 1, 1, hpParameters);
                JxrCodecState state = new JxrCodecState(format,
                    reader, reader, reader, reader);
                state.Entropy.TrimFlexBits = trimFlexbits;
                int imageWidth = (int)main.Width, imageHeight = (int)main.Height;
                int columns = (imageWidth + 15) / 16;
                int rowsCount = (imageHeight + 15) / 16;
                JxrCoefficientPredictionRows rows =
                    new JxrCoefficientPredictionRows(columns, 3);
                int[][] topCbp = { new int[columns], new int[columns], new int[columns] };
                int[][] planes = new int[3][];
                for (int channel = 0; channel < 3; channel++)
                    state.CoefficientPlanes.GetPlane(channel, out planes[channel]);
                int count = imageWidth * imageHeight;
                int chromaWidth = columns * 8;
                int chromaHeight = rowsCount *
                    (colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
                int chromaCount = subsampled ? chromaWidth * chromaHeight : count;
                int[] yPlane = new int[count], uPlane = new int[chromaCount],
                    vPlane = new int[chromaCount];
                int[][] outputPlanes = { yPlane, uPlane, vPlane };
                int[][][] overlapCoefficients = main.Overlap == 0 ? null :
                    new int[3][][];
                if (overlapCoefficients != null)
                    for (int channel = 0; channel < 3; channel++)
                        overlapCoefficients[channel] = new int[columns * rowsCount][];
                for (int mbY = 0; mbY < rowsCount; mbY++)
                {
                    if (mbY != 0) rows.AdvanceRow();
                    int[] leftCbp = new int[3];
                    for (int mbX = 0; mbX < columns; mbX++)
                    {
                        state.SetMacroblockPosition(mbX, mbY, columns);
                        for (int channel = 0; channel < 3; channel++)
                        {
                            state.SetNeighborCbp(channel, topCbp[channel][mbX], leftCbp[channel]);
                            Array.Clear(planes[channel], 0, planes[channel].Length);
                        }
                        error = JxrDcCodec.Decode(state);
                        if (error != JxrError.None) return error;
                        if (!dcOnly)
                        {
                            error = JxrLpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                        }
                        error = JxrCoefficientPrediction.DecodeDcLp(state.Macroblock,
                            rows, colorFormat, mbX, mbX == 0, mbY == 0);
                        if (error != JxrError.None) return error;
                        error = JxrCoefficientPrediction.StoreCurrent(state.Macroblock,
                            rows, colorFormat, mbX);
                        if (error != JxrError.None) return error;
                        error = JxrQuantization.DequantizeMacroblock(state.CoefficientPlanes,
                            state.Macroblock, quantizers, colorFormat,
                            3, dcOnly);
                        if (error != JxrError.None) return error;
                        if (hasHighpass)
                        {
                            error = JxrHpCodec.Decode(state);
                            if (error != JxrError.None) return error;
                            for (int channel = 0; channel < 3; channel++)
                            {
                                error = state.MacroblockCbp.GetCbp(channel, out leftCbp[channel]);
                                if (error != JxrError.None) return error;
                                topCbp[channel][mbX] = leftCbp[channel];
                            }
                        }
                        error = JxrCoefficientPrediction.DecodeAc(state.Macroblock,
                            state.CoefficientPlanes, colorFormat);
                        if (error != JxrError.None) return error;
                        for (int channel = 0; channel < 3; channel++)
                        {
                            int[] samples = new int[planes[channel].Length];
                            Array.Copy(planes[channel], samples, samples.Length);
                            if (overlapCoefficients != null)
                                overlapCoefficients[channel][mbY * columns + mbX] = samples;
                            else
                            {
                                if (channel == 0 || !subsampled)
                                {
                                    InverseMacroblock(samples,
                                        channel != 0 && scaledArithmetic);
                                    CopyColorMacroblock(samples, outputPlanes[channel],
                                        imageWidth, imageHeight, mbX, mbY, 16, 16);
                                }
                                else
                                {
                                    InverseChromaMacroblock(samples, colorFormat,
                                        scaledArithmetic);
                                    CopyColorMacroblock(samples, outputPlanes[channel],
                                        chromaWidth, chromaHeight, mbX, mbY, 8,
                                        colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
                                }
                            }
                        }
                    }
                }
                if (overlapCoefficients != null)
                    for (int channel = 0; channel < 3; channel++)
                    {
                        int[][] overlapSamples = subsampled && channel != 0 ?
                            JxrChromaOverlapInverse.Transform(
                                overlapCoefficients[channel], columns, rowsCount,
                                colorFormat, main.Overlap, scaledArithmetic) :
                            JxrOverlapInverse.Transform(
                                overlapCoefficients[channel], imageWidth, imageHeight,
                                main.Overlap, channel != 0 && scaledArithmetic,
                                hp[channel][0].Parameter, !hasHighpass);
                        for (int mbY = 0; mbY < rowsCount; mbY++)
                            for (int mbX = 0; mbX < columns; mbX++)
                                CopyColorMacroblock(
                                    overlapSamples[mbY * columns + mbX],
                                    outputPlanes[channel],
                                    subsampled && channel != 0 ? chromaWidth : imageWidth,
                                    subsampled && channel != 0 ? chromaHeight : imageHeight,
                                    mbX, mbY,
                                    subsampled && channel != 0 ? 8 : 16,
                                    subsampled && channel != 0 &&
                                        colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
                    }
                if (subsampled)
                {
                    int paddedWidth = columns * 16;
                    int paddedHeight = rowsCount * 16;
                    bool vertical = colorFormat == JxrCodecColorFormat.Yuv420;
                    uPlane = JxrChromaResampler.Interpolate(uPlane, imageWidth,
                        imageHeight, paddedWidth, paddedHeight, vertical);
                    vPlane = JxrChromaResampler.Interpolate(vPlane, imageWidth,
                        imageHeight, paddedWidth, paddedHeight, vertical);
                }
                pixels = new byte[imageWidth * imageHeight * 3];
                error = JxrImagePipeline.DecodeRgb8(yPlane, uPlane, vPlane,
                    imageWidth, imageHeight, rgbOrder, scaledArithmetic ? 3 : 0,
                    pixels, imageWidth * 3);
                if (error != JxrError.None) { pixels = null; return error; }
                if (reader.BitPosition > (long)source.Length * 8)
                { pixels = null; return JxrError.UnexpectedEndOfStream; }
                width = imageWidth; height = imageHeight;
                return JxrError.None;
            }
        }

        private static JxrError DecodeRgbTiles(byte[] source, JxrHeaders headers,
            bool rgbOrder, out byte[] pixels, out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            JxrMainHeader main = headers.Main;
            JxrImagePlaneHeader plane = headers.Plane;
            JxrImagePlaneQuantizerHeader q = headers.Quantizers;
            JxrPacketIndexTable index;
            JxrError error = JxrPacketIndexTable.Read(source, headers, out index);
            if (error != JxrError.None) return error;
            int imageWidth = (int)main.Width;
            int imageHeight = (int)main.Height;
            int columns = (imageWidth + 15) / 16;
            int rowsCount = (imageHeight + 15) / 16;
            int tileColumns = main.VerticalSliceCountMinusOne + 1;
            int tileRows = main.HorizontalSliceCountMinusOne + 1;
            int[] tileX, tileY;
            error = BuildTileBoundaries(main, columns, rowsCount,
                out tileX, out tileY);
            if (error != JxrError.None) return error;
            bool frequency = main.BitstreamFormat == 1;
            int packetBandCount = plane.Subband == 3 ? 1 :
                plane.Subband == 2 ? 2 : plane.Subband == 1 ? 3 : 4;
            if (index.PacketCount != tileColumns * tileRows *
                (frequency ? packetBandCount : 1))
                return JxrError.InvalidBitstream;

            JxrCodecColorFormat colorFormat =
                (JxrCodecColorFormat)plane.ColorFormat;
            bool subsampled = colorFormat == JxrCodecColorFormat.Yuv422 ||
                colorFormat == JxrCodecColorFormat.Yuv420;
            bool dcOnly = plane.Subband == (int)JxrGraySubbandMode.DcOnly;
            bool scaled = plane.ScaledArithmetic;
            bool hasHighpass = plane.Subband < (int)JxrGraySubbandMode.NoHighpass;
            bool skipFlexbits = plane.Subband == (int)JxrGraySubbandMode.NoFlexbits;
            JxrQuantizer[] dc = new JxrQuantizer[3];
            JxrQuantizer[][] lp = new JxrQuantizer[3][];
            JxrQuantizer[][] hp = new JxrQuantizer[3][];
            int[][] hpParameters = new int[3][];
            for (int channel = 0; channel < 3; channel++)
            {
                byte dcIndex = q.GetDcIndex(channel);
                byte lpIndex = plane.Subband == 3 ? dcIndex : q.GetLowpassIndex(channel);
                byte hpIndex = plane.Subband < 2 ? q.GetHighpassIndex(channel) : dcIndex;
                dc[channel] = JxrQuantization.Remap(dcIndex, scaled,
                    channel != 0).WithDcOffset();
                lp[channel] = new JxrQuantizer[] {
                    JxrQuantization.Remap(lpIndex, scaled, channel != 0) };
                hp[channel] = new JxrQuantizer[] {
                    JxrQuantization.Remap(hpIndex, scaled, false) };
                hpParameters[channel] = new int[] { hp[channel][0].Parameter };
            }
            JxrQuantizerSet quantizers = new JxrQuantizerSet(dc, lp, hp);
            int pixelCount = imageWidth * imageHeight;
            int chromaWidth = columns * 8;
            int chromaHeight = rowsCount *
                (colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
            int chromaCount = subsampled ? chromaWidth * chromaHeight : pixelCount;
            int[] yPlane = new int[pixelCount];
            int[] uPlane = new int[chromaCount];
            int[] vPlane = new int[chromaCount];
            int[][] outputPlanes = { yPlane, uPlane, vPlane };
            int[][][] globalCoefficients = main.Overlap == 0 ||
                main.HasHardTileBoundaries ? null : new int[3][][];
            if (globalCoefficients != null)
                for (int channel = 0; channel < 3; channel++)
                    globalCoefficients[channel] = new int[columns * rowsCount][];

            for (int tileRow = 0; tileRow < tileRows; tileRow++)
                for (int tileColumn = 0; tileColumn < tileColumns; tileColumn++)
                {
                    int startMbX = tileX[tileColumn];
                    int startMbY = tileY[tileRow];
                    int tileWidth = tileX[tileColumn + 1] - startMbX;
                    int tileHeight = tileY[tileRow + 1] - startMbY;
                    int tileId = (tileRow * tileColumns + tileColumn) & 31;
                    int trimFlexbits = 0;
                    JxrBitReader dcReader, lpReader, hpReader, flexReader;
                    if (frequency)
                    {
                        error = ReadFrequencyReader(index, source, headers,
                            tileRow, tileColumn, 0, tileId, out dcReader);
                        if (error != JxrError.None) return error;
                        lpReader = dcReader; hpReader = dcReader; flexReader = dcReader;
                        if (packetBandCount > 1)
                        {
                            error = ReadFrequencyReader(index, source, headers,
                                tileRow, tileColumn, 1, tileId, out lpReader);
                            if (error != JxrError.None) return error;
                        }
                        if (packetBandCount > 2)
                        {
                            error = ReadFrequencyReader(index, source, headers,
                                tileRow, tileColumn, 2, tileId, out hpReader);
                            if (error != JxrError.None) return error;
                        }
                        if (packetBandCount > 3)
                        {
                            error = ReadFrequencyReader(index, source, headers,
                                tileRow, tileColumn, 3, tileId, out flexReader);
                            if (error != JxrError.None) return error;
                            if (main.TrimFlexbits)
                            {
                                uint trim;
                                error = flexReader.ReadBits(4, out trim);
                                if (error != JxrError.None) return error;
                                trimFlexbits = (int)trim;
                            }
                        }
                    }
                    else
                    {
                        byte[] packetBytes;
                        error = index.ReadPacket(source, headers, tileRow,
                            tileColumn, out packetBytes);
                        if (error != JxrError.None) return error;
                        if (packetBytes.Length == Int32.MaxValue)
                            return JxrError.UnsupportedFeature;
                        byte[] paddedPacket = new byte[packetBytes.Length + 1];
                        Array.Copy(packetBytes, paddedPacket, packetBytes.Length);
                        dcReader = new JxrBitReader(paddedPacket);
                        JxrPacketHeader packet;
                        error = JxrPacketReader.ReadHeader(dcReader, out packet);
                        if (error != JxrError.None) return error;
                        if (!packet.IsValid || packet.TileId != tileId ||
                            packet.PacketType != 0) return JxrError.InvalidBitstream;
                        lpReader = hpReader = flexReader = dcReader;
                        if (main.TrimFlexbits)
                        {
                            uint trim;
                            error = dcReader.ReadBits(4, out trim);
                            if (error != JxrError.None) return error;
                            trimFlexbits = (int)trim;
                        }
                    }

                    JxrCodecConfiguration format = new JxrCodecConfiguration(
                        colorFormat, 3, !frequency, dcOnly, hasHighpass,
                        !skipFlexbits, skipFlexbits, true, true, false, false,
                        0, 0, 1, 1, hpParameters);
                    JxrCodecState state = new JxrCodecState(format,
                        dcReader, lpReader, hpReader, flexReader);
                    state.Entropy.TrimFlexBits = trimFlexbits;
                    JxrCoefficientPredictionRows coefficientRows =
                        new JxrCoefficientPredictionRows(tileWidth, 3);
                    int[][] topCbp = { new int[tileWidth], new int[tileWidth],
                        new int[tileWidth] };
                    int[][][] tileCoefficients = main.Overlap != 0 &&
                        main.HasHardTileBoundaries ? new int[3][][] : null;
                    if (tileCoefficients != null)
                        for (int channel = 0; channel < 3; channel++)
                            tileCoefficients[channel] = new int[tileWidth * tileHeight][];
                    int tilePixelWidth = Math.Min(imageWidth - startMbX * 16,
                        tileWidth * 16);
                    int tilePixelHeight = Math.Min(imageHeight - startMbY * 16,
                        tileHeight * 16);

                    for (int localY = 0; localY < tileHeight; localY++)
                    {
                        if (localY != 0) coefficientRows.AdvanceRow();
                        int[] leftCbp = new int[3];
                        for (int localX = 0; localX < tileWidth; localX++)
                        {
                            state.SetMacroblockPosition(localX, localY, tileWidth);
                            for (int channel = 0; channel < 3; channel++)
                            {
                                state.SetNeighborCbp(channel,
                                    topCbp[channel][localX], leftCbp[channel]);
                                int[] planeCoefficients;
                                error = state.CoefficientPlanes.GetPlane(channel,
                                    out planeCoefficients);
                                if (error != JxrError.None) return error;
                                Array.Clear(planeCoefficients, 0,
                                    planeCoefficients.Length);
                            }
                            error = JxrDcCodec.Decode(state);
                            if (error != JxrError.None) return error;
                            if (!dcOnly)
                            {
                                error = JxrLpCodec.Decode(state);
                                if (error != JxrError.None) return error;
                            }
                            error = JxrCoefficientPrediction.DecodeDcLp(
                                state.Macroblock, coefficientRows, colorFormat,
                                localX, localX == 0, localY == 0);
                            if (error != JxrError.None) return error;
                            error = JxrCoefficientPrediction.StoreCurrent(
                                state.Macroblock, coefficientRows, colorFormat,
                                localX);
                            if (error != JxrError.None) return error;
                            error = JxrQuantization.DequantizeMacroblock(
                                state.CoefficientPlanes, state.Macroblock,
                                quantizers, colorFormat, 3, dcOnly);
                            if (error != JxrError.None) return error;
                            if (hasHighpass)
                            {
                                error = JxrHpCodec.Decode(state);
                                if (error != JxrError.None) return error;
                                for (int channel = 0; channel < 3; channel++)
                                {
                                    error = state.MacroblockCbp.GetCbp(channel,
                                        out leftCbp[channel]);
                                    if (error != JxrError.None) return error;
                                    topCbp[channel][localX] = leftCbp[channel];
                                }
                            }
                            error = JxrCoefficientPrediction.DecodeAc(
                                state.Macroblock, state.CoefficientPlanes,
                                colorFormat);
                            if (error != JxrError.None) return error;
                            int globalX = startMbX + localX;
                            int globalY = startMbY + localY;
                            for (int channel = 0; channel < 3; channel++)
                            {
                                int[] planeCoefficients;
                                error = state.CoefficientPlanes.GetPlane(channel,
                                    out planeCoefficients);
                                if (error != JxrError.None) return error;
                                int[] samples = new int[planeCoefficients.Length];
                                Array.Copy(planeCoefficients, samples,
                                    samples.Length);
                                if (main.Overlap == 0)
                                    InverseAndCopyColorMacroblock(samples,
                                        outputPlanes[channel], imageWidth,
                                        imageHeight, chromaWidth, chromaHeight,
                                        globalX, globalY, channel, subsampled,
                                        colorFormat, scaled);
                                else if (main.HasHardTileBoundaries)
                                    tileCoefficients[channel][localY * tileWidth +
                                        localX] = samples;
                                else
                                    globalCoefficients[channel][globalY * columns +
                                        globalX] = samples;
                            }
                        }
                    }
                    if (tileCoefficients != null)
                        for (int channel = 0; channel < 3; channel++)
                        {
                            int[][] tileSamples = GetInverseOverlapSamples(
                                tileCoefficients[channel], tileWidth, tileHeight,
                                tilePixelWidth, tilePixelHeight, channel,
                                subsampled, colorFormat, main.Overlap, scaled,
                                hp[channel][0].Parameter, !hasHighpass);
                            for (int localY = 0; localY < tileHeight; localY++)
                                for (int localX = 0; localX < tileWidth; localX++)
                                    CopyColorMacroblock(
                                        tileSamples[localY * tileWidth + localX],
                                        outputPlanes[channel],
                                        subsampled && channel != 0 ? chromaWidth : imageWidth,
                                        subsampled && channel != 0 ? chromaHeight : imageHeight,
                                        startMbX + localX, startMbY + localY,
                                        subsampled && channel != 0 ? 8 : 16,
                                        subsampled && channel != 0 &&
                                            colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
                        }
                }

            if (globalCoefficients != null)
                for (int channel = 0; channel < 3; channel++)
                {
                    int[][] overlapSamples = GetInverseOverlapSamples(
                        globalCoefficients[channel], columns, rowsCount,
                        imageWidth, imageHeight, channel, subsampled,
                        colorFormat, main.Overlap, scaled,
                        hp[channel][0].Parameter, !hasHighpass);
                    for (int mbY = 0; mbY < rowsCount; mbY++)
                        for (int mbX = 0; mbX < columns; mbX++)
                            CopyColorMacroblock(
                                overlapSamples[mbY * columns + mbX],
                                outputPlanes[channel],
                                subsampled && channel != 0 ? chromaWidth : imageWidth,
                                subsampled && channel != 0 ? chromaHeight : imageHeight,
                                mbX, mbY,
                                subsampled && channel != 0 ? 8 : 16,
                                subsampled && channel != 0 &&
                                    colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
                }
            if (subsampled)
            {
                int paddedWidth = columns * 16;
                int paddedHeight = rowsCount * 16;
                bool vertical = colorFormat == JxrCodecColorFormat.Yuv420;
                uPlane = JxrChromaResampler.Interpolate(uPlane, imageWidth,
                    imageHeight, paddedWidth, paddedHeight, vertical);
                vPlane = JxrChromaResampler.Interpolate(vPlane, imageWidth,
                    imageHeight, paddedWidth, paddedHeight, vertical);
            }
            pixels = new byte[pixelCount * 3];
            error = JxrImagePipeline.DecodeRgb8(yPlane, uPlane, vPlane,
                imageWidth, imageHeight, rgbOrder, scaled ? 3 : 0,
                pixels, imageWidth * 3);
            if (error != JxrError.None) { pixels = null; return error; }
            width = imageWidth;
            height = imageHeight;
            return JxrError.None;
        }

        private static int[][] GetInverseOverlapSamples(int[][] coefficients,
            int columns, int rows, int pixelWidth, int pixelHeight, int channel,
            bool subsampled, JxrCodecColorFormat colorFormat, int overlap,
            bool scaled, int highpassQuantizer, bool highpassAbsent)
        {
            if (subsampled && channel != 0)
                return JxrChromaOverlapInverse.Transform(coefficients,
                    columns, rows, colorFormat, overlap, scaled);
            return JxrOverlapInverse.Transform(coefficients, pixelWidth,
                pixelHeight, overlap, channel != 0 && scaled,
                highpassQuantizer, highpassAbsent);
        }

        private static void InverseAndCopyColorMacroblock(int[] samples,
            int[] destination, int imageWidth, int imageHeight,
            int chromaWidth, int chromaHeight, int mbX, int mbY, int channel,
            bool subsampled, JxrCodecColorFormat colorFormat, bool scaled)
        {
            if (channel == 0 || !subsampled)
            {
                InverseMacroblock(samples, channel != 0 && scaled);
                CopyColorMacroblock(samples, destination, imageWidth,
                    imageHeight, mbX, mbY, 16, 16);
            }
            else
            {
                InverseChromaMacroblock(samples, colorFormat, scaled);
                CopyColorMacroblock(samples, destination, chromaWidth,
                    chromaHeight, mbX, mbY, 8,
                    colorFormat == JxrCodecColorFormat.Yuv420 ? 8 : 16);
            }
        }

        private static JxrError BuildTileBoundaries(JxrMainHeader main,
            int columns, int rows, out int[] tileX, out int[] tileY)
        {
            tileX = null;
            tileY = null;
            int tileColumns = main.VerticalSliceCountMinusOne + 1;
            int tileRows = main.HorizontalSliceCountMinusOne + 1;
            if (columns < 1 || rows < 1 || tileColumns < 1 || tileRows < 1 ||
                tileColumns > columns || tileRows > rows)
                return JxrError.InvalidBitstream;
            tileX = new int[tileColumns + 1];
            tileY = new int[tileRows + 1];
            for (int index = 0; index < tileColumns; index++)
            {
                int boundary = main.GetTileX(index);
                if (index == 0 && boundary != 0 ||
                    index > 0 && (boundary <= tileX[index - 1] ||
                        boundary >= columns))
                    return JxrError.InvalidBitstream;
                tileX[index] = boundary;
            }
            for (int index = 0; index < tileRows; index++)
            {
                int boundary = main.GetTileY(index);
                if (index == 0 && boundary != 0 ||
                    index > 0 && (boundary <= tileY[index - 1] ||
                        boundary >= rows))
                    return JxrError.InvalidBitstream;
                tileY[index] = boundary;
            }
            tileX[tileColumns] = columns;
            tileY[tileRows] = rows;
            return JxrError.None;
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

        private static void CopyColorMacroblock(int[] samples, int[] plane,
            int width, int height, int mbX, int mbY,
            int macroblockWidth, int macroblockHeight)
        {
            for (int y = 0; y < macroblockHeight; y++)
                for (int x = 0; x < macroblockWidth; x++)
                {
                    int pixelX = mbX * macroblockWidth + x;
                    int pixelY = mbY * macroblockHeight + y;
                    if (pixelX >= width || pixelY >= height) continue;
                    int block = (x >> 2) * macroblockHeight * 4 + (y >> 2) * 16;
                    int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                    plane[pixelY * width + pixelX] = samples[block + local];
                }
        }

        private static void InverseChromaMacroblock(int[] values,
            JxrCodecColorFormat colorFormat, bool scaledArithmetic)
        {
            if (colorFormat == JxrCodecColorFormat.Yuv420)
            {
                if (scaledArithmetic)
                    JxrInverseTransformMath.ApplyScaledDct2x2Down(
                        ref values[0], ref values[32], ref values[16],
                        ref values[48]);
                else
                    JxrTransformMath.ApplyDct2x2Down(values, 0, 32, 16, 48);
            }
            else
            {
                values[0] = unchecked(values[0] - ((values[32] + 1) >> 1));
                values[32] = unchecked(values[32] + values[0]);
                if (scaledArithmetic)
                {
                    JxrInverseTransformMath.ApplyScaledDct2x2Down(
                        ref values[0], ref values[64], ref values[16],
                        ref values[80]);
                    JxrInverseTransformMath.ApplyScaledDct2x2Down(
                        ref values[32], ref values[96], ref values[48],
                        ref values[112]);
                }
                else
                {
                    JxrTransformMath.ApplyDct2x2Down(values, 0, 64, 16, 80);
                    JxrTransformMath.ApplyDct2x2Down(values, 32, 96, 48, 112);
                }
            }
            for (int offset = 0; offset < values.Length; offset += 16)
            {
                int[] work = new int[16];
                Array.Copy(values, offset, work, 0, 16);
                JxrTransformMath.ApplyDct2x2Up(work, 0, 1, 2, 3);
                JxrInverseTransformMath.ApplyOdd(ref work[5], ref work[4],
                    ref work[7], ref work[6]);
                JxrInverseTransformMath.ApplyOdd(ref work[10], ref work[8],
                    ref work[11], ref work[9]);
                JxrInverseTransformMath.ApplyOddOdd(ref work[15], ref work[14],
                    ref work[13], ref work[12]);
                JxrTransformMath.ApplyFirstStageFourButterfly(work);
                Array.Copy(work, 0, values, offset, 16);
            }
        }

        private static void CopyGrayMacroblock(int[] samples, byte[] gray,
            int width, int height, int mbX, int mbY, bool scaledArithmetic)
        {
            for (int y = 0; y < 16; y++)
                for (int x = 0; x < 16; x++)
                {
                    int pixelX = mbX * 16 + x, pixelY = mbY * 16 + y;
                    if (pixelX >= width || pixelY >= height) continue;
                    int block = (x >> 2) * 64 + (y >> 2) * 16;
                    int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                    int sample = samples[block + local];
                    sample = scaledArithmetic ? unchecked(sample + 1027) >> 3 :
                        unchecked(sample + 128);
                    gray[pixelY * width + pixelX] = JxrImagePipeline.ClipByte(sample);
                }
        }

        private static void InverseMacroblock(int[] values)
        {
            InverseMacroblock(values, false);
        }

        private static void InverseMacroblock(int[] values, bool normalizeChroma)
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
            // Native JxrInverseTransformPlaneStage2Apply normalizes scaled
            // full-resolution chroma here, after stage 2 but before stage 1.
            // Doubling quantizers instead changes intermediate rounding.
            if (normalizeChroma)
                for (int offset = 0; offset < 256; offset += 16)
                    values[offset] = unchecked(values[offset] + values[offset]);

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

    }
}
