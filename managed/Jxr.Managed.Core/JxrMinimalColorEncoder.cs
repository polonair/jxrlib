using System;

namespace Jxr.Managed.Core
{
    // Full-resolution RGB/BGR input, reversible RGB-to-YUV transform and
    // per-tile spatial or frequency packets. Three coefficient planes share the adaptive
    // entropy context, prediction rows and macroblock position.
    internal static class JxrMinimalColorEncoder
    {
        private static readonly int[] LocalSampleOrder =
            { 0,1,5,4,2,3,7,6,10,11,15,14,8,9,13,12 };

        internal static JxrError Encode(JxrImage image, JxrEncoderOptions options,
            out byte[] jxr)
        {
            jxr = null;
            if (image == null || options == null ||
                (image.Format != JxrPixelFormat.Rgb24 &&
                 image.Format != JxrPixelFormat.Bgr24))
                return JxrError.InvalidArgument;
            if (image.Width > Int32.MaxValue - 15 ||
                image.Height > Int32.MaxValue - 15 ||
                (long)image.Width * image.Height > Int32.MaxValue / 3)
                return JxrError.UnsupportedFeature;
            JxrCodecColorFormat colorFormat =
                (JxrCodecColorFormat)options.ChromaSubsampling;
            bool subsampled = colorFormat != JxrCodecColorFormat.Yuv444;
            // The C encoder rejects a single-MB-wide subsampled image with
            // two overlap levels; keep the same public profile boundary.
            if (subsampled && options.Overlap == 2 && image.Width <= 16)
                return JxrError.UnsupportedFeature;
            byte dcIndex = QpIndex(options.DcQuantizerIndex, options.QualityIndex);
            byte lpIndex = QpIndex(options.LowpassQuantizerIndex, options.QualityIndex);
            byte hpIndex = QpIndex(options.HighpassQuantizerIndex, options.QualityIndex);
            bool scaled = options.Subbands != JxrGraySubbandMode.All ||
                dcIndex > 1 || lpIndex > 1 || hpIndex > 1;
            int width = image.Width, height = image.Height;
            int[] yPlane, uPlane, vPlane;
            JxrError error = JxrImagePipeline.EncodeRgb8(image.Pixels,
                image.Stride, width, height,
                image.Format == JxrPixelFormat.Rgb24, scaled ? 3 : 0,
                out yPlane, out uPlane, out vPlane);
            if (error != JxrError.None) return error;
            if (subsampled)
            {
                bool vertical = colorFormat == JxrCodecColorFormat.Yuv420;
                uPlane = JxrChromaResampler.Downsample(uPlane, width, height,
                    vertical);
                vPlane = JxrChromaResampler.Downsample(vPlane, width, height,
                    vertical);
            }
            int[][] sourcePlanes = { yPlane, uPlane, vPlane };
            int[][][] overlapped = null;
            if (options.Overlap != 0)
            {
                overlapped = new int[3][][];
                for (int channel = 0; channel < 3; channel++)
                    overlapped[channel] = subsampled && channel != 0 ?
                        JxrChromaOverlapForward.Transform(sourcePlanes[channel],
                            (width + 15) / 16, (height + 15) / 16,
                            colorFormat, options.Overlap, scaled) :
                        JxrOverlapForward.Transform(sourcePlanes[channel],
                            width, height, options.Overlap,
                            channel != 0 && scaled);
            }
            int columns = (width + 15) / 16, rowsCount = (height + 15) / 16;
            JxrTileGeometry tiles;
            error = JxrTileGeometry.Create(width, height, options.TileLayout,
                out tiles);
            if (error != JxrError.None) return error;
            JxrSessionConfiguration sessionConfig = new JxrSessionConfiguration(
                width, height, (int)colorFormat, 3, 4, false);
            using (JxrEncoderSession session = JxrEncoderSession.Create(
                sessionConfig, 0, 0))
            {
                JxrQuantizer[] dc = new JxrQuantizer[3];
                JxrQuantizer[][] lp = new JxrQuantizer[3][];
                JxrQuantizer[][] hp = new JxrQuantizer[3][];
                int[][] hpParameters = new int[3][];
                for (int channel = 0; channel < 3; channel++)
                {
                    dc[channel] = JxrQuantization.Remap(dcIndex, scaled,
                        channel != 0).WithDcOffset();
                    lp[channel] = new JxrQuantizer[] {
                        JxrQuantization.Remap(lpIndex, scaled, channel != 0) };
                    hp[channel] = new JxrQuantizer[] {
                        JxrQuantization.Remap(hpIndex, scaled, false) };
                    hpParameters[channel] = new int[] { hp[channel][0].Parameter };
                }
                JxrQuantizerSet quantizers = new JxrQuantizerSet(dc, lp, hp);
                JxrBitReader unused = new JxrBitReader(new byte[0]);
                byte[][] entropyPackets = new byte[tiles.Columns * tiles.Rows][];
                int[] entropyBitCounts = new int[entropyPackets.Length];
            bool frequency = options.Layout == JxrBitstreamLayout.Frequency;
            byte[][][] frequencyPackets = frequency ? new byte[4][][] : null;
            int[][] frequencyBitCounts = frequency ? new int[4][] : null;
            if (frequency)
                for (int band = 0; band < 4; band++)
                {
                    frequencyPackets[band] = new byte[entropyPackets.Length][];
                    frequencyBitCounts[band] = new int[entropyPackets.Length];
                }
                for (int tileY = 0; tileY < tiles.Rows; tileY++)
                    for (int tileX = 0; tileX < tiles.Columns; tileX++)
                    {
                        int startMbX = tiles.GetX(tileX);
                        int startMbY = tiles.GetY(tileY);
                        int tileWidth = tiles.GetX(tileX + 1) - startMbX;
                        int tileHeight = tiles.GetY(tileY + 1) - startMbY;
                        JxrCodecConfiguration format = new JxrCodecConfiguration(
                            colorFormat, 3, !frequency,
                            options.Subbands == JxrGraySubbandMode.DcOnly,
                            (int)options.Subbands < (int)JxrGraySubbandMode.NoHighpass,
                            options.Subbands != JxrGraySubbandMode.NoFlexbits,
                            options.Subbands == JxrGraySubbandMode.NoFlexbits,
                            true, true, false, false, 0, 0, 1, 1, hpParameters);
                        JxrCodecState state = new JxrCodecState(format,
                            unused, unused, unused, unused);
                        JxrCoefficientPredictionRows rows =
                            new JxrCoefficientPredictionRows(tileWidth, 3);
                        int[][] topCbp = { new int[tileWidth], new int[tileWidth],
                            new int[tileWidth] };
                        int[][] coefficients = new int[3][];
                        int[][] dcCoefficients = { new int[16],
                            new int[16], new int[16] };
                        for (int channel = 0; channel < 3; channel++)
                            state.CoefficientPlanes.GetPlane(channel,
                                out coefficients[channel]);
                        JxrBitWriter writer = new JxrBitWriter();
                        JxrBitWriter lpWriter = frequency ? new JxrBitWriter() : writer;
                        JxrBitWriter hpWriter = frequency ? new JxrBitWriter() : writer;
                        JxrBitWriter flexWriter = frequency ? new JxrBitWriter() : writer;
                        for (int localY = 0; localY < tileHeight; localY++)
                        {
                            if (localY != 0) rows.AdvanceRow();
                            int mbY = startMbY + localY;
                            int[] leftCbp = new int[3];
                            for (int localX = 0; localX < tileWidth; localX++)
                            {
                                int mbX = startMbX + localX;
                                state.SetMacroblockPosition(localX, localY, tileWidth);
                                for (int channel = 0; channel < 3; channel++)
                                {
                                    state.SetNeighborCbp(channel,
                                        topCbp[channel][localX], leftCbp[channel]);
                                    int[] values = coefficients[channel];
                                    int[] source = sourcePlanes[channel];
                                    if (overlapped != null)
                                        Array.Copy(overlapped[channel][mbY * columns + mbX],
                                            values, Math.Min(values.Length,
                                                overlapped[channel][mbY * columns + mbX].Length));
                                    else if (subsampled && channel != 0)
                                        JxrChromaForward.LoadAndTransform(source,
                                            columns * 8, mbX, mbY, colorFormat,
                                            scaled, values);
                                    else
                                    {
                                        for (int y = 0; y < 16; y++)
                                            for (int x = 0; x < 16; x++)
                                            {
                                                int pixelX = Math.Min(width - 1, mbX * 16 + x);
                                                int pixelY = Math.Min(height - 1, mbY * 16 + y);
                                                int block = (x >> 2) * 64 + (y >> 2) * 16;
                                                int local = LocalSampleOrder[(y & 3) * 4 + (x & 3)];
                                                values[block + local] =
                                                    source[pixelY * width + pixelX];
                                            }
                                        JxrMinimalEncoder.ForwardMacroblock(values,
                                            channel != 0 && scaled);
                                    }
                                }
                                error = JxrQuantization.QuantizeMacroblock(
                                    state.CoefficientPlanes, state.Macroblock,
                                    quantizers, colorFormat, 3,
                                    options.Subbands == JxrGraySubbandMode.DcOnly,
                                    (int)options.Subbands >=
                                        (int)JxrGraySubbandMode.NoHighpass, false);
                                if (error != JxrError.None) return error;
                                error = JxrCoefficientPrediction.Encode(
                                    state.Macroblock, state.CoefficientPlanes,
                                    rows, colorFormat, localX,
                                    localX == 0, localY == 0);
                                if (error != JxrError.None) return error;
                                for (int channel = 0; channel < 3; channel++)
                                    for (int index = 0; index < 16; index++)
                                    {
                                        error = state.Macroblock.GetDcCoefficient(
                                            channel, index,
                                            out dcCoefficients[channel][index]);
                                        if (error != JxrError.None) return error;
                                    }
                                error = subsampled ?
                                    JxrMinimalEntropyEncoder.EncodeSubsampledMacroblock(
                                        state, coefficients, dcCoefficients,
                                        state.Macroblock.Orientation,
                                        (int)options.Subbands,
                                        options.TrimFlexbits, writer, lpWriter,
                                        hpWriter, flexWriter) :
                                    JxrMinimalEntropyEncoder.EncodeYuv444Macroblock(
                                        state, coefficients, dcCoefficients,
                                        state.Macroblock.Orientation,
                                        (int)options.Subbands,
                                        options.TrimFlexbits, writer, lpWriter,
                                        hpWriter, flexWriter);
                                if (error != JxrError.None) return error;
                                for (int channel = 0; channel < 3; channel++)
                                {
                                    int previousTop;
                                    state.GetNeighborCbp(channel, out previousTop,
                                        out leftCbp[channel]);
                                    topCbp[channel][localX] = leftCbp[channel];
                                }
                            }
                        }
                        int packetIndex = tileY * tiles.Columns + tileX;
                        if (frequency)
                        {
                            JxrBitWriter[] writers = { writer, lpWriter,
                                hpWriter, flexWriter };
                            for (int band = 0; band < 4; band++)
                            {
                                frequencyBitCounts[band][packetIndex] =
                                    writers[band].BitCount;
                                writers[band].AlignByte();
                                frequencyPackets[band][packetIndex] =
                                    writers[band].ToArray();
                            }
                        }
                        else
                        {
                            entropyBitCounts[packetIndex] = writer.BitCount;
                            writer.AlignByte();
                            entropyPackets[packetIndex] = writer.ToArray();
                        }
                    }
                byte[] codestream;
                if (frequency)
                    error = JxrFrequencyCodestreamWriter.WriteRgb(
                        frequencyPackets, frequencyBitCounts, width, height,
                        dcIndex, lpIndex, hpIndex, options.Subbands, scaled,
                        options.TrimFlexbits, options.Overlap,
                        options.ChromaSubsampling, options.TileLayout == null ?
                            new JxrTileLayout(new int[] { columns },
                                new int[] { rowsCount }) : options.TileLayout,
                        options.Progressive, out codestream);
                else error = JxrCodestreamWriter.WriteRgbSpatialTiles(entropyPackets,
                    entropyBitCounts, width, height, dcIndex, lpIndex, hpIndex,
                    options.Subbands, scaled, options.TrimFlexbits,
                    options.Overlap, options.ChromaSubsampling,
                    options.TileLayout == null ?
                        new JxrTileLayout(new int[] { columns },
                            new int[] { rowsCount }) : options.TileLayout,
                    out codestream);
                if (error != JxrError.None) return error;
                return JxrContainerWriter.WriteRgb24(codestream,
                    width, height, 96.012f, 96.012f, out jxr);
            }
        }

        private static byte QpIndex(int overrideIndex, int qualityIndex)
        {
            int index = overrideIndex < 0 ? qualityIndex : overrideIndex;
            return (byte)(index < 2 ? 0 : index);
        }
    }
}
