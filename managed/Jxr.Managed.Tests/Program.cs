using System;
using System.IO;
using Jxr.Managed.Core;

namespace Jxr.Managed.Tests
{
    internal delegate bool TestMethod();

    internal sealed class TestCase
    {
        internal string Name;
        internal TestMethod Method;
        internal TestCase(string name, TestMethod method) { Name = name; Method = method; }
    }

    internal static class Program
    {
        private static readonly TestCase[] Tests = {
            new TestCase("bit_math_vectors", TestBitMathVectors),
            new TestCase("bit_reader_vectors", TestBitReaderVectors),
            new TestCase("bit_writer_vectors", TestBitWriterVectors),
            new TestCase("packet_header_syntax_reader_vectors", TestPacketHeaderSyntaxReaderVectors),
            new TestCase("adaptive_scan_vectors", TestAdaptiveScanVectors),
            new TestCase("adaptive_scan_state_vectors", TestAdaptiveScanStateVectors),
            new TestCase("adaptive_scan_default_vectors", TestAdaptiveScanDefaultVectors),
            new TestCase("explicit_entropy_context", TestExplicitEntropyContext),
            new TestCase("minimal_entropy_codec_fixture", TestMinimalEntropyCodecFixture),
            new TestCase("color_entropy_codec_fixture", TestColorEntropyCodecFixture),
            new TestCase("real_rgb444_decode", TestRealRgb444Decode),
            new TestCase("rgb444_quality_native_fixtures", TestRgb444QualityNativeFixtures),
            new TestCase("quantization_reference_vectors", TestQuantizationReferenceVectors),
            new TestCase("quantization_macroblock_vectors", TestQuantizationMacroblockVectors),
            new TestCase("quantization_channel_modes", TestQuantizationChannelModes),
            new TestCase("coefficient_prediction_vectors", TestCoefficientPredictionVectors),
            new TestCase("transform_math_reference_vectors", TestTransformMathReferenceVectors),
            new TestCase("forward_transform_math_reference_vectors", TestForwardTransformMathReferenceVectors),
            new TestCase("inverse_transform_math_reference_vectors", TestInverseTransformMathReferenceVectors),
            new TestCase("headers_reference_fixtures", TestHeadersReferenceFixtures),
            new TestCase("source_profile_reference_fixtures", TestSourceProfileReferenceFixtures),
            new TestCase("headers_syntax_vectors", TestHeadersSyntaxVectors),
            new TestCase("header_writer_fixture", TestHeaderWriterFixture),
            new TestCase("header_writer_fields", TestHeaderWriterFields),
            new TestCase("rgb444_header_writer_fixture", TestRgb444HeaderWriterFixture),
            new TestCase("rgb444_encoder_native_fixture", TestRgb444EncoderNativeFixture),
            new TestCase("rgb444_encoder_quality_native_fixtures", TestRgb444EncoderQualityNativeFixtures),
            new TestCase("bgr444_encoder_round_trip", TestBgr444EncoderRoundTrip),
            new TestCase("rgb444_sizes_round_trip", TestRgb444SizesRoundTrip),
            new TestCase("overlap_forward_native_fixtures", TestOverlapForwardNativeFixtures),
            new TestCase("subsampled_decode_native_fixtures", TestSubsampledDecodeNativeFixtures),
            new TestCase("subsampled_encode_native_fixtures", TestSubsampledEncodeNativeFixtures),
            new TestCase("subsampled_edge_native_fixtures", TestSubsampledEdgeNativeFixtures),
            new TestCase("real_subsampled_native_fixtures", TestRealSubsampledNativeFixtures),
            new TestCase("spatial_tile_native_fixtures", TestSpatialTileNativeFixtures),
            new TestCase("spatial_tile_encode_native_fixtures", TestSpatialTileEncodeNativeFixtures),
            new TestCase("frequency_layout_native_fixtures", TestFrequencyLayoutNativeFixtures),
            new TestCase("spatial_tile_layout_validation", TestSpatialTileLayoutValidation),
            new TestCase("spatial_tile_invalid_tables", TestSpatialTileInvalidTables),
            new TestCase("minimal_decoder_end_to_end", TestMinimalDecoderEndToEnd),
            new TestCase("minimal_entropy_encoder_fixture", TestMinimalEntropyEncoderFixture),
            new TestCase("minimal_encoder_end_to_end", TestMinimalEncoderEndToEnd),
            new TestCase("gray_sizes_native_fixtures", TestGraySizesNativeFixtures),
            new TestCase("gray_quality_native_fixtures", TestGrayQualityNativeFixtures),
            new TestCase("gray_mixed_qp_options", TestGrayMixedQpOptions),
            new TestCase("public_pixel_api", TestPublicPixelApi),
            new TestCase("planar_alpha_native_fixtures", TestPlanarAlphaNativeFixtures),
            new TestCase("pbgra_alpha_conversion_vectors", TestPbgraAlphaConversionVectors),
            new TestCase("public_stream_api", TestPublicStreamApi),
            new TestCase("image_pipeline_reference_vectors", TestImagePipelineReferenceVectors),
            new TestCase("image_pipeline_bitmap_fixtures", TestImagePipelineBitmapFixtures),
            new TestCase("session_memory_reference_vectors", TestSessionMemoryReferenceVectors),
            new TestCase("session_lifecycle_vectors", TestSessionLifecycleVectors),
            new TestCase("transcode_coefficient_reference_vectors",
                TestTranscodeCoefficientReferenceVectors),
            new TestCase("transcode_roi_reference_vectors",
                TestTranscodeRoiReferenceVectors),
            new TestCase("coefficient_buffer_vectors", TestCoefficientBufferVectors),
            new TestCase("coefficient_plane_state_vectors", TestCoefficientPlaneStateVectors),
            new TestCase("macroblock_state_vectors", TestMacroblockStateVectors),
            new TestCase("macroblock_cbp_state_vectors", TestMacroblockCbpStateVectors),
            new TestCase("lowpass_cbp_state_vectors", TestLowpassCbpStateVectors),
            new TestCase("highpass_cbp_state_vectors", TestHighpassCbpStateVectors),
            new TestCase("huffman_state_set_vectors", TestHuffmanStateSetVectors),
            new TestCase("adaptive_huffman_vectors", TestAdaptiveHuffmanVectors),
            new TestCase("adaptive_huffman_table_catalog_vectors", TestAdaptiveHuffmanTableCatalogVectors),
            new TestCase("adaptive_huffman_catalog_signature_vectors", TestAdaptiveHuffmanCatalogSignatureVectors),
            new TestCase("huffman_decoder_vectors", TestHuffmanDecoderVectors),
            new TestCase("huffman_short_tail_vectors", TestHuffmanShortTailVectors),
            new TestCase("adaptive_huffman_decode_vectors", TestAdaptiveDecodeVectors),
            new TestCase("huffman_error_vectors", TestErrorVectors)
        };

        private static int Main(string[] args)
        {
            int index;
            bool found = false;
            bool passed = true;
            for (index = 0; index < Tests.Length; index++)
            {
                if (args.Length != 0 && args[0] != Tests[index].Name) continue;
                found = true;
                if (Tests[index].Method()) Console.WriteLine("PASS " + Tests[index].Name);
                else { Console.WriteLine("FAIL " + Tests[index].Name); passed = false; }
            }
            if (!found) { Console.WriteLine("Unknown test"); return 2; }
            return passed ? 0 : 1;
        }

        // Native decoder trace: DC [1320,1330), LP [1330,1544),
        // HP [1544,3502); the same JPEG XR bytes are the oracle.
        private static bool TestMinimalEntropyCodecFixture()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            byte[] bytes = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            JxrBitReader reader = new JxrBitReader(bytes);
            int remaining = 1320;
            while (remaining > 0)
            {
                int count = Math.Min(remaining, 32);
                if (reader.ConsumeBits(count) != JxrError.None) return false;
                remaining -= count;
            }
            JxrCodecConfiguration format = new JxrCodecConfiguration(
                JxrCodecColorFormat.YOnly, 1, true, false, true, true,
                false, true, true, false, false, 0, 0, 1, 1,
                new int[][] { new int[] { 1 } });
            JxrCodecState state = new JxrCodecState(format, reader, reader, reader, reader);
            if (JxrDcCodec.Decode(state) != JxrError.None || reader.BitPosition != 1330)
            { Console.WriteLine("DC bit position: " + reader.BitPosition); return false; }
            if (JxrLpCodec.Decode(state) != JxrError.None || reader.BitPosition != 1544)
            { Console.WriteLine("LP bit position: " + reader.BitPosition); return false; }
            int[] dequantized = ReadTraceValues(Path.Combine(directory.FullName,
                "minimal-profile\\trace\\decoder-mb-000-000-after_dequantization.json"));
            int[] dcOffsets = { 0,128,64,208,32,240,48,224,
                16,192,80,144,112,176,96,160 };
            if (dequantized == null || dequantized.Length != 256) return false;
            for (int coefficient = 0; coefficient < 16; coefficient++)
            {
                int actual;
                if (state.Macroblock.GetDcCoefficient(0, coefficient, out actual) != JxrError.None ||
                    actual != dequantized[dcOffsets[coefficient]])
                { Console.WriteLine("DC/LP coefficient " + coefficient + ": " + actual);
                  return false; }
            }
            JxrCoefficientPredictionRows predictionRows =
                new JxrCoefficientPredictionRows(1, 1);
            if (JxrCoefficientPrediction.DecodeDcLp(state.Macroblock,
                predictionRows, JxrCodecColorFormat.YOnly, 0, true, true) != JxrError.None ||
                state.Macroblock.Orientation != 1) return false;
            if (JxrCoefficientPrediction.StoreCurrent(state.Macroblock,
                predictionRows, JxrCodecColorFormat.YOnly, 0) != JxrError.None) return false;
            int[] values;
            if (state.CoefficientPlanes.GetPlane(0, out values) != JxrError.None) return false;
            JxrQuantizer lossless = JxrQuantization.Remap(0, false, false);
            JxrQuantizerSet quantizers = new JxrQuantizerSet(
                new JxrQuantizer[] { lossless.WithDcOffset() },
                new JxrQuantizer[][] { new JxrQuantizer[] { lossless } },
                new JxrQuantizer[][] { new JxrQuantizer[] { lossless } });
            if (JxrQuantization.DequantizeMacroblock(state.CoefficientPlanes,
                state.Macroblock, quantizers, JxrCodecColorFormat.YOnly,
                1, false) != JxrError.None) return false;
            for (int coefficient = 0; coefficient < 256; coefficient++)
                if (values[coefficient] != dequantized[coefficient])
                { Console.WriteLine("Dequant coefficient " + coefficient); return false; }
            if (JxrHpCodec.Decode(state) != JxrError.None || reader.BitPosition != 3502)
            { Console.WriteLine("HP bit position: " + reader.BitPosition); return false; }
            int cbp, differential;
            if (state.MacroblockCbp.GetCbp(0, out cbp) != JxrError.None ||
                state.MacroblockCbp.GetDifferential(0, out differential) != JxrError.None)
                return false;
            int[] expected = ReadTraceValues(Path.Combine(directory.FullName,
                "minimal-profile\\trace\\decoder-mb-000-000-after_hp.json"));
            if (expected == null || expected.Length != 256) return false;
            for (int coefficient = 0; coefficient < 256; coefficient++)
                if (values[coefficient] != expected[coefficient])
                { Console.WriteLine("HP coefficient " + coefficient + ": " +
                    values[coefficient] + " != " + expected[coefficient]); return false; }
            if (JxrCoefficientPrediction.DecodeAc(state.Macroblock,
                state.CoefficientPlanes, JxrCodecColorFormat.YOnly) != JxrError.None)
                return false;
            expected = ReadTraceValues(Path.Combine(directory.FullName,
                "minimal-profile\\trace\\decoder-mb-000-000-after_ac_prediction.json"));
            if (expected == null || expected.Length != 256) return false;
            for (int coefficient = 0; coefficient < 256; coefficient++)
                if (values[coefficient] != expected[coefficient])
                { Console.WriteLine("AC coefficient " + coefficient + ": " +
                    values[coefficient] + " != " + expected[coefficient]); return false; }
            return cbp == 65535 && differential == 0;
        }

        private static int[] ReadTraceValues(string path)
        {
            string trace = File.ReadAllText(path);
            int start = trace.IndexOf("\"values\": [", StringComparison.Ordinal);
            if (start < 0) return null;
            start = trace.IndexOf('[', start) + 1;
            int end = trace.IndexOf(']', start);
            if (end < start) return null;
            string[] fields = trace.Substring(start, end - start).Split(',');
            int[] values = new int[fields.Length];
            for (int index = 0; index < fields.Length; index++)
                values[index] = Int32.Parse(fields[index].Trim(),
                    System.Globalization.CultureInfo.InvariantCulture);
            return values;
        }

        private static bool TestSourceProfileReferenceFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr")))
                directory = directory.Parent;
            if (directory == null) return false;
            string[] files = {
                "minimal-profile\\minimal-gray-16x16.jxr",
                "managed\\fixtures\\rgb444-tiles2x2-q16-ol2.jxr",
                "managed\\fixtures\\frequency-gray32-tile2x2.jxr",
                "managed\\fixtures\\alpha-planar-q1-32x32.jxr"
            };
            JxrSourceProfile[] profiles = new JxrSourceProfile[files.Length];
            for (int index = 0; index < files.Length; index++)
            {
                byte[] bytes = File.ReadAllBytes(Path.Combine(directory.FullName,
                    files[index]));
                JxrError error = JxrSourceProfileReader.Read(bytes,
                    out profiles[index]);
                if (error != JxrError.None || profiles[index] == null ||
                    !profiles[index].PacketSyntaxComplete)
                {
                    Console.WriteLine("Source profile failed for " + files[index] +
                        ": " + error);
                    return false;
                }
            }
            if (profiles[0].Headers.ContainerKind != JxrContainerKind.TiffLike ||
                profiles[0].Headers.PixelFormatGuid == null ||
                profiles[0].Headers.ContainerWidth != 16 ||
                profiles[0].ColorPlane.TileCount != 1 ||
                profiles[1].ColorPlane.TileCount != 4 ||
                profiles[2].ColorPlane.TileCount != 4 ||
                profiles[2].ColorPlane.GetTile(0).PacketCount < 3 ||
                profiles[2].ColorPlane.GetTile(0).PacketCount > 4 ||
                !profiles[3].HasPlanarAlpha || profiles[3].AlphaPlane == null ||
                profiles[3].Headers.AlphaRangeInterpretation !=
                    JxrAlphaRangeInterpretation.AbsoluteEndOffset ||
                profiles[3].Headers.AlphaByteCount != profiles[3].Headers.AlphaRangeTagValue -
                    profiles[3].Headers.AlphaOffset)
            {
                Console.WriteLine("Profile fixture values: container=" +
                    profiles[0].Headers.ContainerKind + " guid=" +
                    profiles[0].Headers.PixelFormatGuid + " dims=" +
                    profiles[0].Headers.ContainerWidth + "x" +
                    profiles[0].Headers.ContainerHeight + " spatialTiles=" +
                    profiles[1].ColorPlane.TileCount + " frequencyTiles=" +
                    profiles[2].ColorPlane.TileCount + " frequencyPackets=" +
                    profiles[2].ColorPlane.GetTile(0).PacketCount + " alpha=" +
                    profiles[3].Headers.AlphaRangeInterpretation + ":" +
                    profiles[3].Headers.AlphaByteCount + "/" +
                    profiles[3].Headers.AlphaRangeTagValue + "/" +
                    profiles[3].Headers.AlphaOffset + " subband=" +
                    profiles[2].ColorPlane.Headers.Plane.Subband + " checks=" +
                    (profiles[0].Headers.ContainerKind == JxrContainerKind.TiffLike) + "," +
                    (profiles[0].Headers.PixelFormatGuid != null) + "," +
                    (profiles[0].Headers.ContainerWidth == 16) + "," +
                    (profiles[0].ColorPlane.TileCount == 1) + "," +
                    (profiles[1].ColorPlane.TileCount == 4) + "," +
                    (profiles[2].ColorPlane.TileCount == 4) + "," +
                    (profiles[2].ColorPlane.GetTile(0).PacketCount ==
                        (profiles[2].ColorPlane.Headers.Plane.Subband == 3 ? 1 :
                         profiles[2].ColorPlane.Headers.Plane.Subband == 2 ? 2 :
                         profiles[2].ColorPlane.Headers.Plane.Subband == 1 ? 3 : 4)) + "," +
                    profiles[3].HasPlanarAlpha + "," +
                    (profiles[3].AlphaPlane != null) + "," +
                    (profiles[3].Headers.AlphaRangeInterpretation ==
                        JxrAlphaRangeInterpretation.AbsoluteEndOffset) + "," +
                    (profiles[3].Headers.AlphaByteCount == profiles[3].Headers.AlphaRangeTagValue -
                        profiles[3].Headers.AlphaOffset));
                return false;
            }
            byte[] alphaSource = File.ReadAllBytes(Path.Combine(directory.FullName,
                files[3]));
            uint ifdOffset = BitConverter.ToUInt32(alphaSource, 4);
            int ifd = (int)ifdOffset;
            int entryCount = BitConverter.ToUInt16(alphaSource, ifd);
            int rangeValuePosition = -1;
            for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
            {
                int entry = ifd + 2 + entryIndex * 12;
                if (BitConverter.ToUInt16(alphaSource, entry) == 0xbcc3)
                { rangeValuePosition = entry + 8; break; }
            }
            if (rangeValuePosition < 0) return false;
            byte[] byteCountAlpha = (byte[])alphaSource.Clone();
            Array.Copy(BitConverter.GetBytes((uint)profiles[3].Headers.AlphaByteCount),
                0, byteCountAlpha, rangeValuePosition, 4);
            JxrSourceProfile byteCountProfile;
            if (JxrSourceProfileReader.Read(byteCountAlpha, out byteCountProfile) !=
                    JxrError.None || byteCountProfile == null ||
                byteCountProfile.Headers.AlphaRangeInterpretation !=
                    JxrAlphaRangeInterpretation.ByteCount ||
                byteCountProfile.Headers.AlphaByteCount != profiles[3].Headers.AlphaByteCount)
                return false;
            byte[] ambiguousAlpha = new byte[alphaSource.Length + 4000];
            Array.Copy(alphaSource, ambiguousAlpha, alphaSource.Length);
            JxrHeaders ambiguousHeaders;
            if (JxrHeaders.Read(ambiguousAlpha, out ambiguousHeaders) !=
                JxrError.InvalidBitstream) return false;
            JxrSourceProfile streamProfile;
            using (MemoryStream stream = new MemoryStream(byteCountAlpha))
                if (JxrSourceProfileReader.Read(stream, out streamProfile) != JxrError.None ||
                    streamProfile == null || !streamProfile.PacketSyntaxComplete)
                    return false;
            return true;
        }

        private static bool TestHeadersReferenceFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string[] paths = {
                "minimal-profile\\minimal-gray-16x16.jxr",
                "real-image-profile\\test-sign-334x330.jxr",
                "default-profile\\city-park-605x478.jxr"
            };
            ulong hash = 14695981039346656037UL;
            for (int fixture = 0; fixture < paths.Length; fixture++)
            {
                byte[] bytes = File.ReadAllBytes(Path.Combine(directory.FullName, paths[fixture]));
                JxrHeaders headers;
                JxrError error = JxrHeaders.Read(bytes, out headers);
                if (error != JxrError.None || headers == null)
                { Console.WriteLine("Header read " + paths[fixture] + ": " + error); return false; }
                JxrMainHeader main = headers.Main;
                JxrImagePlaneHeader plane = headers.Plane;
                JxrImagePlaneQuantizerHeader q = headers.Quantizers;
                if (headers.CodestreamOffset <= 0 || headers.ByteCount <= 8)
                    return false;
                byte[] rawCodestream = new byte[bytes.Length - headers.CodestreamOffset];
                Array.Copy(bytes, headers.CodestreamOffset, rawCodestream, 0,
                    rawCodestream.Length);
                JxrHeaders rawHeaders;
                if (JxrHeaders.Read(rawCodestream, out rawHeaders) != JxrError.None ||
                    rawHeaders.CodestreamOffset != 0 ||
                    rawHeaders.ByteCount != headers.ByteCount ||
                    rawHeaders.Main.Width != main.Width ||
                    rawHeaders.Main.Height != main.Height ||
                    rawHeaders.Quantizers.Mode != q.Mode) return false;
                hash = QuantizationHashValue(hash, unchecked((int)main.Width));
                hash = QuantizationHashValue(hash, unchecked((int)main.Height));
                hash = QuantizationHashValue(hash, main.SourceColorFormat);
                hash = QuantizationHashValue(hash, main.SourceBitDepth);
                hash = QuantizationHashValue(hash, main.Orientation);
                hash = QuantizationHashValue(hash, main.BitstreamFormat);
                hash = QuantizationHashValue(hash, main.Overlap);
                hash = QuantizationHashValue(hash, plane.Subband);
                hash = QuantizationHashValue(hash, plane.ColorFormat);
                hash = QuantizationHashValue(hash, main.VerticalSliceCountMinusOne);
                hash = QuantizationHashValue(hash, main.HorizontalSliceCountMinusOne);
                hash = QuantizationHashValue(hash, plane.MantissaOrShift);
                hash = QuantizationHashValue(hash, plane.ExponentBias);
                hash = QuantizationHashValue(hash, main.BlackWhite ? 1 : 0);
                hash = QuantizationHashValue(hash, main.Version);
                hash = QuantizationHashValue(hash, main.Subversion);
                hash = QuantizationHashValue(hash, main.HasHardTileBoundaries ? 1 : 0);
                hash = QuantizationHashValue(hash, main.HasIndexTable ? 1 : 0);
                hash = QuantizationHashValue(hash, main.TrimFlexbits ? 1 : 0);
                hash = QuantizationHashValue(hash, main.RedBlueSwapped ? 1 : 0);
                hash = QuantizationHashValue(hash, main.HasAlpha ? 1 : 0);
                hash = QuantizationHashValue(hash, main.ExtraTop);
                hash = QuantizationHashValue(hash, main.ExtraLeft);
                hash = QuantizationHashValue(hash, main.ExtraBottom);
                hash = QuantizationHashValue(hash, main.ExtraRight);
                hash = QuantizationHashValue(hash, plane.ChannelCount);
                hash = QuantizationHashValue(hash, plane.ScaledArithmetic ? 1 : 0);
                hash = QuantizationHashValue(hash, q.Mode);
                hash = QuantizationHashValue(hash, headers.ByteCount - 8);
                for (int channel = 0; channel < plane.ChannelCount; channel++)
                {
                    hash = QuantizationHashValue(hash, q.GetDcIndex(channel));
                    hash = QuantizationHashValue(hash, q.GetLowpassIndex(channel));
                    hash = QuantizationHashValue(hash, q.GetHighpassIndex(channel));
                }
                if (fixture == 0)
                {
                    int codestreamOffset = headers.CodestreamOffset;
                    byte[] invalid = (byte[])bytes.Clone();
                    invalid[0] = 0;
                    if (JxrHeaders.Read(invalid, out headers) != JxrError.InvalidBitstream ||
                        headers != null || JxrHeaders.Read(null, out headers) !=
                        JxrError.InvalidArgument || headers != null)
                        return false;
                    invalid = (byte[])bytes.Clone();
                    invalid[codestreamOffset + 8] =
                        (byte)(invalid[codestreamOffset + 8] & 15);
                    if (JxrHeaders.Read(invalid, out headers) != JxrError.InvalidBitstream)
                        return false;
                    if (JxrHeaders.Read(new byte[] { (byte)'W', (byte)'M',
                        (byte)'P', (byte)'H', (byte)'O', (byte)'T', (byte)'O' }, out headers) !=
                        JxrError.UnexpectedEndOfStream) return false;
                }
            }
            Console.WriteLine("Headers signature: " + hash.ToString("X16"));
            return hash == 0xE5CFDDD005A012B9UL;
        }

        private static bool TestMinimalDecoderEndToEnd()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            byte[] jxr = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            byte[] expected = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16-restored.bmp"));
            byte[] original = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"));
            byte[] actual;
            JxrError error = JxrMinimalDecoder.DecodeGrayBmp(jxr, out actual);
            if (error != JxrError.None || actual == null)
            {
                Console.WriteLine("Minimal decode error: " + error);
                return false;
            }
            if (actual.Length != expected.Length || original.Length != expected.Length)
            {
                Console.WriteLine("BMP length: " + actual.Length + " vs " + expected.Length);
                return false;
            }
            for (int index = 0; index < expected.Length; index++)
                if (actual[index] != expected[index] ||
                    actual[index] != original[index])
                {
                    Console.WriteLine("BMP first difference at " + index + ": " +
                        actual[index] + " vs " + expected[index]);
                    return false;
                }
            JxrHeaders headers;
            if (JxrHeaders.Read(jxr, out headers) != JxrError.None) return false;
            byte[] raw = new byte[jxr.Length - headers.CodestreamOffset];
            Array.Copy(jxr, headers.CodestreamOffset, raw, 0, raw.Length);
            byte[] rawBitmap;
            if (JxrMinimalDecoder.DecodeGrayBmp(raw, out rawBitmap) != JxrError.None ||
                rawBitmap.Length != expected.Length) return false;
            for (int index = 0; index < expected.Length; index++)
                if (rawBitmap[index] != expected[index]) return false;
            int wordOffset = headers.CodestreamOffset + headers.ByteCount;
            int skip = (jxr[wordOffset] << 8) | jxr[wordOffset + 1];
            int packetOffset = wordOffset + 2 + skip;
            byte[] invalid = (byte[])jxr.Clone();
            invalid[packetOffset + 2] = 0;
            if (JxrMinimalDecoder.DecodeGrayBmp(invalid, out actual) !=
                JxrError.InvalidBitstream || actual != null) return false;
            byte[] rotated = (byte[])jxr.Clone();
            rotated[headers.CodestreamOffset + 9] |= 8;
            if (JxrMinimalDecoder.DecodeGrayBmp(rotated, out actual) !=
                JxrError.UnsupportedFeature || actual != null) return false;
            byte[] color = File.ReadAllBytes(Path.Combine(directory.FullName,
                "real-image-profile\\test-sign-334x330.jxr"));
            if (JxrMinimalDecoder.DecodeGrayBmp(color, out actual) !=
                JxrError.UnsupportedFeature || actual != null) return false;
            return true;
        }

        private static bool TestMinimalEntropyEncoderFixture()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            int[] coefficients = ReadTraceValues(Path.Combine(directory.FullName,
                "minimal-profile\\trace\\encoder-mb-000-000-predicted_coefficients.json"));
            if (coefficients == null || coefficients.Length != 256) return false;
            int[] dcOffsets = { 0,128,64,208,32,240,48,224,16,192,80,144,112,176,96,160 };
            int[] dc = new int[16];
            for (int index = 0; index < 16; index++) dc[index] = coefficients[dcOffsets[index]];
            byte[] jxr = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            JxrBitWriter writer = new JxrBitWriter();
            int dcEnd, lpEnd, hpEnd;
            JxrError error = JxrMinimalEntropyEncoder.Encode(coefficients, dc,
                1, writer, out dcEnd, out lpEnd, out hpEnd);
            if (error != JxrError.None || dcEnd != 10 || lpEnd != 224 ||
                hpEnd != 2182) return false;
            byte[] data = writer.ToArray();
            for (int bit = 0; bit < hpEnd; bit++)
            {
                int actual = (data[bit >> 3] >> (7 - (bit & 7))) & 1;
                int expected = (jxr[165 + (bit >> 3)] >> (7 - (bit & 7))) & 1;
                if (actual != expected)
                { Console.WriteLine("Entropy first bit difference " + bit); return false; }
            }
            return true;
        }

        private static bool TestBitWriterVectors()
        {
            JxrBitWriter writer = new JxrBitWriter();
            if (writer.Write(123, 0) != JxrError.None || writer.BitCount != 0 ||
                writer.Write(10, 4) != JxrError.None ||
                writer.Write(11, 4) != JxrError.None ||
                writer.Write(0x1234, 16) != JxrError.None ||
                writer.Write(0xdeadbeefU, 32) != JxrError.None ||
                writer.Write(1, 33) != JxrError.InvalidArgument ||
                writer.BitCount != 56) return false;
            byte[] data = writer.ToArray();
            byte[] expected = { 0xab, 0x12, 0x34, 0xde, 0xad, 0xbe, 0xef };
            if (data.Length != expected.Length) return false;
            for (int i = 0; i < data.Length; i++)
                if (data[i] != expected[i]) return false;
            JxrBitWriter partial = new JxrBitWriter();
            if (partial.Write(5, 3) != JxrError.None || partial.BitCount != 3)
                return false;
            partial.AlignByte();
            data = partial.ToArray();
            return partial.BitCount == 8 && data.Length == 1 && data[0] == 0xa0;
        }

        private static bool TestMinimalEncoderEndToEnd()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"))) directory = directory.Parent;
            if (directory == null) return false;
            byte[] bmp = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"));
            byte[] native = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            byte[] encoded;
            JxrError error = JxrMinimalEncoder.EncodeGrayBmp(bmp, out encoded);
            if (error != JxrError.None || encoded == null)
            { Console.WriteLine("Minimal encode error: " + error); return false; }
            if (encoded.Length != native.Length)
            { Console.WriteLine("JXR length: " + encoded.Length + " vs " + native.Length); return false; }
            for (int index = 0; index < native.Length; index++)
                if (encoded[index] != native[index])
                { Console.WriteLine("JXR first difference: " + index + " " +
                    encoded[index] + " vs " + native[index]); return false; }
            byte[] restored;
            if (JxrMinimalDecoder.DecodeGrayBmp(encoded, out restored) != JxrError.None ||
                restored == null || restored.Length != bmp.Length) return false;
            for (int index = 0; index < bmp.Length; index++)
                if (restored[index] != bmp[index]) return false;
            for (int variant = 0; variant < 18; variant++)
            {
                byte[] changed = (byte[])bmp.Clone();
                for (int y = 0; y < 16; y++)
                    for (int x = 0; x < 16; x++)
                    {
                        int value = variant == 0 ? 0 : variant == 1 ? 255 :
                            variant == 2 ? 128 : variant == 3 ?
                            ((x + y) & 1) * 255 : variant == 4 ?
                            x * 17 : (x * (73 + variant * 11) +
                            y * (131 + variant * 7) + x * y * (7 + variant)) & 255;
                        changed[1078 + (15 - y) * 16 + x] = (byte)value;
                    }
                error = JxrMinimalEncoder.EncodeGrayBmp(changed, out encoded);
                if (error != JxrError.None || encoded == null)
                { Console.WriteLine("Variant encode " + variant + ": " + error); return false; }
                error = JxrMinimalDecoder.DecodeGrayBmp(encoded, out restored);
                if (error != JxrError.None || restored == null ||
                    restored.Length != changed.Length)
                { Console.WriteLine("Variant decode " + variant + ": " + error); return false; }
                for (int index = 0; index < changed.Length; index++)
                    if (restored[index] != changed[index])
                    { Console.WriteLine("Variant " + variant + " BMP difference at " + index);
                      return false; }
            }
            byte[] invalid = (byte[])bmp.Clone();
            invalid[54] = 1;
            if (JxrMinimalEncoder.EncodeGrayBmp(invalid, out encoded) !=
                JxrError.UnsupportedFeature || encoded != null) return false;
            return true;
        }

        private static bool TestGraySizesNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\gray-1x1.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            int[,] sizes = { { 1, 1 }, { 15, 17 }, { 16, 16 },
                { 32, 32 }, { 31, 19 }, { 17, 1 }, { 33, 18 },
                { 257, 17 } };
            for (int item = 0; item < sizes.GetLength(0); item++)
            {
                int width = sizes[item, 0], height = sizes[item, 1];
                string prefix = Path.Combine(directory.FullName, "managed\\fixtures\\gray-" +
                    width + "x" + height);
                byte[] bmp = File.ReadAllBytes(prefix + ".bmp");
                byte[] native = File.ReadAllBytes(prefix + ".jxr");
                JxrImage image, decoded;
                if (JxrBmpAdapter.ReadGray8(bmp, out image) != JxrError.None ||
                    image.Width != width || image.Height != height)
                { Console.WriteLine("BMP read: " + width + "x" + height); return false; }
                byte[] rewrittenBmp;
                if (JxrBmpAdapter.WriteGray8(image, out rewrittenBmp) != JxrError.None ||
                    !EqualBytes(rewrittenBmp, bmp))
                { Console.WriteLine("BMP write: " + width + "x" + height); return false; }
                JxrError error = JxrCodec.Decode(native, new JxrDecoderOptions(), out decoded);
                if (error != JxrError.None || !EqualBytes(decoded.Pixels, image.Pixels))
                { Console.WriteLine("Native decode: " + width + "x" + height + " " + error);
                  return false; }
                byte[] encoded;
                error = JxrCodec.Encode(image, new JxrEncoderOptions(), out encoded);
                if (error != JxrError.None || !EqualBytes(encoded, native))
                {
                    int difference = 0;
                    if (encoded != null)
                        while (difference < encoded.Length && difference < native.Length &&
                            encoded[difference] == native[difference]) difference++;
                    Console.WriteLine("Native encode: " + width + "x" + height +
                        " " + error + " first difference " + difference +
                        " sizes " + (encoded == null ? 0 : encoded.Length) + "/" + native.Length);
                    return false;
                }
                error = JxrCodec.Decode(encoded, new JxrDecoderOptions(), out decoded);
                if (error != JxrError.None || !EqualBytes(decoded.Pixels, image.Pixels))
                { Console.WriteLine("Round-trip: " + width + "x" + height + " " + error);
                  return false; }
            }
            return true;
        }

        private static int ReadJsonInt(string path, string key)
        {
            string json = File.ReadAllText(path);
            int position = json.IndexOf("\"" + key + "\"", StringComparison.Ordinal);
            if (position < 0) throw new InvalidDataException("Missing JSON field " + key);
            position = json.IndexOf(':', position) + 1;
            while (position < json.Length && Char.IsWhiteSpace(json[position])) position++;
            int end = position;
            if (end < json.Length && json[end] == '-') end++;
            while (end < json.Length && Char.IsDigit(json[end])) end++;
            return Int32.Parse(json.Substring(position, end - position));
        }

        private static int[] ReadJsonValues(string path)
        {
            string json = File.ReadAllText(path);
            int position = json.IndexOf("\"values\"", StringComparison.Ordinal);
            if (position < 0) throw new InvalidDataException("Missing values in " + path);
            position = json.IndexOf('[', position) + 1;
            int end = json.IndexOf(']', position);
            string[] fields = json.Substring(position, end - position).Split(',');
            int[] values = new int[fields.Length];
            for (int index = 0; index < fields.Length; index++)
                values[index] = Int32.Parse(fields[index].Trim());
            return values;
        }

        private static bool TestGrayQualityNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\q16-all.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtureRoot = Path.Combine(directory.FullName, "managed\\fixtures");
            byte[] bmp = File.ReadAllBytes(Path.Combine(fixtureRoot, "gray-31x19.bmp"));
            JxrImage source;
            if (JxrBmpAdapter.ReadGray8(bmp, out source) != JxrError.None) return false;
            string[] names = { "q1-no-flex", "q2-all", "q16-all", "q64-all",
                "q255-all", "q16-no-flex", "q16-trim3", "q16-trim15",
                "q16-no-hp", "q16-dc-only" };
            JxrGraySubbandMode[] subbands = { JxrGraySubbandMode.NoFlexbits,
                JxrGraySubbandMode.All, JxrGraySubbandMode.All,
                JxrGraySubbandMode.All, JxrGraySubbandMode.All,
                JxrGraySubbandMode.NoFlexbits, JxrGraySubbandMode.All,
                JxrGraySubbandMode.All, JxrGraySubbandMode.NoHighpass,
                JxrGraySubbandMode.DcOnly };
            int[] qualities = { 1, 2, 16, 64, 255, 16, 16, 16, 16, 16 };
            int[] trims = { 0, 0, 0, 0, 0, 0, 3, 15, 0, 0 };
            for (int item = 0; item < names.Length; item++)
            {
                JxrEncoderOptions options = new JxrEncoderOptions();
                options.QualityIndex = qualities[item];
                options.Subbands = subbands[item];
                options.TrimFlexbits = trims[item];
                byte[] encoded;
                JxrGrayEncodingTrace trace = new JxrGrayEncodingTrace();
                JxrError error = JxrCodec.Encode(source, options, out encoded, trace);
                byte[] native = File.ReadAllBytes(Path.Combine(fixtureRoot, names[item] + ".jxr"));
                if (error != JxrError.None || !EqualBytes(encoded, native))
                {
                    int difference = 0;
                    if (encoded != null)
                        while (difference < encoded.Length && difference < native.Length &&
                            encoded[difference] == native[difference]) difference++;
                    Console.WriteLine("Gray quality encode " + names[item] + ": " + error +
                        " difference " + difference + " lengths " +
                        (encoded == null ? 0 : encoded.Length) + "/" + native.Length);
                    return false;
                }
                {
                    JxrImage decoded, expected;
                    error = JxrCodec.Decode(native, new JxrDecoderOptions(), out decoded);
                    bool decodedMatches = error == JxrError.None;
                    if (decodedMatches)
                        decodedMatches = JxrBmpAdapter.ReadGray8(
                            File.ReadAllBytes(Path.Combine(fixtureRoot,
                                names[item] + "-restored.bmp")), out expected) == JxrError.None &&
                            EqualBytes(decoded.Pixels, expected.Pixels);
                    if (!decodedMatches)
                    { Console.WriteLine("Gray quality decode " + names[item] + ": " + error); return false; }
                }
                string traceDir = Path.Combine(fixtureRoot, names[item] + "-trace");
                if (trace.MacroblockCount != 4) return false;
                int entropyStart = ReadJsonInt(Path.Combine(traceDir,
                    "encoder-mb-000-000-bitstream-dc.json"), "bit_start");
                for (int y = 0; y < 2; y++)
                    for (int x = 0; x < 2; x++)
                    {
                        int index = y * 2 + x;
                        string prefix = Path.Combine(traceDir, "encoder-mb-" +
                            x.ToString("D3") + "-" + y.ToString("D3"));
                        JxrGrayMacroblockTrace actual = trace.GetMacroblock(index);
                        bool quantizedMatch = EqualInts(actual.QuantizedCoefficients,
                            ReadJsonValues(prefix + "-quantized_coefficients.json"));
                        bool predictedMatch = EqualInts(actual.PredictedCoefficients,
                            ReadJsonValues(prefix + "-predicted_coefficients.json"));
                        bool bitRangesMatch = actual.DcBitStart + entropyStart ==
                            ReadJsonInt(prefix + "-bitstream-dc.json", "bit_start") &&
                            actual.DcBitEnd + entropyStart ==
                            ReadJsonInt(prefix + "-bitstream-dc.json", "bit_end");
                        if (subbands[item] != JxrGraySubbandMode.DcOnly)
                            bitRangesMatch = bitRangesMatch &&
                                actual.LpBitStart + entropyStart ==
                                ReadJsonInt(prefix + "-bitstream-lp.json", "bit_start") &&
                                actual.LpBitEnd + entropyStart ==
                                ReadJsonInt(prefix + "-bitstream-lp.json", "bit_end");
                        if ((int)subbands[item] < (int)JxrGraySubbandMode.NoHighpass)
                            bitRangesMatch = bitRangesMatch &&
                                actual.HpBitStart + entropyStart ==
                                ReadJsonInt(prefix + "-bitstream-hp.json", "bit_start") &&
                                actual.HpBitEnd + entropyStart ==
                                ReadJsonInt(prefix + "-bitstream-hp.json", "bit_end");
                        if (!quantizedMatch || !predictedMatch || !bitRangesMatch)
                        { Console.WriteLine("Gray quality trace differs for " + names[item] +
                            " at macroblock " + x + "," + y + " (quantized=" +
                            quantizedMatch + ", predicted=" + predictedMatch +
                            ", bit ranges=" + bitRangesMatch + ")"); return false; }
                    }
            }
            return true;
        }

        private static bool TestGrayMixedQpOptions()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\gray-31x19.bmp"))) directory = directory.Parent;
            if (directory == null) return false;
            JxrImage source;
            if (JxrBmpAdapter.ReadGray8(File.ReadAllBytes(Path.Combine(directory.FullName,
                "managed\\fixtures\\gray-31x19.bmp")), out source) != JxrError.None)
                return false;
            JxrEncoderOptions options = new JxrEncoderOptions();
            options.QualityIndex = 16;
            options.DcQuantizerIndex = 3;
            options.LowpassQuantizerIndex = 29;
            options.HighpassQuantizerIndex = 93;
            byte[] encoded;
            if (JxrCodec.Encode(source, options, out encoded) != JxrError.None)
                return false;
            JxrHeaders headers;
            if (JxrHeaders.Read(encoded, out headers) != JxrError.None ||
                headers.Quantizers.GetDcIndex(0) != 3 ||
                headers.Quantizers.GetLowpassIndex(0) != 29 ||
                headers.Quantizers.GetHighpassIndex(0) != 93)
                return false;
            JxrImage decoded;
            if (JxrCodec.Decode(encoded, new JxrDecoderOptions(), out decoded) !=
                JxrError.None || decoded.Width != source.Width ||
                decoded.Height != source.Height) return false;
            options.HighpassQuantizerIndex = 256;
            if (JxrCodec.Encode(source, options, out encoded) !=
                JxrError.InvalidArgument || encoded != null) return false;
            options.HighpassQuantizerIndex = -2;
            return JxrCodec.Encode(source, options, out encoded) ==
                JxrError.InvalidArgument && encoded == null;
        }

        private static bool EqualBytes(byte[] left, byte[] right)
        {
            if (left == null || right == null || left.Length != right.Length)
                return false;
            for (int index = 0; index < left.Length; index++)
                if (left[index] != right[index]) return false;
            return true;
        }

        private static bool EqualInts(int[] left, int[] right)
        {
            if (left == null || right == null || left.Length != right.Length)
                return false;
            for (int index = 0; index < left.Length; index++)
                if (left[index] != right[index]) return false;
            return true;
        }

        private static bool TestHeaderWriterFixture()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            byte[] fixture = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            JxrBitWriter header = new JxrBitWriter();
            if (JxrHeaderWriter.WriteGraySpatial(header, 16, 16, 0) !=
                JxrError.None || header.BitCount != 21 * 8) return false;
            byte[] headerBytes = header.ToArray();
            for (int index = 0; index < headerBytes.Length; index++)
                if (headerBytes[index] != fixture[134 + index]) return false;
            byte[] entropy = new byte[fixture.Length - 165];
            Array.Copy(fixture, 165, entropy, 0, entropy.Length);
            byte[] codestream;
            if (JxrCodestreamWriter.WriteGraySpatial(entropy, 16, 16, 0,
                out codestream) != JxrError.None ||
                codestream.Length != fixture.Length - 134) return false;
            for (int index = 0; index < codestream.Length; index++)
                if (codestream[index] != fixture[134 + index]) return false;
            byte[] container;
            if (JxrContainerWriter.WriteGray8(codestream, 16, 16,
                95.9866f, 95.9866f, out container) != JxrError.None ||
                !EqualBytes(container, fixture)) return false;
            return true;
        }

        private static bool TestHeaderWriterFields()
        {
            byte[] entropy = { 0x12, 0x34, 0x56 };
            byte[] codestream;
            if (JxrCodestreamWriter.WriteGraySpatial(entropy, 32, 48, 17,
                out codestream) != JxrError.None || codestream.Length != 34 ||
                codestream[31] != 0x12 || codestream[33] != 0x56)
            { Console.WriteLine("Codestream layout"); return false; }
            JxrHeaders headers;
            if (JxrHeaders.Read(codestream, out headers) != JxrError.None ||
                headers.CodestreamOffset != 0 || headers.ByteCount != 21 ||
                headers.Main.Width != 32 || headers.Main.Height != 48 ||
                headers.Quantizers.GetDcIndex(0) != 17 ||
                headers.Quantizers.GetLowpassIndex(0) != 17 ||
                headers.Quantizers.GetHighpassIndex(0) != 17)
            { Console.WriteLine("Header parse"); return false; }
            JxrBitReader reader = new JxrBitReader(codestream);
            int prefixBits = 27 * 8;
            while (prefixBits > 0)
            {
                int count = Math.Min(prefixBits, 32);
                if (reader.ConsumeBits(count) != JxrError.None) return false;
                prefixBits -= count;
            }
            JxrPacketHeader packet;
            if (JxrPacketReader.ReadHeader(reader, out packet) != JxrError.None ||
                !packet.IsValid || packet.TileId != 0 || packet.PacketType != 0)
            { Console.WriteLine("Packet parse"); return false; }
            byte[] container;
            if (JxrContainerWriter.WriteGray8(codestream, 32, 48,
                72, 144, out container) != JxrError.None ||
                container.Length != 134 + codestream.Length ||
                BitConverter.ToInt32(container, 66) != 32 ||
                BitConverter.ToInt32(container, 78) != 48 ||
                BitConverter.ToInt32(container, 114) != 134 ||
                BitConverter.ToInt32(container, 126) != codestream.Length)
            { Console.WriteLine("Container layout"); return false; }
            if (JxrHeaders.Read(container, out headers) != JxrError.None ||
                headers.CodestreamOffset != 134 || headers.Main.Width != 32 ||
                headers.Main.Height != 48)
            { Console.WriteLine("Container parse"); return false; }
            if (JxrCodestreamWriter.WriteGraySpatial(entropy, 4096, 16, 0,
                out codestream) != JxrError.None ||
                JxrHeaders.Read(codestream, out headers) != JxrError.None ||
                headers.ByteCount != 25 || headers.Main.Width != 4096 ||
                headers.Main.Height != 16)
            { Console.WriteLine("Extended dimensions"); return false; }
            JxrBitWriter writer = new JxrBitWriter();
            if (JxrPacketWriter.WriteHeader(writer, 5, 3) != JxrError.None ||
                !EqualBytes(writer.ToArray(), new byte[] { 0, 0, 1, 43 }) ||
                JxrPacketWriter.WriteHeader(writer, 32, 0) !=
                JxrError.InvalidArgument ||
                JxrHeaderWriter.WriteGraySpatial(new JxrBitWriter(),
                    0, 16, 0) != JxrError.InvalidArgument)
            { Console.WriteLine("Writer validation"); return false; }
            writer = new JxrBitWriter();
            if (JxrVariableLengthWordWriter.Write(writer, 0xfaff) !=
                JxrError.None || !EqualBytes(writer.ToArray(),
                new byte[] { 0xfa, 0xff })) return false;
            writer = new JxrBitWriter();
            if (JxrVariableLengthWordWriter.Write(writer, 0xfb00) !=
                JxrError.None || !EqualBytes(writer.ToArray(),
                new byte[] { 0xfb, 0, 0, 0xfb, 0 })) return false;
            return true;
        }

        private static bool TestPlanarAlphaNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\alpha-planar-q1-32x32.jxr")))
                directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            byte[] sourceBitmap = File.ReadAllBytes(Path.Combine(fixtures,
                "alpha-bgra-32x32.bmp"));
            byte[] sourcePixels;
            int width, height;
            JxrImage source;
            if (JxrBmpAdapter.ReadBgra32(sourceBitmap, out source) != JxrError.None ||
                source.Width != 32 || source.Height != 32) return false;
            sourcePixels = source.Pixels;
            width = source.Width;
            height = source.Height;
            byte[] rewrittenBmp;
            JxrImage bmpRoundTrip;
            if (JxrBmpAdapter.WriteBgra32(source, out rewrittenBmp) != JxrError.None ||
                JxrBmpAdapter.ReadBgra32(rewrittenBmp, out bmpRoundTrip) != JxrError.None ||
                !EqualBytes(source.Pixels, bmpRoundTrip.Pixels)) return false;
            int[] alphaQualities = { 1, 16 };
            for (int item = 0; item < alphaQualities.Length; item++)
            {
                string name = "alpha-planar-q" + alphaQualities[item] + "-32x32";
                byte[] native = File.ReadAllBytes(Path.Combine(fixtures, name + ".jxr"));
                JxrHeaders headers;
                JxrError headerError = JxrHeaders.Read(native, out headers);
                if (headerError != JxrError.None || headers == null ||
                    !headers.HasPlanarAlpha || headers.AlphaOffset <
                    headers.CodestreamOffset + headers.CodestreamLength ||
                    headers.AlphaByteCount != native.Length - headers.AlphaOffset)
                {
                    Console.WriteLine("Planar alpha header differs for " + name +
                        ": " + headerError + " offset=" + (headers == null ? -1 : headers.AlphaOffset) +
                        " bytes=" + (headers == null ? -1 : headers.AlphaByteCount) +
                        " length=" + native.Length);
                    return false;
                }
                JxrEncoderOptions encodeOptions = new JxrEncoderOptions();
                encodeOptions.QualityIndex = 16;
                encodeOptions.AlphaQualityIndex = alphaQualities[item];
                byte[] encoded;
                JxrError error = JxrCodec.Encode(source, encodeOptions, out encoded);
                if (error != JxrError.None || !EqualBytes(encoded, native))
                {
                    Console.WriteLine("Planar alpha encode differs for " + name +
                        ": " + error + " managed=" + (encoded == null ? -1 : encoded.Length) +
                        " native=" + native.Length);
                    if (encoded != null)
                        for (int index = 0; index < Math.Min(encoded.Length, native.Length); index++)
                            if (encoded[index] != native[index])
                            {
                                Console.WriteLine("First alpha JXR mismatch at " + index +
                                    ": " + encoded[index] + "/" + native[index]);
                                break;
                            }
                    return false;
                }

                JxrDecoderOptions alphaOptions = new JxrDecoderOptions();
                alphaOptions.OutputFormat = JxrPixelFormat.Gray8;
                alphaOptions.AlphaMode = JxrAlphaDecodeMode.AlphaOnly;
                JxrImage alpha;
                error = JxrCodec.Decode(native, alphaOptions, out alpha);
                JxrImage nativeAlpha = null;
                byte[] alphaBitmap = File.ReadAllBytes(Path.Combine(fixtures,
                    name + "-only.bmp"));
                Array.Copy(BitConverter.GetBytes(3779), 0, alphaBitmap, 38, 4);
                Array.Copy(BitConverter.GetBytes(3779), 0, alphaBitmap, 42, 4);
                if (error != JxrError.None || alpha == null ||
                    JxrBmpAdapter.ReadGray8(alphaBitmap, out nativeAlpha) != JxrError.None ||
                    !EqualBytes(alpha.Pixels, nativeAlpha.Pixels))
                {
                    Console.WriteLine("Planar alpha-only decode differs for " + name +
                        ": " + error);
                    if (alpha != null && nativeAlpha != null)
                        for (int pixel = 0; pixel < Math.Min(alpha.Pixels.Length,
                            nativeAlpha.Pixels.Length); pixel++)
                            if (alpha.Pixels[pixel] != nativeAlpha.Pixels[pixel])
                            {
                                Console.WriteLine("First alpha pixel mismatch at " + pixel +
                                    ": " + alpha.Pixels[pixel] + "/" +
                                    nativeAlpha.Pixels[pixel]);
                                break;
                            }
                    return false;
                }

                JxrDecoderOptions fullOptions = new JxrDecoderOptions();
                fullOptions.OutputFormat = JxrPixelFormat.Bgra32;
                JxrImage full;
                error = JxrCodec.Decode(native, fullOptions, out full);
                byte[] nativeFull;
                int fullWidth, fullHeight;
                if (error != JxrError.None || full == null ||
                    !ReadBgra32(File.ReadAllBytes(Path.Combine(fixtures,
                        name + "-restored.bmp")), out nativeFull,
                        out fullWidth, out fullHeight) || fullWidth != width ||
                    fullHeight != height || !EqualBytes(full.Pixels, nativeFull))
                {
                    Console.WriteLine("Planar alpha full decode differs for " + name +
                        ": " + error);
                    return false;
                }
                if (item == 0)
                {
                    byte[] rgbaPixels = new byte[sourcePixels.Length];
                    for (int pixel = 0; pixel < width * height; pixel++)
                    {
                        rgbaPixels[pixel * 4] = sourcePixels[pixel * 4 + 2];
                        rgbaPixels[pixel * 4 + 1] = sourcePixels[pixel * 4 + 1];
                        rgbaPixels[pixel * 4 + 2] = sourcePixels[pixel * 4];
                        rgbaPixels[pixel * 4 + 3] = sourcePixels[pixel * 4 + 3];
                    }
                    JxrImage rgbaSource = new JxrImage(width, height,
                        JxrPixelFormat.Rgba32, rgbaPixels, width * 4);
                    byte[] rgbaJxr;
                    if (JxrCodec.Encode(rgbaSource, encodeOptions, out rgbaJxr) !=
                        JxrError.None) return false;
                    JxrDecoderOptions rgbaOptions = new JxrDecoderOptions();
                    rgbaOptions.OutputFormat = JxrPixelFormat.Rgba32;
                    JxrImage rgbaRestored;
                    if (JxrCodec.Decode(rgbaJxr, rgbaOptions, out rgbaRestored) !=
                        JxrError.None || rgbaRestored == null) return false;
                    for (int pixel = 0; pixel < width * height; pixel++)
                        if (rgbaRestored.Pixels[pixel * 4] != full.Pixels[pixel * 4 + 2] ||
                            rgbaRestored.Pixels[pixel * 4 + 1] != full.Pixels[pixel * 4 + 1] ||
                            rgbaRestored.Pixels[pixel * 4 + 2] != full.Pixels[pixel * 4] ||
                            rgbaRestored.Pixels[pixel * 4 + 3] != full.Pixels[pixel * 4 + 3])
                            return false;
                }

                JxrDecoderOptions colorOptions = new JxrDecoderOptions();
                colorOptions.OutputFormat = JxrPixelFormat.Bgr24;
                colorOptions.AlphaMode = JxrAlphaDecodeMode.ColorOnly;
                JxrImage color;
                error = JxrCodec.Decode(native, colorOptions, out color);
                byte[] nativeColor;
                int colorWidth, colorHeight;
                if (error != JxrError.None || color == null ||
                    !ReadBgra32(File.ReadAllBytes(Path.Combine(fixtures,
                        name + "-color.bmp")), out nativeColor,
                        out colorWidth, out colorHeight) || colorWidth != width ||
                    colorHeight != height)
                {
                    Console.WriteLine("Planar alpha color-only decode failed for " +
                        name + ": " + error);
                    return false;
                }
                for (int pixel = 0; pixel < width * height; pixel++)
                    if (color.Pixels[pixel * 3] != nativeColor[pixel * 4] ||
                        color.Pixels[pixel * 3 + 1] != nativeColor[pixel * 4 + 1] ||
                        color.Pixels[pixel * 3 + 2] != nativeColor[pixel * 4 + 2])
                    {
                        Console.WriteLine("Planar alpha color-only pixels differ for " +
                            name + " at " + pixel);
                        return false;
                    }
            }

            JxrEncoderOptions frequencyTiles = new JxrEncoderOptions();
            frequencyTiles.QualityIndex = 16;
            frequencyTiles.AlphaQualityIndex = 1;
            frequencyTiles.Layout = JxrBitstreamLayout.Frequency;
            frequencyTiles.TileLayout = new JxrTileLayout(
                new int[] { 1, 1 }, new int[] { 1, 1 });
            byte[] tiled;
            if (JxrCodec.Encode(source, frequencyTiles, out tiled) !=
                JxrError.None) return false;
            JxrHeaders tiledHeaders;
            if (JxrHeaders.Read(tiled, out tiledHeaders) != JxrError.None ||
                !tiledHeaders.HasPlanarAlpha ||
                tiledHeaders.AlphaByteCount != tiled.Length - tiledHeaders.AlphaOffset)
                return false;
            JxrImage tiledRestored;
            JxrDecoderOptions tiledDecodeOptions = new JxrDecoderOptions();
            tiledDecodeOptions.OutputFormat = JxrPixelFormat.Bgra32;
            if (JxrCodec.Decode(tiled, tiledDecodeOptions, out tiledRestored) !=
                JxrError.None || tiledRestored == null || tiledRestored.Width != width ||
                tiledRestored.Height != height) return false;
            for (int pixel = 0; pixel < width * height; pixel++)
                if (tiledRestored.Pixels[pixel * 4 + 3] != sourcePixels[pixel * 4 + 3])
                    return false;

            byte[] q1 = File.ReadAllBytes(Path.Combine(fixtures,
                "alpha-planar-q1-32x32.jxr"));
            uint directoryOffset = BitConverter.ToUInt32(q1, 4);
            int directoryPosition = (int)directoryOffset;
            int entries = BitConverter.ToUInt16(q1, directoryPosition);
            byte[] badOffset = (byte[])q1.Clone();
            bool changed = false;
            for (int index = 0; index < entries; index++)
            {
                int entry = directoryPosition + 2 + index * 12;
                if (BitConverter.ToUInt16(badOffset, entry) == 0xbcc2)
                {
                    Array.Copy(BitConverter.GetBytes((uint)(q1.Length - 1)), 0,
                        badOffset, entry + 8, 4);
                    changed = true;
                }
            }
            JxrHeaders ignored;
            if (!changed || JxrHeaders.Read(badOffset, out ignored) == JxrError.None)
                return false;
            byte[] badEnd = (byte[])q1.Clone();
            changed = false;
            for (int index = 0; index < entries; index++)
            {
                int entry = directoryPosition + 2 + index * 12;
                if (BitConverter.ToUInt16(badEnd, entry) == 0xbcc3)
                {
                    Array.Copy(BitConverter.GetBytes((uint)(q1.Length + 1)), 0,
                        badEnd, entry + 8, 4);
                    changed = true;
                }
            }
            if (!changed || JxrHeaders.Read(badEnd, out ignored) == JxrError.None)
                return false;
            byte[] truncated = new byte[q1.Length - 1];
            Array.Copy(q1, truncated, truncated.Length);
            if (JxrHeaders.Read(truncated, out ignored) == JxrError.None)
                return false;

            JxrEncoderOptions unsupported = new JxrEncoderOptions();
            unsupported.AlphaMode = JxrAlphaMode.Interleaved;
            byte[] rejected;
            if (JxrCodec.Encode(source, unsupported, out rejected) !=
                JxrError.UnsupportedFeature || rejected != null) return false;
            byte[] interleaved = File.ReadAllBytes(Path.Combine(fixtures,
                "alpha-interleaved-q1-32x32.jxr"));
            JxrDecoderOptions interleavedOptions = new JxrDecoderOptions();
            interleavedOptions.OutputFormat = JxrPixelFormat.Bgra32;
            JxrImage interleavedImage;
            return JxrCodec.Decode(interleaved, interleavedOptions,
                out interleavedImage) != JxrError.None && interleavedImage == null;
        }

        private static bool TestPbgraAlphaConversionVectors()
        {
            const int width = 32, height = 32;
            byte[] pixels = new byte[width * height * 4];
            byte[] alphaValues = { 0, 1, 2, 32, 63, 64, 127, 128,
                191, 200, 254, 255 };
            for (int pixel = 0; pixel < width * height; pixel++)
            {
                int offset = pixel * 4;
                byte alpha = alphaValues[pixel % alphaValues.Length];
                pixels[offset] = (byte)(alpha / 3);
                pixels[offset + 1] = (byte)(alpha / 2);
                pixels[offset + 2] = alpha;
                pixels[offset + 3] = alpha;
            }
            JxrImage input = new JxrImage(width, height,
                JxrPixelFormat.Bgra32, pixels, width * 4);
            JxrEncoderOptions encodeOptions = new JxrEncoderOptions();
            encodeOptions.QualityIndex = 1;
            encodeOptions.AlphaQualityIndex = 1;
            encodeOptions.Layout = JxrBitstreamLayout.Frequency;
            byte[] encoded;
            if (JxrCodec.Encode(input, encodeOptions, out encoded) !=
                JxrError.None) return false;
            Guid oldGuid = new Guid("6fddc324-4e03-4bfe-b185-3d77768dc90f");
            Guid pbgraGuid = new Guid("6fddc324-4e03-4bfe-b185-3d77768dc910");
            byte[] sourceGuid = oldGuid.ToByteArray();
            byte[] targetGuid = pbgraGuid.ToByteArray();
            int guidOffset = FindBytes(encoded, sourceGuid);
            if (guidOffset < 0 || FindBytes(encoded, sourceGuid, guidOffset + 1) >= 0)
                return false;
            Array.Copy(targetGuid, 0, encoded, guidOffset, targetGuid.Length);
            JxrHeaders headers;
            if (JxrHeaders.Read(encoded, out headers) != JxrError.None ||
                !headers.HasPlanarAlpha ||
                !String.Equals(headers.PixelFormatGuid, pbgraGuid.ToString(),
                    StringComparison.OrdinalIgnoreCase)) return false;

            JxrDecoderOptions options = new JxrDecoderOptions();
            options.AlphaMode = JxrAlphaDecodeMode.ColorAndAlpha;
            options.OutputFormat = JxrPixelFormat.Pbgra32;
            JxrImage decoded;
            if (JxrCodec.Decode(encoded, options, out decoded) != JxrError.None ||
                decoded == null || decoded.Format != JxrPixelFormat.Pbgra32 ||
                !EqualBytes(decoded.Pixels, pixels)) return false;

            options.OutputFormat = JxrPixelFormat.Bgra32;
            JxrImage straight;
            if (JxrCodec.Decode(encoded, options, out straight) != JxrError.None ||
                straight == null || straight.Format != JxrPixelFormat.Bgra32)
                return false;
            for (int pixel = 0; pixel < width * height; pixel++)
            {
                int offset = pixel * 4;
                byte alpha = pixels[offset + 3];
                for (int channel = 0; channel < 3; channel++)
                {
                    int expected = alpha == 0 ? 0 :
                        (pixels[offset + channel] * 255 + alpha / 2) / alpha;
                    if (expected > 255) expected = 255;
                    if (straight.Pixels[offset + channel] != expected) return false;
                }
                if (straight.Pixels[offset + 3] != alpha) return false;
            }

            options.OutputFormat = JxrPixelFormat.Rgba32;
            JxrImage rgba;
            if (JxrCodec.Decode(encoded, options, out rgba) != JxrError.None ||
                rgba == null || rgba.Format != JxrPixelFormat.Rgba32) return false;
            for (int pixel = 0; pixel < width * height; pixel++)
            {
                int offset = pixel * 4;
                if (rgba.Pixels[offset] != straight.Pixels[offset + 2] ||
                    rgba.Pixels[offset + 1] != straight.Pixels[offset + 1] ||
                    rgba.Pixels[offset + 2] != straight.Pixels[offset] ||
                    rgba.Pixels[offset + 3] != straight.Pixels[offset + 3]) return false;
            }
            return true;
        }

        private static int FindBytes(byte[] data, byte[] value)
        { return FindBytes(data, value, 0); }

        private static int FindBytes(byte[] data, byte[] value, int start)
        {
            for (int index = start; index <= data.Length - value.Length; index++)
            {
                int item;
                for (item = 0; item < value.Length; item++)
                    if (data[index + item] != value[item]) break;
                if (item == value.Length) return index;
            }
            return -1;
        }

        private static bool ReadBgra32(byte[] bitmap, out byte[] pixels,
            out int width, out int height)
        {
            pixels = null;
            width = height = 0;
            if (bitmap == null || bitmap.Length < 54 || bitmap[0] != 'B' ||
                bitmap[1] != 'M' || BitConverter.ToInt32(bitmap, 14) != 40 ||
                BitConverter.ToUInt16(bitmap, 26) != 1 ||
                BitConverter.ToUInt16(bitmap, 28) != 32 ||
                BitConverter.ToUInt32(bitmap, 30) != 0) return false;
            width = BitConverter.ToInt32(bitmap, 18);
            int signedHeight = BitConverter.ToInt32(bitmap, 22);
            if (signedHeight == Int32.MinValue) return false;
            height = signedHeight < 0 ? -signedHeight : signedHeight;
            int offset = BitConverter.ToInt32(bitmap, 10);
            if (width < 1 || height < 1 || offset < 54 ||
                (long)width * height > Int32.MaxValue / 4 ||
                (long)offset + (long)width * height * 4 != bitmap.Length)
                return false;
            pixels = new byte[width * height * 4];
            bool topDown = signedHeight < 0;
            for (int row = 0; row < height; row++)
            {
                int sourceRow = topDown ? row : height - 1 - row;
                Array.Copy(bitmap, offset + sourceRow * width * 4,
                    pixels, row * width * 4, width * 4);
            }
            return true;
        }

        private static bool TestPublicPixelApi()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"))) directory = directory.Parent;
            if (directory == null) return false;
            byte[] bmp = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"));
            byte[] expectedJxr = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            JxrImage image;
            if (JxrBmpAdapter.ReadGray8(bmp, out image) != JxrError.None ||
                image.Width != 16 || image.Height != 16 || image.Stride != 16 ||
                image.Format != JxrPixelFormat.Gray8) return false;
            byte[] pixels = image.Pixels;
            byte[] padded = new byte[16 * 20];
            for (int row = 0; row < 16; row++)
                Array.Copy(pixels, row * 16, padded, row * 20, 16);
            JxrImage paddedImage = new JxrImage(16, 16,
                JxrPixelFormat.Gray8, padded, 20);
            JxrEncoderOptions options = new JxrEncoderOptions();
            byte[] encoded;
            if (JxrCodec.Encode(paddedImage, options, out encoded) !=
                JxrError.None || !EqualBytes(encoded, expectedJxr)) return false;
            JxrImage decoded;
            if (JxrCodec.Decode(encoded, new JxrDecoderOptions(), out decoded) !=
                JxrError.None || !EqualBytes(decoded.Pixels, pixels)) return false;
            byte[] restoredBmp;
            if (JxrBmpAdapter.WriteGray8(decoded, out restoredBmp) !=
                JxrError.None || !EqualBytes(restoredBmp, bmp)) return false;
            options.Overlap = 1;
            if (JxrCodec.Encode(image, options, out encoded) !=
                JxrError.None || encoded == null ||
                JxrCodec.Decode(encoded, new JxrDecoderOptions(), out decoded) !=
                JxrError.None || !EqualBytes(decoded.Pixels, pixels)) return false;
            options.Overlap = 0;
            options.ChromaSubsampling = JxrChromaSubsampling.Yuv420;
            if (JxrCodec.Encode(image, options, out encoded) !=
                JxrError.InvalidArgument || encoded != null) return false;
            options.ChromaSubsampling = JxrChromaSubsampling.Yuv444;
            options.QualityIndex = 0;
            if (JxrCodec.Encode(image, options, out encoded) !=
                JxrError.InvalidArgument || encoded != null) return false;
            options.QualityIndex = 1;
            options.Layout = JxrBitstreamLayout.Frequency;
            JxrError frequencyEncodeError = JxrCodec.Encode(image, options, out encoded);
            if (frequencyEncodeError != JxrError.None || encoded == null) return false;
            JxrError frequencyDecodeError = JxrCodec.Decode(encoded,
                new JxrDecoderOptions(), out decoded);
            if (frequencyDecodeError != JxrError.None ||
                !EqualBytes(decoded.Pixels, pixels)) return false;
            if (JxrCodec.Encode(image, options, out encoded,
                new JxrGrayEncodingTrace()) != JxrError.UnsupportedFeature ||
                encoded != null) return false;
            JxrImage rgb = new JxrImage(16, 16, JxrPixelFormat.Rgb24,
                new byte[16 * 16 * 3], 48);
            if (JxrCodec.Encode(rgb, new JxrEncoderOptions(), out encoded) !=
                JxrError.None || encoded == null) return false;
            JxrDecoderOptions decoderOptions = new JxrDecoderOptions();
            decoderOptions.OutputFormat = JxrPixelFormat.Rgb24;
            if (JxrCodec.Decode(encoded, decoderOptions, out decoded) !=
                JxrError.None || decoded == null ||
                decoded.Format != JxrPixelFormat.Rgb24 ||
                !EqualBytes(decoded.Pixels, rgb.Pixels)) return false;
            if (JxrCodec.Decode(expectedJxr, decoderOptions, out decoded) !=
                JxrError.UnsupportedFeature || decoded != null) return false;
            try { new JxrImage(16, 16, JxrPixelFormat.Gray8, new byte[255], 16); }
            catch (ArgumentException) { return true; }
            return false;
        }

        private sealed class NonSeekableStream : Stream
        {
            private readonly MemoryStream inner;
            internal bool FailRead;
            internal bool FailWrite;
            internal NonSeekableStream(byte[] initial)
            {
                inner = initial == null ? new MemoryStream() :
                    new MemoryStream(initial, false);
            }
            public override bool CanRead { get { return inner.CanRead; } }
            public override bool CanWrite { get { return inner.CanWrite; } }
            public override bool CanSeek { get { return false; } }
            public override long Length { get { throw new NotSupportedException(); } }
            public override long Position
            {
                get { throw new NotSupportedException(); }
                set { throw new NotSupportedException(); }
            }
            public override void Flush() { inner.Flush(); }
            public override int Read(byte[] buffer, int offset, int count)
            {
                if (FailRead) throw new IOException("test read failure");
                return inner.Read(buffer, offset, Math.Min(count, 7));
            }
            public override void Write(byte[] buffer, int offset, int count)
            {
                if (FailWrite) throw new IOException("test write failure");
                inner.Write(buffer, offset, count);
            }
            public override long Seek(long offset, SeekOrigin origin)
            { throw new NotSupportedException(); }
            public override void SetLength(long value)
            { throw new NotSupportedException(); }
            internal byte[] ToArray() { return inner.ToArray(); }
        }

        private static bool TestPublicStreamApi()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"))) directory = directory.Parent;
            if (directory == null) return false;
            byte[] bmp = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"));
            byte[] jxr = File.ReadAllBytes(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"));
            JxrImage image;
            if (JxrBmpAdapter.ReadGray8(bmp, out image) != JxrError.None)
                return false;
            NonSeekableStream destination = new NonSeekableStream(null);
            if (JxrCodec.Encode(image, new JxrEncoderOptions(), destination) !=
                JxrError.None || !EqualBytes(destination.ToArray(), jxr) ||
                !destination.CanWrite) return false;
            NonSeekableStream source = new NonSeekableStream(jxr);
            JxrImage decoded;
            if (JxrCodec.Decode(source, new JxrDecoderOptions(), out decoded) !=
                JxrError.None || !EqualBytes(decoded.Pixels, image.Pixels) ||
                !source.CanRead) return false;
            byte[] colorBmp = File.ReadAllBytes(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb444-15x17.bmp"));
            byte[] colorJxr = File.ReadAllBytes(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb444-15x17.jxr"));
            JxrImage color;
            if (JxrBmpAdapter.ReadRgb24(colorBmp, out color) != JxrError.None)
                return false;
            destination = new NonSeekableStream(null);
            if (JxrCodec.Encode(color, new JxrEncoderOptions(), destination) !=
                JxrError.None || !EqualBytes(destination.ToArray(), colorJxr))
                return false;
            JxrDecoderOptions colorOptions = new JxrDecoderOptions();
            colorOptions.OutputFormat = JxrPixelFormat.Rgb24;
            source = new NonSeekableStream(colorJxr);
            if (JxrCodec.Decode(source, colorOptions, out decoded) !=
                JxrError.None || !EqualBytes(decoded.Pixels, color.Pixels))
                return false;
            MemoryStream readOnly = new MemoryStream(new byte[0], false);
            if (JxrCodec.Encode(image, new JxrEncoderOptions(), readOnly) !=
                JxrError.InvalidArgument) return false;
            MemoryStream malformed = new MemoryStream(new byte[] { 1, 2, 3 });
            if (JxrCodec.Decode(malformed, new JxrDecoderOptions(), out decoded) ==
                JxrError.None || decoded != null) return false;
            source = new NonSeekableStream(jxr);
            source.FailRead = true;
            if (JxrCodec.Decode(source, new JxrDecoderOptions(), out decoded) !=
                JxrError.IoFailure || decoded != null) return false;
            destination = new NonSeekableStream(null);
            destination.FailWrite = true;
            if (JxrCodec.Encode(image, new JxrEncoderOptions(), destination) !=
                JxrError.IoFailure) return false;
            return true;
        }

        private static byte[] PackHeaderFields(int[] values, int[] widths)
        {
            if (values.Length != widths.Length) return null;
            int bitCount = 0;
            for (int field = 0; field < widths.Length; field++) bitCount += widths[field];
            byte[] bytes = new byte[(bitCount + 7) / 8];
            int bitPosition = 0;
            for (int field = 0; field < widths.Length; field++)
                for (int bit = widths[field] - 1; bit >= 0; bit--)
                {
                    if ((((uint)values[field] >> bit) & 1U) != 0)
                        bytes[bitPosition >> 3] |= (byte)(1 << (7 - (bitPosition & 7)));
                    bitPosition++;
                }
            return bytes;
        }

        // Translations of native main_header_reader_vectors,
        // image_plane_descriptor_reader_vectors and
        // image_plane_quantizer_header_reader_vectors.
        private static bool TestHeadersSyntaxVectors()
        {
            int[] mainValues = { 1,9,1,1,4,1,2,1,1,1,1,1,1,0,1,7,7,
                31,15,1,1,3,5,0,0,0,0,0,0,0,0 };
            int[] mainWidths = { 4,4,1,1,3,1,2,1,1,1,1,1,1,1,1,4,4,
                16,16,12,12,8,8,8,8,8,8,6,6,6,6 };
            JxrMainHeader main;
            JxrBitReader reader = new JxrBitReader(PackHeaderFields(mainValues, mainWidths));
            if (JxrHeaders.ReadMainHeader(reader, out main) != JxrError.None ||
                main.Version != 1 || main.Subversion != 9 || !main.HasHardTileBoundaries ||
                main.BitstreamFormat != 1 || main.Orientation != 4 ||
                !main.HasIndexTable || main.Overlap != 2 ||
                main.SourceColorFormat != 7 || main.SourceBitDepth != 7 ||
                main.Width != 32 || main.Height != 16 ||
                main.VerticalSliceCountMinusOne != 1 ||
                main.HorizontalSliceCountMinusOne != 1 ||
                main.GetTileX(1) != 3 || main.GetTileY(1) != 5 ||
                reader.BitPosition != 160)
                return false;

            int[][] planeValues = {
                new int[] { 0,1,0 },
                new int[] { 1,0,1,0,5,1,3,9 },
                new int[] { 2,0,2,1,6,0,10 },
                new int[] { 3,0,0,0,0 },
                new int[] { 4,0,0 },
                new int[] { 6,0,0,4,0,13,0x82 }
            };
            int[][] planeWidths = {
                new int[] { 3,1,4 },
                new int[] { 3,1,4,1,3,1,3,8 },
                new int[] { 3,1,4,1,3,4,8 },
                new int[] { 3,1,4,4,4 },
                new int[] { 3,1,4 },
                new int[] { 3,1,4,4,4,8,8 }
            };
            int[] depths = { 1,2,6,1,1,7 };
            int[] channels = { 1,3,3,3,4,5 };
            for (int format = 0; format < planeValues.Length; format++)
            {
                JxrImagePlaneHeader plane;
                reader = new JxrBitReader(PackHeaderFields(
                    planeValues[format], planeWidths[format]));
                if (JxrHeaders.ReadImagePlaneHeader(reader, depths[format], out plane) !=
                    JxrError.None || plane.ColorFormat != planeValues[format][0] ||
                    plane.ChannelCount != channels[format]) return false;
                if (format == 1 && (!plane.HasChromaCenteringX ||
                    !plane.HasChromaCenteringY || plane.ChromaCenteringX != 5 ||
                    plane.ChromaCenteringY != 3 || plane.MantissaOrShift != 9)) return false;
                if (format == 2 && (!plane.HasChromaCenteringX ||
                    plane.HasChromaCenteringY || plane.ChromaCenteringX != 6 ||
                    plane.MantissaOrShift != 10)) return false;
                if (format == 5 && (plane.MantissaOrShift != 13 ||
                    plane.ExponentBias != -126)) return false;
            }
            reader = new JxrBitReader(PackHeaderFields(
                new int[] { 5 }, new int[] { 3 }));
            JxrImagePlaneHeader invalidPlane;
            if (JxrHeaders.ReadImagePlaneHeader(reader, 1, out invalidPlane) !=
                JxrError.InvalidBitstream) return false;

            int[] quantizerValues = { 1,0,7,0,1,1,8,9,0,1,2,10,11,12 };
            int[] quantizerWidths = { 1,2,8,1,1,2,8,8,1,1,2,8,8,8 };
            reader = new JxrBitReader(PackHeaderFields(quantizerValues, quantizerWidths));
            JxrImagePlaneQuantizerHeader q;
            if (JxrHeaders.ReadImagePlaneQuantizerHeader(reader, 3, 0, out q) !=
                JxrError.None || q.Mode != 0x720 || !q.HasDc || !q.HasLowpass ||
                !q.HasHighpass || q.DcMode != 0 || q.LowpassMode != 1 ||
                q.HighpassMode != 2 || q.GetDcIndex(0) != 7 ||
                q.GetLowpassIndex(1) != 9 || q.GetHighpassIndex(2) != 12)
                return false;
            return JxrHeaders.ReadMainHeader(null, out main) == JxrError.InvalidArgument &&
                JxrHeaders.ReadImagePlaneQuantizerHeader(null, 3, 0, out q) ==
                JxrError.InvalidArgument;
        }

        private static ulong QuantizationHashValue(ulong hash, int value)
        {
            return unchecked((hash ^ (uint)value) * 1099511628211UL);
        }

        private static bool TestImagePipelineReferenceVectors()
        {
            const int width = 17, height = 3, rgbStride = 53, outputStride = 55;
            ulong hash = 14695981039346656037UL;
            for (int shift = 0; shift <= 3; shift += 3)
                for (int order = 0; order < 2; order++)
                {
                    byte[] source = new byte[rgbStride * height];
                    byte[] graySource = new byte[(width + 2) * height];
                    byte[] output = new byte[outputStride * height];
                    byte[] grayOutput = new byte[(width + 2) * height];
                    for (int row = 0; row < height; row++)
                        for (int column = 0; column < width; column++)
                        {
                            byte red = (byte)((row * 41 + column * 17) & 255);
                            byte green = (byte)((row * 83 + column * 29 + 127) & 255);
                            byte blue = (byte)((row * 13 + column * 47 + 255) & 255);
                            int offset = row * rgbStride + column * 3;
                            source[offset + (order == 0 ? 0 : 2)] = red;
                            source[offset + 1] = green;
                            source[offset + (order == 0 ? 2 : 0)] = blue;
                            graySource[row * (width + 2) + column] = green;
                        }
                    int[] y, u, v, gray;
                    if (JxrImagePipeline.EncodeRgb8(source, rgbStride, width, height,
                        order == 0, shift, out y, out u, out v) != JxrError.None ||
                        JxrImagePipeline.DecodeRgb8(y, u, v, width, height,
                        order == 0, shift, output, outputStride) != JxrError.None ||
                        JxrImagePipeline.EncodeGray8(graySource, width + 2, width,
                        height, shift, out gray) != JxrError.None ||
                        JxrImagePipeline.DecodeGray8(gray, width, height, shift,
                        grayOutput, width + 2) != JxrError.None) return false;
                    for (int row = 0; row < height; row++)
                        for (int column = 0; column < width; column++)
                        {
                            int index = row * width + column;
                            int inputIndex = row * rgbStride + column * 3;
                            int outputIndex = row * outputStride + column * 3;
                            hash = QuantizationHashValue(hash, y[index]);
                            hash = QuantizationHashValue(hash, u[index]);
                            hash = QuantizationHashValue(hash, v[index]);
                            for (int channel = 0; channel < 3; channel++)
                            {
                                byte sample = output[outputIndex + channel];
                                hash = QuantizationHashValue(hash, sample);
                                if (sample != source[inputIndex + channel]) return false;
                            }
                            hash = QuantizationHashValue(hash, gray[index]);
                            byte graySample = grayOutput[row * (width + 2) + column];
                            hash = QuantizationHashValue(hash, graySample);
                            if (graySample != graySource[row * (width + 2) + column])
                                return false;
                        }
                    for (int row = 0; row < height; row++)
                    {
                        if (output[row * outputStride + width * 3] != 0 ||
                            grayOutput[row * (width + 2) + width] != 0)
                            return false;
                    }
                }
            for (int value = -257; value <= 257; value += 17)
            {
                int c = value, m = value + 13, y = value - 31, k = value + 7;
                JxrImagePipeline.ForwardCmyk(ref c, ref m, ref y, ref k);
                hash = QuantizationHashValue(hash, c);
                hash = QuantizationHashValue(hash, m);
                hash = QuantizationHashValue(hash, y);
                hash = QuantizationHashValue(hash, k);
                JxrImagePipeline.InverseCmyk(ref c, ref m, ref y, ref k);
                if (c != value || m != value + 13 || y != value - 31 ||
                    k != value + 7) return false;
            }
            for (int value = -512; value <= 768; value += 17)
                hash = QuantizationHashValue(hash, JxrImagePipeline.ClipByte(value));
            int[] invalid;
            int[] iy, iu, iv;
            if (JxrImagePipeline.EncodeGray8(new byte[4], 1, 2, 2, 0,
                out invalid) != JxrError.InvalidArgument || invalid != null ||
                JxrImagePipeline.EncodeRgb8(new byte[3], 3, 1, 1, true, 1,
                out iy, out iu, out iv) != JxrError.InvalidArgument ||
                iy != null || iu != null || iv != null)
                return false;
            Console.WriteLine("Image pipeline signature: " + hash.ToString("X16"));
            return hash == 0xCC4DA223E707A8DBUL;
        }

        private static bool TestImagePipelineBitmapFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.bmp"))) directory = directory.Parent;
            if (directory == null) return false;
            string[] paths = {
                "minimal-profile\\minimal-gray-16x16.bmp",
                "real-image-profile\\test-sign-334x330.bmp",
                "default-profile\\city-park-605x478.bmp"
            };
            for (int fixture = 0; fixture < paths.Length; fixture++)
            {
                byte[] bitmap = File.ReadAllBytes(Path.Combine(directory.FullName,
                    paths[fixture]));
                if (bitmap.Length < 54 || bitmap[0] != 'B' || bitmap[1] != 'M' ||
                    BitConverter.ToInt32(bitmap, 30) != 0) return false;
                int offset = BitConverter.ToInt32(bitmap, 10);
                int width = BitConverter.ToInt32(bitmap, 18);
                int height = BitConverter.ToInt32(bitmap, 22);
                int bitsPerPixel = BitConverter.ToInt16(bitmap, 28);
                int channels = bitsPerPixel == 8 ? 1 : bitsPerPixel == 24 ? 3 : 0;
                if (channels == 0 || width <= 0 || height <= 0) return false;
                long strideLong = (((long)width * channels + 3) / 4) * 4;
                if (strideLong > Int32.MaxValue || offset < 0 ||
                    (long)offset + strideLong * height > bitmap.Length) return false;
                int stride = (int)strideLong;
                byte[] pixels = new byte[stride * height];
                byte[] restored = new byte[pixels.Length];
                Array.Copy(bitmap, offset, pixels, 0, pixels.Length);
                for (int shift = 0; shift <= 3; shift += 3)
                {
                    JxrError error;
                    if (channels == 1)
                    {
                        int[] y;
                        error = JxrImagePipeline.EncodeGray8(pixels, stride,
                            width, height, shift, out y);
                        if (error != JxrError.None) return false;
                        error = JxrImagePipeline.DecodeGray8(y, width, height,
                            shift, restored, stride);
                    }
                    else
                    {
                        int[] y, u, v;
                        error = JxrImagePipeline.EncodeRgb8(pixels, stride,
                            width, height, false, shift, out y, out u, out v);
                        if (error != JxrError.None) return false;
                        error = JxrImagePipeline.DecodeRgb8(y, u, v,
                            width, height, false, shift, restored, stride);
                    }
                    if (error != JxrError.None) return false;
                    for (int row = 0; row < height; row++)
                        for (int column = 0; column < width * channels; column++)
                            if (pixels[row * stride + column] !=
                                restored[row * stride + column]) return false;
                }
            }
            return true;
        }

        private static bool TestSessionMemoryReferenceVectors()
        {
            int[] widths = { 1, 16, 17, 605, 65536, 1048576 };
            int[] formats = { 0, 1, 2, 3, 4, 6 };
            int[] channels = { 1, 3, 3, 3, 4, 5 };
            ulong hash = 14695981039346656037UL;
            for (int width = 0; width < widths.Length; width++)
                for (int format = 0; format < formats.Length; format++)
                    for (int depth = 0; depth < 2; depth++)
                        for (int thirtyTwo = 0; thirtyTwo < 2; thirtyTwo++)
                        {
                            JxrSessionConfiguration config = new JxrSessionConfiguration(
                                widths[width], 16, formats[format], channels[format],
                                depth == 0 ? 2 : 4, false);
                            JxrSessionMemoryPlan decoder = JxrSessionPlanner.Decoder(
                                config, 100, 20, 30, thirtyTwo != 0);
                            JxrSessionMemoryPlan encoder = JxrSessionPlanner.Encoder(
                                config, 100, 30, thirtyTwo != 0);
                            hash = QuantizationHashValue(hash, decoder.AllocationIsSafe ? 1 : 0);
                            hash = QuantizationHashValue(hash, (int)decoder.ChannelBytes);
                            hash = QuantizationHashValue(hash, (int)decoder.ChromaBlockCount);
                            hash = QuantizationHashValue(hash, (int)decoder.MacroblockCount);
                            hash = QuantizationHashValue(hash,
                                (int)decoder.FullResolutionMacroblockBytes);
                            hash = QuantizationHashValue(hash, (int)decoder.ChromaMacroblockBytes);
                            hash = QuantizationHashValue(hash, (int)decoder.PrimaryMacroblockRowBytes);
                            hash = QuantizationHashValue(hash,
                                unchecked((int)decoder.PrimaryMacroblockBufferBytes));
                            hash = QuantizationHashValue(hash,
                                unchecked((int)decoder.PrimaryAllocationBytes));
                            hash = QuantizationHashValue(hash, encoder.AllocationIsSafe ? 1 : 0);
                            hash = QuantizationHashValue(hash, (int)encoder.MacroblockCount);
                            hash = QuantizationHashValue(hash,
                                (int)encoder.FullResolutionMacroblockBytes);
                            hash = QuantizationHashValue(hash, (int)encoder.ChromaMacroblockBytes);
                            hash = QuantizationHashValue(hash,
                                (int)encoder.PrimaryMacroblockRowBytes);
                            hash = QuantizationHashValue(hash,
                                unchecked((int)encoder.PrimaryMacroblockBufferBytes));
                            hash = QuantizationHashValue(hash,
                                unchecked((int)encoder.PrimaryAllocationBytes));
                            hash = QuantizationHashValue(hash,
                                unchecked((int)encoder.SecondaryMacroblockBufferBytes));
                            hash = QuantizationHashValue(hash,
                                unchecked((int)encoder.SecondaryAllocationBytes));
                        }
            Console.WriteLine("Session memory signature: " + hash.ToString("X16"));
            return hash == 0x45A6AAFA34554155UL;
        }

        private static bool TestSessionLifecycleVectors()
        {
            JxrSessionConfiguration gray = new JxrSessionConfiguration(
                16, 16, 0, 1, 4, false);
            JxrDecoderSession decoder = JxrDecoderSession.Create(gray, 100, 20, 30);
            JxrEncoderSession encoder = JxrEncoderSession.Create(gray, 100, 30);
            if (decoder.MemoryPlan.MacroblockCount != 1 ||
                encoder.MemoryPlan.MacroblockCount != 1 ||
                decoder.GetPrimaryRow(0, 0).Length != 256 ||
                encoder.GetPrimaryRow(1, 0).Length != 256 ||
                Object.ReferenceEquals(decoder.GetPrimaryRow(0, 0),
                    decoder.GetPrimaryRow(1, 0))) return false;
            decoder.GetPrimaryRow(0, 0)[0] = 73;
            if (decoder.GetPrimaryRow(1, 0)[0] != 0) return false;
            decoder.Dispose(); decoder.Dispose(); encoder.Dispose(); encoder.Dispose();
            if (!decoder.IsClosed || !encoder.IsClosed) return false;
            try { decoder.GetPrimaryRow(0, 0); return false; }
            catch (ObjectDisposedException) { }

            JxrSessionConfiguration color = new JxrSessionConfiguration(
                605, 478, 1, 3, 4, true);
            decoder = JxrDecoderSession.Create(color, 100, 20, 30);
            encoder = JxrEncoderSession.Create(color, 100, 30);
            if (decoder.MemoryPlan.MacroblockCount != 38 ||
                decoder.GetPrimaryRow(0, 0).Length != 38 * 256 ||
                decoder.GetPrimaryRow(0, 1).Length != 38 * 64 ||
                encoder.GetPrimaryRow(1, 2).Length != 38 * 64 ||
                decoder.GetAlphaRow(0).Length != 38 * 256 ||
                encoder.GetAlphaRow(1).Length != 38 * 256)
                return false;
            decoder.Dispose(); encoder.Dispose();
            try { new JxrSessionConfiguration(0, 16, 0, 1, 4, false); return false; }
            catch (ArgumentException) { }
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "minimal-profile\\minimal-gray-16x16.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string[] paths = {
                "minimal-profile\\minimal-gray-16x16.jxr",
                "real-image-profile\\test-sign-334x330.jxr",
                "default-profile\\city-park-605x478.jxr"
            };
            for (int index = 0; index < paths.Length; index++)
            {
                JxrHeaders headers;
                JxrSessionConfiguration fromHeader;
                if (JxrHeaders.Read(File.ReadAllBytes(Path.Combine(directory.FullName,
                    paths[index])), out headers) != JxrError.None ||
                    JxrSessionPlanner.FromHeaders(headers, out fromHeader) != JxrError.None)
                    return false;
                decoder = JxrDecoderSession.Create(fromHeader, 100, 20, 30);
                encoder = JxrEncoderSession.Create(fromHeader, 100, 30);
                if (decoder.MemoryPlan.MacroblockCount !=
                    ((long)fromHeader.Width + 15) / 16 ||
                    encoder.MemoryPlan.MacroblockCount !=
                    decoder.MemoryPlan.MacroblockCount) return false;
                decoder.Dispose(); encoder.Dispose();
            }
            return true;
        }

        private static bool TestTranscodeCoefficientReferenceVectors()
        {
            int[] formats = { 3, 2, 1 };
            ulong hash = 14695981039346656037UL;
            for (int format = 0; format < formats.Length; format++)
                for (int orientationCode = 0; orientationCode < 8; orientationCode++)
                    for (int operation = 0; operation < 2; operation++)
                    {
                        int[] source = new int[300];
                        int[] destination = new int[300];
                        for (int index = 0; index < 300; index++)
                        {
                            source[index] = index * 37 + format * 101 - 500;
                            destination[index] = -9;
                        }
                        JxrTranscodeOrientation orientation =
                            JxrTranscodeOrientation.FromCode(orientationCode);
                        JxrError error = operation == 0 ?
                            JxrTranscoder.TransformDc(formats[format], source, 7,
                                destination, 11, orientation) :
                            JxrTranscoder.TransformAc(formats[format], source, 7,
                                destination, 11, orientation);
                        int success = error == JxrError.None ? 1 : 0;
                        if (error != JxrError.None &&
                            (formats[format] != 2 || !orientation.Transpose ||
                            error != JxrError.UnsupportedFeature)) return false;
                        hash = QuantizationHashValue(hash, success);
                        for (int index = 0; index < 300; index++)
                        {
                            hash = QuantizationHashValue(hash, source[index]);
                            hash = QuantizationHashValue(hash, destination[index]);
                        }
                    }
            if (JxrTranscoder.TransformDc(3, new int[15], 0,
                new int[16], 0, JxrTranscodeOrientation.FromCode(0)) !=
                JxrError.InvalidArgument) return false;
            Console.WriteLine("Transcode coefficients signature: " + hash.ToString("X16"));
            return hash == 0x44BE42DA9EB4250DUL;
        }

        private static bool TestTranscodeRoiReferenceVectors()
        {
            int[,] rectangles = {
                { 0, 0, 5, 5 }, { 10, 10, 20, 20 },
                { 90, 70, 10, 10 }, { 31, 15, 37, 29 }
            };
            ulong hash = 14695981039346656037UL;
            for (int rectangle = 0; rectangle < 4; rectangle++)
                for (int overlap = 0; overlap <= 2; overlap++)
                    for (int ignore = 0; ignore < 2; ignore++)
                    {
                        JxrTranscodeRoi roi;
                        JxrError error = JxrTranscoder.CalculateRoi(100, 80, 3, 5,
                            1, 2, rectangles[rectangle, 0], rectangles[rectangle, 1],
                            rectangles[rectangle, 2], rectangles[rectangle, 3],
                            overlap, ignore != 0, out roi);
                        if (error != JxrError.None || roi == null) return false;
                        int[] fields = {
                            roi.ExpandedLeft, roi.ExpandedTop, roi.ExpandedWidth,
                            roi.ExpandedHeight, roi.MacroblockLeft, roi.MacroblockTop,
                            roi.MacroblockRight, roi.MacroblockBottom, roi.ExtraLeft,
                            roi.ExtraTop, roi.ExtraRight, roi.ExtraBottom,
                            roi.ImageWidth, roi.ImageHeight
                        };
                        for (int field = 0; field < fields.Length; field++)
                            hash = QuantizationHashValue(hash, fields[field]);
                    }
            JxrTranscodeRoi invalid;
            if (JxrTranscoder.CalculateRoi(100, 80, 0, 0, 0, 0,
                99, 0, 2, 1, 0, false, out invalid) !=
                JxrError.InvalidArgument || invalid != null) return false;
            Console.WriteLine("Transcode ROI signature: " + hash.ToString("X16"));
            return hash == 0x44A3954B420AA640UL;
        }

        private static bool TestTransformMathReferenceVectors()
        {
            ulong hash = 14695981039346656037UL;
            int[] values = new int[4];
            for (int first = -4; first <= 4; first++)
                for (int second = -4; second <= 4; second++)
                    for (int third = -4; third <= 4; third++)
                        for (int fourth = -4; fourth <= 4; fourth++)
                        {
                            values[0] = first; values[1] = second;
                            values[2] = third; values[3] = fourth;
                            if (JxrTransformMath.ApplyDct2x2Down(values, 0, 1, 2, 3) != JxrError.None)
                                return false;
                            for (int index = 0; index < 4; index++)
                                hash = QuantizationHashValue(hash, values[index]);
                            values[0] = first; values[1] = second;
                            values[2] = third; values[3] = fourth;
                            if (JxrTransformMath.ApplyDct2x2Up(values, 0, 1, 2, 3) != JxrError.None)
                                return false;
                            for (int index = 0; index < 4; index++)
                                hash = QuantizationHashValue(hash, values[index]);
                        }
            for (int seed = 0; seed < 32; seed++)
            {
                int[] firstStage = new int[16];
                int[] secondStage = new int[256];
                for (int index = 0; index < 16; index++)
                    firstStage[index] = ((index * 17 + seed * 11) % 51) - 25;
                for (int index = 0; index < 256; index++)
                    secondStage[index] = ((index * 7 + seed * 19) % 97) - 48;
                if (JxrTransformMath.ApplyFirstStageFourButterfly(firstStage) != JxrError.None ||
                    JxrTransformMath.ApplySecondStageFourButterfly(secondStage) != JxrError.None)
                    return false;
                for (int index = 0; index < 16; index++)
                    hash = QuantizationHashValue(hash, firstStage[index]);
                for (int index = 0; index < 256; index++)
                    hash = QuantizationHashValue(hash, secondStage[index]);
            }
            if (JxrTransformMath.ApplyDct2x2Down(values, 0, 1, 2, 2) !=
                    JxrError.InvalidArgument ||
                JxrTransformMath.ApplyFourButterfly(new int[15], new int[16]) !=
                    JxrError.InvalidArgument ||
                JxrTransformMath.ApplyDct2x2Up(null, 0, 1, 2, 3) !=
                    JxrError.InvalidArgument)
                return false;
            Console.WriteLine("Transform math signature: " + hash.ToString("X16"));
            return hash == 0xC0E5BCDC572F04F7UL;
        }

        private static bool TestForwardTransformMathReferenceVectors()
        {
            ulong hash = 14695981039346656037UL;
            int[] values = new int[4];
            for (int operation = 0; operation < 11; operation++)
                for (int first = -4; first <= 4; first++)
                    for (int second = -4; second <= 4; second++)
                        for (int third = -4; third <= 4; third++)
                            for (int fourth = -4; fourth <= 4; fourth++)
                            {
                                values[0] = first; values[1] = second;
                                values[2] = third; values[3] = fourth;
                                switch (operation)
                                {
                                    case 0: JxrForwardTransformMath.RotateHalf(ref values[0], ref values[1]); break;
                                    case 1: JxrForwardTransformMath.RotateThreeEighths(ref values[0], ref values[1]); break;
                                    case 2: JxrForwardTransformMath.ApplyDct2x2Down(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                    case 3: JxrForwardTransformMath.ApplyPre2(ref values[0], ref values[1]); break;
                                    case 4: JxrForwardTransformMath.ApplyPre2x2(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                    case 5: JxrForwardTransformMath.ApplyPre4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                    case 6: JxrForwardTransformMath.ApplyHst4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                    case 7: JxrForwardTransformMath.ApplyHst1(ref values[0], ref values[3]); break;
                                    case 8: JxrForwardTransformMath.ApplyOddOdd(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                    case 9: JxrForwardTransformMath.ApplyOddOddPre(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                    case 10: JxrForwardTransformMath.ApplyOdd(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                                }
                                for (int index = 0; index < 4; index++)
                                    hash = QuantizationHashValue(hash, values[index]);
                            }
            for (int operation = 0; operation < 11; operation++)
                for (int seed = 0; seed < 512; seed++)
                {
                    values[0] = ((seed * 137 + 1009) % 200001) - 100000;
                    values[1] = ((seed * 311 + 2701) % 200001) - 100000;
                    values[2] = ((seed * 509 + 4003) % 200001) - 100000;
                    values[3] = ((seed * 733 + 6011) % 200001) - 100000;
                    switch (operation)
                    {
                        case 0: JxrForwardTransformMath.RotateHalf(ref values[0], ref values[1]); break;
                        case 1: JxrForwardTransformMath.RotateThreeEighths(ref values[0], ref values[1]); break;
                        case 2: JxrForwardTransformMath.ApplyDct2x2Down(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                        case 3: JxrForwardTransformMath.ApplyPre2(ref values[0], ref values[1]); break;
                        case 4: JxrForwardTransformMath.ApplyPre2x2(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                        case 5: JxrForwardTransformMath.ApplyPre4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                        case 6: JxrForwardTransformMath.ApplyHst4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                        case 7: JxrForwardTransformMath.ApplyHst1(ref values[0], ref values[3]); break;
                        case 8: JxrForwardTransformMath.ApplyOddOdd(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                        case 9: JxrForwardTransformMath.ApplyOddOddPre(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                        case 10: JxrForwardTransformMath.ApplyOdd(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                    }
                    for (int index = 0; index < 4; index++)
                        hash = QuantizationHashValue(hash, values[index]);
                }
            for (int chroma = 0; chroma < 2; chroma++)
                for (int stride = 1; stride <= 16; stride *= 4)
                    for (int seed = 0; seed < 10; seed++)
                    {
                        int[] samples = new int[64];
                        for (int index = 0; index < 64; index++)
                            samples[index] = ((index * 13 + seed * 17) % 101) - 50;
                        if (JxrForwardTransformMath.NormalizeBlock(samples,
                            chroma != 0, 64, stride) != JxrError.None) return false;
                        for (int index = 0; index < 64; index++)
                            hash = QuantizationHashValue(hash, samples[index]);
                    }
            if (JxrForwardTransformMath.NormalizeBlock(null, true, 1, 1) !=
                    JxrError.InvalidArgument ||
                JxrForwardTransformMath.NormalizeBlock(new int[64], true, 64, 0) !=
                    JxrError.InvalidArgument ||
                JxrForwardTransformMath.NormalizeBlock(new int[64], true, 65, 1) !=
                    JxrError.InvalidArgument)
                return false;
            Console.WriteLine("Forward transform math signature: " + hash.ToString("X16"));
            return hash == 0xB4F5A8D8EC5F85A4UL;
        }

        private static void ApplyInverseTransformMathOperation(int operation, int[] values)
        {
            switch (operation)
            {
                case 0: JxrInverseTransformMath.RotateHalf(ref values[0], ref values[1]); break;
                case 1: JxrInverseTransformMath.RotateThreeEighths(ref values[0], ref values[1]); break;
                case 2: JxrInverseTransformMath.ApplyPost4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 3: JxrInverseTransformMath.ApplyAlternatePost4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 4: JxrInverseTransformMath.ApplyHadamardScale4(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 5: JxrInverseTransformMath.ApplyHadamardScale2(ref values[0], ref values[1]); break;
                case 6: JxrInverseTransformMath.ApplyAlternateHadamardScale2(ref values[0], ref values[1]); break;
                case 7: JxrInverseTransformMath.ApplyOddOdd(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 8: JxrInverseTransformMath.ApplyOddOddPost(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 9: JxrInverseTransformMath.ApplyOdd(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 10: JxrInverseTransformMath.ApplyScaledDct2x2Down(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 11: JxrInverseTransformMath.ApplyPost2(ref values[0], ref values[1]); break;
                case 12: JxrInverseTransformMath.ApplyAlternatePost2(ref values[0], ref values[1]); break;
                case 13: JxrInverseTransformMath.ApplyPost2x2(ref values[0], ref values[1], ref values[2], ref values[3]); break;
                case 14: JxrInverseTransformMath.ApplyAlternatePost2x2(ref values[0], ref values[1], ref values[2], ref values[3]); break;
            }
        }

        private static bool TestInverseTransformMathReferenceVectors()
        {
            ulong hash = 14695981039346656037UL;
            int[] values = new int[4];
            for (int operation = 0; operation < 15; operation++)
                for (int first = -3; first <= 3; first++)
                    for (int second = -3; second <= 3; second++)
                        for (int third = -3; third <= 3; third++)
                            for (int fourth = -3; fourth <= 3; fourth++)
                            {
                                values[0] = first; values[1] = second;
                                values[2] = third; values[3] = fourth;
                                ApplyInverseTransformMathOperation(operation, values);
                                for (int index = 0; index < 4; index++)
                                    hash = QuantizationHashValue(hash, values[index]);
                            }
            for (int operation = 0; operation < 15; operation++)
                for (int seed = 0; seed < 512; seed++)
                {
                    values[0] = ((seed * 137 + 1009) % 200001) - 100000;
                    values[1] = ((seed * 311 + 2701) % 200001) - 100000;
                    values[2] = ((seed * 509 + 4003) % 200001) - 100000;
                    values[3] = ((seed * 733 + 6011) % 200001) - 100000;
                    ApplyInverseTransformMathOperation(operation, values);
                    for (int index = 0; index < 4; index++)
                        hash = QuantizationHashValue(hash, values[index]);
                }
            for (int direct = -40; direct <= 40; direct++)
                for (int quantizer = 0; quantizer <= 48; quantizer++)
                    for (int absent = 0; absent < 2; absent++)
                    {
                        values[0] = 10 + (direct & 7);
                        values[1] = 20 - (quantizer & 7);
                        values[2] = 30 + absent;
                        values[3] = 40 - (direct & 3);
                        hash = QuantizationHashValue(hash,
                            JxrInverseTransformMath.ShouldCompensateDc(direct,
                                quantizer, absent != 0) ? 1 : 0);
                        int result = JxrInverseTransformMath.ApplyConditionalDcCompensation(
                            ref values[0], ref values[1], ref values[2], ref values[3],
                            direct, quantizer, absent != 0);
                        hash = QuantizationHashValue(hash, result);
                        for (int index = 0; index < 4; index++)
                            hash = QuantizationHashValue(hash, values[index]);
                    }
            for (int direct = -40; direct <= 40; direct++)
                for (int alternate = -40; alternate <= 40; alternate++)
                    hash = QuantizationHashValue(hash,
                        JxrInverseTransformMath.ClipDcWithAlternate(direct, alternate));
            for (int direct = -41; direct <= 41; direct++)
            {
                values[0] = 10; values[1] = 20; values[2] = 30; values[3] = 40;
                JxrInverseTransformMath.ApplyDcCompensation(ref values[0], ref values[1],
                    ref values[2], ref values[3], direct);
                for (int index = 0; index < 4; index++)
                    hash = QuantizationHashValue(hash, values[index]);
            }
            for (int absent = 0; absent < 2; absent++)
                for (int stride = 1; stride <= 16; stride *= 4)
                    for (int seed = 0; seed < 10; seed++)
                    {
                        int[] samples = new int[64];
                        for (int index = 0; index < 64; index++)
                            samples[index] = ((index * 13 + seed * 17) % 101) - 50;
                        if (JxrInverseTransformMath.NormalizeBlock(samples, absent != 0,
                            64, stride) != JxrError.None ||
                            JxrInverseTransformMath.AddCornerPredictionAt(samples, 7, seed) != JxrError.None ||
                            JxrInverseTransformMath.SubtractCornerPredictionAt(samples, 35, -seed) != JxrError.None)
                            return false;
                        for (int index = 0; index < 64; index++)
                            hash = QuantizationHashValue(hash, samples[index]);
                    }
            if (JxrInverseTransformMath.NormalizeBlock(null, true, 64, 1) !=
                    JxrError.InvalidArgument ||
                JxrInverseTransformMath.AddCornerPredictionAt(new int[4], 4, 1) !=
                    JxrError.InvalidArgument ||
                JxrInverseTransformMath.SubtractCornerPredictionAt(new int[4], -1, 1) !=
                    JxrError.InvalidArgument)
                return false;
            Console.WriteLine("Inverse transform math signature: " + hash.ToString("X16"));
            return hash == 0xED777FEF36FFE720UL;
        }

        private static bool TestCoefficientPredictionVectors()
        {
            JxrCodecColorFormat[] formats = { JxrCodecColorFormat.YOnly,
                JxrCodecColorFormat.Yuv420, JxrCodecColorFormat.Yuv422,
                JxrCodecColorFormat.Yuv444, JxrCodecColorFormat.NComponent };
            ulong hash = 14695981039346656037UL;
            for (int formatIndex = 0; formatIndex < formats.Length; formatIndex++)
                for (int scenario = 0; scenario < 6; scenario++)
                {
                    JxrCodecColorFormat format = formats[formatIndex];
                    int channels = format == JxrCodecColorFormat.YOnly ? 1 :
                        (format == JxrCodecColorFormat.NComponent ? 4 : 3);
                    int column = scenario == 0 || scenario == 2 ? 0 : 1;
                    bool leftBoundary = scenario == 0 || scenario == 2;
                    bool topBoundary = scenario == 0 || scenario == 1;
                    JxrCoefficientPredictionRows rows =
                        new JxrCoefficientPredictionRows(2, channels);
                    for (int channel = 0; channel < channels; channel++)
                        for (int position = 0; position < 2; position++)
                        {
                            JxrPredictionInfo left = rows.Current(channel, position);
                            JxrPredictionInfo top = rows.Previous(channel, position);
                            left.Dc = 15 + scenario * 11 + channel * 7 + position * 3;
                            top.Dc = 31 - scenario * 5 + channel * 9 - position * 4;
                            left.QuantizerIndex = (byte)(scenario & 1);
                            top.QuantizerIndex = (byte)((scenario == 2 || scenario == 4) ?
                                (scenario & 1) : ((scenario + 1) & 1));
                            for (int index = 0; index < 6; index++)
                            {
                                left.SetAd(index, 2 + index * 3 + channel);
                                top.SetAd(index, -4 + index * 2 - channel);
                            }
                        }
                    if (scenario == 4 || scenario == 5)
                        for (int channel = 0; channel < channels; channel++)
                        {
                            int topLeft = rows.Previous(channel, 0).Dc;
                            rows.Current(channel, 0).Dc = topLeft + (scenario == 4 ? 2 : 20);
                            rows.Previous(channel, 1).Dc = topLeft + (scenario == 4 ? 80 : 20);
                        }
                    JxrMacroblockState mb = new JxrMacroblockState(channels);
                    mb.SetLowpassQuantizerIndex((byte)(scenario & 1));
                    int[][] planes = new int[channels][];
                    int[][] originalPlanes = new int[channels][];
                    int[][] originalDc = new int[channels][];
                    for (int channel = 0; channel < channels; channel++)
                    {
                        int length = channel == 0 || (format != JxrCodecColorFormat.Yuv420 &&
                            format != JxrCodecColorFormat.Yuv422) ? 256 :
                            (format == JxrCodecColorFormat.Yuv420 ? 64 : 128);
                        planes[channel] = new int[length];
                        originalPlanes[channel] = new int[length];
                        originalDc[channel] = new int[16];
                        for (int index = 0; index < 16; index++)
                        {
                            int value = 100 + scenario * 13 + channel * 17 + index * 2;
                            if (index == 1 || index == 2 || index == 3)
                                value = scenario % 3 == 0 ? 40 : 2;
                            if (index == 4 || index == 8 || index == 12)
                                value = scenario % 3 == 1 ? 40 : 2;
                            mb.SetDcCoefficient(channel, index, value);
                            originalDc[channel][index] = value;
                        }
                        for (int index = 0; index < length; index++)
                        {
                            planes[channel][index] = ((index * 7 + scenario * 11 +
                                channel * 13) % 47) - 23;
                            originalPlanes[channel][index] = planes[channel][index];
                        }
                    }
                    JxrCoefficientColorFormat planeFormat =
                        format == JxrCodecColorFormat.Yuv420 ? JxrCoefficientColorFormat.Yuv420 :
                        (format == JxrCodecColorFormat.Yuv422 ? JxrCoefficientColorFormat.Yuv422 :
                        JxrCoefficientColorFormat.Other);
                    JxrCoefficientPlaneState planeState =
                        new JxrCoefficientPlaneState(planes, planeFormat, channels);
                    int mode = JxrCoefficientPrediction.GetDcAdMode(rows, format,
                        column, leftBoundary, topBoundary, mb.LowpassQuantizerIndex);
                    hash = QuantizationHashValue(hash, mode);
                    if (JxrCoefficientPrediction.Encode(mb, planeState, rows, format,
                        column, leftBoundary, topBoundary) != JxrError.None) return false;
                    hash = QuantizationHashValue(hash, mb.Orientation);
                    for (int channel = 0; channel < channels; channel++)
                    {
                        JxrPredictionInfo saved = rows.Current(channel, column);
                        hash = QuantizationHashValue(hash, saved.Dc);
                        hash = QuantizationHashValue(hash, saved.QuantizerIndex);
                        for (int index = 0; index < 6; index++)
                            hash = QuantizationHashValue(hash, saved.GetAd(index));
                        for (int index = 0; index < 16; index++)
                        {
                            int value;
                            mb.GetDcCoefficient(channel, index, out value);
                            hash = QuantizationHashValue(hash, value);
                        }
                        for (int index = 0; index < planes[channel].Length; index++)
                            hash = QuantizationHashValue(hash, planes[channel][index]);
                    }
                    if (JxrCoefficientPrediction.DecodeDcLp(mb, rows, format,
                        column, leftBoundary, topBoundary) != JxrError.None ||
                        JxrCoefficientPrediction.DecodeAc(mb, planeState, format) != JxrError.None)
                        return false;
                    for (int channel = 0; channel < channels; channel++)
                    {
                        for (int index = 0; index < 16; index++)
                        {
                            int value;
                            mb.GetDcCoefficient(channel, index, out value);
                            if (value != originalDc[channel][index])
                            { Console.WriteLine("Prediction DC mismatch " + format + " " + scenario + " " + channel + " " + index); return false; }
                        }
                        for (int index = 0; index < planes[channel].Length; index++)
                            if (planes[channel][index] != originalPlanes[channel][index])
                            { Console.WriteLine("Prediction AC mismatch " + format + " " + scenario + " " + channel + " " + index); return false; }
                    }
                }
            Console.WriteLine("Coefficient prediction signature: " + hash.ToString("X16"));
            return hash == 0x0392D4AF067B8926UL;
        }

        // Matches native remapQP + coefficient quantizer + dequantizer over
        // every QP index in scaled/unscaled and luma/chroma modes.
        private static bool TestQuantizationReferenceVectors()
        {
            int[] samples = { -1000000, -257, -17, -1, 0,
                1, 17, 257, 1000000 };
            ulong hash = 14695981039346656037UL;
            for (int scaled = 0; scaled < 2; scaled++)
                for (int chroma = 0; chroma < 2; chroma++)
                    for (int index = 0; index < 256; index++)
                    {
                        JxrQuantizer quantizer = JxrQuantization.Remap(
                            (byte)index, scaled != 0, chroma != 0);
                        hash = QuantizationHashValue(hash, quantizer.Parameter);
                        hash = QuantizationHashValue(hash, quantizer.Offset);
                        hash = QuantizationHashValue(hash,
                            unchecked((int)quantizer.Multiplier));
                        hash = QuantizationHashValue(hash, quantizer.Exponent);
                        for (int sample = 0; sample < samples.Length; sample++)
                        {
                            int quantized = JxrQuantization.QuantizeCoefficient(
                                samples[sample], quantizer);
                            hash = QuantizationHashValue(hash, quantized);
                            hash = QuantizationHashValue(hash,
                                JxrQuantization.DequantizeCoefficient(quantized, quantizer));
                        }
                        hash = QuantizationHashValue(hash,
                            JxrQuantization.QuantizeCoefficient(257,
                                quantizer.WithDcOffset()));
                    }
            if (hash != 0x1fa6c38918dde471UL)
            { Console.WriteLine("Managed quantization signature: " + hash.ToString("x16"));
              return false; }
            return true;
        }

        private static bool TestQuantizationMacroblockVectors()
        {
            JxrCodecColorFormat[] formats = {
                JxrCodecColorFormat.YOnly, JxrCodecColorFormat.Yuv444,
                JxrCodecColorFormat.Yuv422, JxrCodecColorFormat.Yuv420
            };
            ulong hash = 14695981039346656037UL;
            for (int formatIndex = 0; formatIndex < formats.Length; formatIndex++)
                for (int band = 0; band < 3; band++)
                    for (int transcode = 0; transcode < 2; transcode++)
                    {
                        int channelCount = formatIndex == 0 ? 1 : 3;
                        int[][] source = new int[channelCount][];
                        int[][] destination = new int[channelCount][];
                        JxrQuantizer[] dc = new JxrQuantizer[channelCount];
                        JxrQuantizer[][] lp = new JxrQuantizer[channelCount][];
                        JxrQuantizer[][] hp = new JxrQuantizer[channelCount][];
                        JxrCoefficientColorFormat planeFormat =
                            formats[formatIndex] == JxrCodecColorFormat.Yuv420 ?
                                JxrCoefficientColorFormat.Yuv420 :
                            formats[formatIndex] == JxrCodecColorFormat.Yuv422 ?
                                JxrCoefficientColorFormat.Yuv422 :
                            formats[formatIndex] == JxrCodecColorFormat.Yuv444 ?
                                JxrCoefficientColorFormat.Yuv444 :
                                JxrCoefficientColorFormat.Other;
                        for (int channel = 0; channel < channelCount; channel++)
                        {
                            int length = channel > 0 && formatIndex == 3 ? 64 :
                                channel > 0 && formatIndex == 2 ? 128 : 256;
                            source[channel] = new int[256];
                            destination[channel] = new int[256];
                            dc[channel] = JxrQuantization.Remap((byte)(6 + channel),
                                true, channel > 0).WithDcOffset();
                            lp[channel] = new JxrQuantizer[] {
                                JxrQuantization.Remap((byte)(23 + channel),
                                    true, channel > 0) };
                            hp[channel] = new JxrQuantizer[] {
                                JxrQuantization.Remap((byte)(37 + channel),
                                    true, false) };
                            for (int index = 0; index < length; index++)
                                source[channel][index] = (index % 19 - 9) * 7 + channel * 3;
                        }
                        JxrMacroblockState macroblock = new JxrMacroblockState(16);
                        JxrQuantizerSet quantizers = new JxrQuantizerSet(dc, lp, hp);
                        JxrCoefficientPlaneState sourcePlanes = new JxrCoefficientPlaneState(
                            source, planeFormat, channelCount);
                        JxrCoefficientPlaneState destinationPlanes = new JxrCoefficientPlaneState(
                            destination, planeFormat, channelCount);
                        if (JxrQuantization.QuantizeMacroblock(sourcePlanes, macroblock,
                            quantizers, formats[formatIndex], channelCount,
                            band == 2, band == 1, transcode != 0) != JxrError.None)
                            return false;
                        for (int channel = 0; channel < channelCount; channel++)
                        {
                            int length = channel > 0 && formatIndex == 3 ? 64 :
                                channel > 0 && formatIndex == 2 ? 128 : 256;
                            for (int index = 0; index < length; index++)
                                hash = QuantizationHashValue(hash, source[channel][index]);
                            for (int index = 0; index < 16; index++)
                            {
                                int coefficient;
                                if (macroblock.GetDcCoefficient(channel, index,
                                    out coefficient) != JxrError.None) return false;
                                hash = QuantizationHashValue(hash, coefficient);
                            }
                        }
                        if (JxrQuantization.DequantizeMacroblock(destinationPlanes,
                            macroblock, quantizers, formats[formatIndex], channelCount,
                            band == 2) != JxrError.None) return false;
                        for (int channel = 0; channel < channelCount; channel++)
                        {
                            int length = channel > 0 && formatIndex == 3 ? 64 :
                                channel > 0 && formatIndex == 2 ? 128 : 256;
                            for (int index = 0; index < length; index++)
                                hash = QuantizationHashValue(hash, destination[channel][index]);
                        }
                    }
            if (hash != 0xde5bd4b43499b328UL)
            { Console.WriteLine("Managed macroblock quantization signature: " +
                hash.ToString("x16")); return false; }
            int[][] passthrough = { new int[256] };
            passthrough[0][0] = 7;
            passthrough[0][128] = -3;
            JxrMacroblockState withoutQuantizers = new JxrMacroblockState(1);
            if (JxrQuantization.QuantizeMacroblock(
                new JxrCoefficientPlaneState(passthrough,
                    JxrCoefficientColorFormat.Other, 1),
                withoutQuantizers, null, JxrCodecColorFormat.YOnly,
                1, false, false, true) != JxrError.None) return false;
            int dc0, dc1;
            return withoutQuantizers.GetDcCoefficient(0, 0, out dc0) == JxrError.None &&
                withoutQuantizers.GetDcCoefficient(0, 1, out dc1) == JxrError.None &&
                dc0 == 7 && dc1 == -3;
        }

        private static bool TestQuantizationChannelModes()
        {
            ulong hash = 14695981039346656037UL;
            byte[] indices = { 8, 20, 40 };
            for (int mode = 0; mode < 4; mode++)
                for (int shifted = 0; shifted < 2; shifted++)
                {
                    JxrQuantizer[] quantizers;
                    if (JxrQuantization.RemapChannels(indices, mode, true,
                        shifted != 0, false, out quantizers) != JxrError.None)
                        return false;
                    for (int channel = 0; channel < 3; channel++)
                    {
                        hash = QuantizationHashValue(hash, quantizers[channel].Index);
                        hash = QuantizationHashValue(hash, quantizers[channel].Parameter);
                        hash = QuantizationHashValue(hash, quantizers[channel].Offset);
                        hash = QuantizationHashValue(hash,
                            unchecked((int)quantizers[channel].Multiplier));
                        hash = QuantizationHashValue(hash, quantizers[channel].Exponent);
                    }
                }
            if (hash != 0x053a7d101da6569bUL)
            { Console.WriteLine("Managed channel quantization signature: " +
                hash.ToString("x16")); return false; }
            return true;
        }

        // First YUV444 macroblock of the native real-image trace.  Native
        // ranges: DC [1384,1424), LP [1424,1607), HP [1607,1620).
        private static bool TestColorEntropyCodecFixture()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "real-image-profile\\test-sign-334x330.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            JxrBitReader reader = new JxrBitReader(File.ReadAllBytes(Path.Combine(
                directory.FullName, "real-image-profile\\test-sign-334x330.jxr")));
            int remaining = 1384;
            while (remaining > 0)
            {
                int count = Math.Min(remaining, 32);
                if (reader.ConsumeBits(count) != JxrError.None) return false;
                remaining -= count;
            }
            JxrCodecConfiguration format = new JxrCodecConfiguration(
                JxrCodecColorFormat.Yuv444, 3, true, false, true, true,
                false, true, true, false, false, 0, 0, 1, 1,
                new int[][] { new int[] { 1 }, new int[] { 1 }, new int[] { 1 } });
            JxrCodecState state = new JxrCodecState(format, reader, reader, reader, reader);
            if (JxrDcCodec.Decode(state) != JxrError.None || reader.BitPosition != 1424)
            { Console.WriteLine("Color DC bit position: " + reader.BitPosition); return false; }
            if (JxrLpCodec.Decode(state) != JxrError.None || reader.BitPosition != 1607)
            { Console.WriteLine("Color LP bit position: " + reader.BitPosition); return false; }
            if (JxrHpCodec.Decode(state) != JxrError.None || reader.BitPosition != 1620)
            { Console.WriteLine("Color HP bit position: " + reader.BitPosition); return false; }
            int cbp, difference, luminanceDc;
            if (state.MacroblockCbp.GetCbp(0, out cbp) != JxrError.None ||
                state.MacroblockCbp.GetDifferential(0, out difference) != JxrError.None ||
                state.Macroblock.GetDcCoefficient(0, 0, out luminanceDc) != JxrError.None)
                return false;
            return cbp == 0 && difference == 1 && luminanceDc == 176;
        }

        private static bool TestRealRgb444Decode()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "real-image-profile\\test-sign-334x330.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixture = Path.Combine(directory.FullName, "real-image-profile");
            JxrImage source;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixture,
                "test-sign-334x330.bmp")), out source) != JxrError.None ||
                source.Width != 334 || source.Height != 330) return false;
            JxrImage decoded;
            JxrDecoderOptions options = new JxrDecoderOptions();
            options.OutputFormat = JxrPixelFormat.Rgb24;
            JxrError error = JxrCodec.Decode(File.ReadAllBytes(Path.Combine(fixture,
                "test-sign-334x330.jxr")), options, out decoded);
            if (error != JxrError.None || decoded == null ||
                decoded.Width != source.Width || decoded.Height != source.Height ||
                decoded.Format != JxrPixelFormat.Rgb24 ||
                !EqualBytes(decoded.Pixels, source.Pixels))
            {
                Console.WriteLine("RGB444 decode: " + error);
                if (decoded != null)
                    for (int index = 0; index < Math.Min(decoded.Pixels.Length,
                        source.Pixels.Length); index++)
                        if (decoded.Pixels[index] != source.Pixels[index])
                        { Console.WriteLine("First RGB mismatch at byte " + index +
                            ": " + decoded.Pixels[index] + " != " + source.Pixels[index]); break; }
                return false;
            }
            options.OutputFormat = JxrPixelFormat.Bgr24;
            error = JxrCodec.Decode(File.ReadAllBytes(Path.Combine(fixture,
                "test-sign-334x330.jxr")), options, out decoded);
            if (error != JxrError.None || decoded == null ||
                decoded.Format != JxrPixelFormat.Bgr24 ||
                decoded.Pixels.Length != source.Pixels.Length) return false;
            for (int index = 0; index < source.Pixels.Length; index += 3)
                if (decoded.Pixels[index] != source.Pixels[index + 2] ||
                    decoded.Pixels[index + 1] != source.Pixels[index + 1] ||
                    decoded.Pixels[index + 2] != source.Pixels[index]) return false;
            return true;
        }

        private static bool TestRgb444HeaderWriterFixture()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "real-image-profile\\test-sign-334x330.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string root = directory.FullName;
            string[] files = {
                "real-image-profile\\test-sign-334x330.jxr",
                "managed\\fixtures\\rgb444-q16-all.jxr",
                "managed\\fixtures\\rgb444-q16-no-flex.jxr",
                "managed\\fixtures\\rgb444-q16-no-hp.jxr",
                "managed\\fixtures\\rgb444-q16-dc-only.jxr",
                "managed\\fixtures\\rgb444-q16-trim3.jxr" };
            for (int item = 0; item < files.Length; item++)
            {
                byte[] reference = File.ReadAllBytes(Path.Combine(root, files[item]));
                JxrHeaders headers;
                if (JxrHeaders.Read(reference, out headers) != JxrError.None) return false;
                JxrBitWriter writer = new JxrBitWriter();
                byte dc = headers.Quantizers.GetDcIndex(0);
                byte lp = headers.Quantizers.GetLowpassIndex(0);
                byte hp = headers.Quantizers.GetHighpassIndex(0);
                int trim = files[item].IndexOf("trim3") >= 0 ? 3 : 0;
                if (JxrHeaderWriter.WriteRgbSpatial(writer, (int)headers.Main.Width,
                    (int)headers.Main.Height, dc, lp, hp,
                    (JxrGraySubbandMode)headers.Plane.Subband,
                    headers.Plane.ScaledArithmetic, trim) != JxrError.None)
                    return false;
                byte[] actual = writer.ToArray();
                if (actual.Length != headers.ByteCount) return false;
                for (int index = 0; index < actual.Length; index++)
                    if (actual[index] != reference[headers.CodestreamOffset + index])
                    {
                        Console.WriteLine("Color header " + files[item] +
                            " differs at byte " + index + ": " + actual[index] +
                            "/" + reference[headers.CodestreamOffset + index]);
                        return false;
                    }
            }
            return true;
        }

        private static bool TestRgb444EncoderNativeFixture()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "real-image-profile\\test-sign-334x330.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string root = directory.FullName;
            JxrImage image;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(root,
                "real-image-profile\\test-sign-334x330.bmp")), out image) != JxrError.None)
                return false;
            byte[] actual;
            JxrEncoderOptions options = new JxrEncoderOptions();
            JxrError error = JxrCodec.Encode(image, options, out actual);
            if (error != JxrError.None || actual == null)
            {
                Console.WriteLine("RGB444 encode: " + error);
                return false;
            }
            byte[] expected = File.ReadAllBytes(Path.Combine(root,
                "real-image-profile\\test-sign-334x330.jxr"));
            if (!EqualBytes(actual, expected))
            {
                Console.WriteLine("RGB444 JXR sizes " + actual.Length + "/" + expected.Length);
                for (int index = 0; index < Math.Min(actual.Length, expected.Length); index++)
                    if (actual[index] != expected[index])
                    {
                        Console.WriteLine("First RGB444 JXR mismatch at " + index +
                            ": " + actual[index] + "/" + expected[index]);
                        break;
                    }
                return false;
            }
            JxrDecoderOptions decodeOptions = new JxrDecoderOptions();
            decodeOptions.OutputFormat = JxrPixelFormat.Rgb24;
            JxrImage decoded;
            error = JxrCodec.Decode(actual, decodeOptions, out decoded);
            return error == JxrError.None && decoded != null &&
                EqualBytes(decoded.Pixels, image.Pixels);
        }

        private static bool TestRgb444EncoderQualityNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb444-q16-all.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string root = directory.FullName;
            string fixtures = Path.Combine(root, "managed\\fixtures");
            JxrImage sign, city;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(root,
                "real-image-profile\\test-sign-334x330.bmp")), out sign) != JxrError.None ||
                JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(root,
                "default-profile\\city-park-605x478.bmp")), out city) != JxrError.None)
                return false;
            string[] names = { "rgb444-q16-all", "rgb444-q16-no-flex",
                "rgb444-q16-no-hp", "rgb444-q16-dc-only",
                "rgb444-q16-trim3", "rgb444-city-q16-all" };
            JxrGraySubbandMode[] subbands = { JxrGraySubbandMode.All,
                JxrGraySubbandMode.NoFlexbits, JxrGraySubbandMode.NoHighpass,
                JxrGraySubbandMode.DcOnly, JxrGraySubbandMode.All,
                JxrGraySubbandMode.All };
            for (int item = 0; item < names.Length; item++)
            {
                JxrEncoderOptions options = new JxrEncoderOptions();
                options.QualityIndex = 16;
                options.Subbands = subbands[item];
                options.TrimFlexbits = item == 4 ? 3 : 0;
                byte[] actual;
                JxrError error = JxrCodec.Encode(item == 5 ? city : sign,
                    options, out actual);
                if (error != JxrError.None || actual == null)
                {
                    Console.WriteLine("RGB444 quality encode " + names[item] +
                        ": " + error);
                    return false;
                }
                byte[] expected = File.ReadAllBytes(Path.Combine(fixtures,
                    names[item] + ".jxr"));
                if (!EqualBytes(actual, expected))
                {
                    Console.WriteLine("RGB444 quality JXR sizes " + names[item] +
                        " " + actual.Length + "/" + expected.Length);
                    for (int index = 0; index < Math.Min(actual.Length, expected.Length); index++)
                        if (actual[index] != expected[index])
                        {
                            Console.WriteLine("First mismatch at " + index +
                                ": " + actual[index] + "/" + expected[index]);
                            break;
                        }
                    return false;
                }
            }
            JxrImage blank = new JxrImage(32, 32, JxrPixelFormat.Gray8,
                new byte[32 * 32], 32);
            for (int pixel = 0; pixel < blank.Pixels.Length; pixel++)
                blank.Pixels[pixel] = 128;
            JxrEncoderOptions blankOptions = new JxrEncoderOptions();
            blankOptions.TileLayout = new JxrTileLayout(new int[] { 1, 1 },
                new int[] { 1, 1 });
            byte[] blankJxr;
            if (JxrCodec.Encode(blank, blankOptions, out blankJxr) != JxrError.None)
                return false;
            JxrDecoderOptions blankDecodeOptions = new JxrDecoderOptions();
            blankDecodeOptions.OutputFormat = JxrPixelFormat.Gray8;
            JxrImage blankDecoded;
            JxrError blankDecodeError = JxrCodec.Decode(blankJxr,
                blankDecodeOptions, out blankDecoded);
            if (blankDecodeError != JxrError.None || blankDecoded == null)
            {
                Console.WriteLine("empty tile packets decode: " + blankDecodeError);
                return false;
            }
            for (int pixel = 0; pixel < blankDecoded.Pixels.Length; pixel++)
                if (blankDecoded.Pixels[pixel] != 128)
                {
                    Console.WriteLine("empty tile output differs at " + pixel);
                    return false;
                }
            return true;
        }

        private static bool TestOverlapForwardNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\gray-31x19-ol1.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            JxrImage gray, rgb;
            if (JxrBmpAdapter.ReadGray8(File.ReadAllBytes(Path.Combine(fixtures,
                "gray-31x19.bmp")), out gray) != JxrError.None ||
                JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtures,
                "rgb-overlap-32x32.bmp")), out rgb) != JxrError.None)
                return false;
            string[] names = { "gray-31x19-ol1", "gray-31x19-ol2",
                "rgb444-32x32-ol1", "rgb444-32x32-ol2" };
            for (int item = 0; item < names.Length; item++)
            {
                JxrEncoderOptions options = new JxrEncoderOptions();
                options.Overlap = item % 2 + 1;
                if (item >= 2) options.QualityIndex = 16;
                byte[] encoded;
                JxrError error = JxrCodec.Encode(item < 2 ? gray : rgb,
                    options, out encoded);
                byte[] expected = File.ReadAllBytes(Path.Combine(fixtures,
                    names[item] + ".jxr"));
                if (error != JxrError.None || !EqualBytes(encoded, expected))
                {
                    Console.WriteLine(names[item] + " overlap encode: " + error);
                    if (encoded != null)
                    {
                        Console.WriteLine("length " + encoded.Length + " vs " + expected.Length);
                        for (int index = 0; index < Math.Min(encoded.Length,
                            expected.Length); index++)
                            if (encoded[index] != expected[index])
                            { Console.WriteLine("first difference at " + index);
                              break; }
                    }
                    return false;
                }
                JxrImage restored;
                if (item < 2)
                {
                    if (JxrBmpAdapter.ReadGray8(File.ReadAllBytes(
                        Path.Combine(fixtures, names[item] + "-restored.bmp")),
                        out restored) != JxrError.None) return false;
                }
                else if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(
                    Path.Combine(fixtures, names[item] + "-restored.bmp")),
                    out restored) != JxrError.None) return false;
                JxrDecoderOptions decodeOptions = new JxrDecoderOptions();
                if (item >= 2) decodeOptions.OutputFormat = JxrPixelFormat.Rgb24;
                JxrImage decoded;
                error = JxrCodec.Decode(expected, decodeOptions, out decoded);
                if (error != JxrError.None || decoded == null ||
                    !EqualBytes(decoded.Pixels, restored.Pixels))
                {
                    Console.WriteLine(names[item] + " overlap decode: " + error);
                    if (decoded != null)
                        for (int index = 0; index < Math.Min(decoded.Pixels.Length,
                            restored.Pixels.Length); index++)
                            if (decoded.Pixels[index] != restored.Pixels[index])
                            { Console.WriteLine("first pixel difference at " + index +
                                ": " + decoded.Pixels[index] + " vs " + restored.Pixels[index]);
                              break; }
                    return false;
                }
            }
            return true;
        }

        private static bool TestSubsampledEncodeNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb422-32x32-ol0.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            JxrImage image, odd;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtures,
                "rgb-overlap-32x32.bmp")), out image) != JxrError.None)
                return false;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtures,
                "rgb-overlap-31x19.bmp")), out odd) != JxrError.None)
                return false;
            string[] names = { "rgb422-32x32-ol0", "rgb422-32x32-ol1",
                "rgb422-32x32-ol2", "rgb420-32x32-ol0",
                "rgb420-32x32-ol1", "rgb420-32x32-ol2",
                "rgb422-32x32-q1-ol0", "rgb422-32x32-q1-ol1",
                "rgb422-32x32-q1-ol2", "rgb420-32x32-q1-ol0",
                "rgb420-32x32-q1-ol1", "rgb420-32x32-q1-ol2",
                "rgb422-31x19-q1-ol1", "rgb422-31x19-q1-ol2",
                "rgb420-31x19-q1-ol1", "rgb420-31x19-q1-ol2" };
            for (int item = 0; item < names.Length; item++)
            {
                JxrEncoderOptions options = new JxrEncoderOptions();
                options.QualityIndex = names[item].IndexOf("-q1-") >= 0 ? 1 : 16;
                options.ChromaSubsampling = names[item].StartsWith("rgb422") ?
                    JxrChromaSubsampling.Yuv422 : JxrChromaSubsampling.Yuv420;
                options.Overlap = names[item].EndsWith("ol0") ? 0 :
                    names[item].EndsWith("ol1") ? 1 : 2;
                byte[] actual;
                JxrError error = JxrCodec.Encode(
                    names[item].IndexOf("31x19") >= 0 ? odd : image,
                    options, out actual);
                byte[] expected = File.ReadAllBytes(Path.Combine(fixtures,
                    names[item] + ".jxr"));
                if (error != JxrError.None || !EqualBytes(actual, expected))
                {
                    Console.WriteLine(names[item] + " encode: " + error);
                    if (actual != null)
                    {
                        Console.WriteLine("length " + actual.Length + " vs " + expected.Length);
                        for (int index = 0; index < Math.Min(actual.Length,
                            expected.Length); index++)
                            if (actual[index] != expected[index])
                            { Console.WriteLine("first difference at " + index +
                                ": " + actual[index] + " vs " + expected[index]);
                              break; }
                    }
                    return false;
                }
            }
            return true;
        }

        private static bool TestSubsampledEdgeNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb422-31x19-ol2.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            string[] dimensions = { "1x1", "31x19" };
            JxrDecoderOptions decodeOptions = new JxrDecoderOptions();
            decodeOptions.OutputFormat = JxrPixelFormat.Rgb24;
            foreach (string dimensionsTag in dimensions)
            {
                JxrImage source;
                if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtures,
                    "rgb-overlap-" + dimensionsTag + ".bmp")), out source) !=
                    JxrError.None)
                { Console.WriteLine(dimensionsTag + " source BMP read"); return false; }
                foreach (int format in new int[] { 2, 1 })
                    foreach (int overlap in new int[] { 1, 2 })
                    {
                        if (dimensionsTag == "1x1" && overlap == 2) continue;
                        string name = (format == 2 ? "rgb422" : "rgb420") +
                            "-" + dimensionsTag +
                            "-ol" + overlap;
                        byte[] reference = File.ReadAllBytes(Path.Combine(fixtures,
                            name + ".jxr"));
                        JxrEncoderOptions options = new JxrEncoderOptions();
                        options.QualityIndex = 16;
                        options.ChromaSubsampling = format == 2 ?
                            JxrChromaSubsampling.Yuv422 : JxrChromaSubsampling.Yuv420;
                        options.Overlap = overlap;
                        byte[] encoded;
                        JxrError error = JxrCodec.Encode(source, options, out encoded);
                        if (error != JxrError.None || !EqualBytes(encoded, reference))
                        {
                            Console.WriteLine(name + " encode: " + error);
                            if (encoded != null)
                            {
                                Console.WriteLine("length " + encoded.Length +
                                    " vs " + reference.Length);
                                for (int index = 0; index < Math.Min(encoded.Length,
                                    reference.Length); index++)
                                    if (encoded[index] != reference[index])
                                    { Console.WriteLine("first difference " + index);
                                      break; }
                            }
                            return false;
                        }
                        JxrImage restored, decoded;
                        if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(
                            Path.Combine(fixtures, name + "-restored.bmp")),
                            out restored) != JxrError.None)
                        { Console.WriteLine(name + " restored BMP read"); return false; }
                        error = JxrCodec.Decode(reference, decodeOptions, out decoded);
                        if (error != JxrError.None || decoded == null ||
                            !EqualBytes(decoded.Pixels, restored.Pixels))
                        {
                            Console.WriteLine(name + " decode: " + error);
                            if (decoded != null)
                                for (int index = 0; index < decoded.Pixels.Length; index++)
                                    if (decoded.Pixels[index] != restored.Pixels[index])
                                    { Console.WriteLine("first pixel difference " + index);
                                      break; }
                            return false;
                        }
                    }
                if (dimensionsTag == "1x1")
                {
                    foreach (JxrChromaSubsampling rejectedFormat in new
                        JxrChromaSubsampling[] { JxrChromaSubsampling.Yuv422,
                            JxrChromaSubsampling.Yuv420 })
                    {
                        JxrEncoderOptions unsupported = new JxrEncoderOptions();
                        unsupported.ChromaSubsampling = rejectedFormat;
                        unsupported.Overlap = 2;
                        byte[] rejected;
                        if (JxrCodec.Encode(source, unsupported, out rejected) !=
                            JxrError.UnsupportedFeature || rejected != null)
                            return false;
                    }
                }
            }
            return true;
        }

        private static bool TestRealSubsampledNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\sign422-q16-ol1.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            JxrImage source;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(
                directory.FullName, "real-image-profile\\test-sign-334x330.bmp")),
                out source) != JxrError.None) return false;
            string[] names = { "sign422-q16-ol1", "sign420-q16-ol2" };
            for (int item = 0; item < names.Length; item++)
            {
                JxrEncoderOptions encodeOptions = new JxrEncoderOptions();
                encodeOptions.QualityIndex = 16;
                encodeOptions.ChromaSubsampling = item == 0 ?
                    JxrChromaSubsampling.Yuv422 : JxrChromaSubsampling.Yuv420;
                encodeOptions.Overlap = item + 1;
                byte[] actual;
                JxrError error = JxrCodec.Encode(source, encodeOptions, out actual);
                byte[] reference = File.ReadAllBytes(Path.Combine(fixtures,
                    names[item] + ".jxr"));
                if (error != JxrError.None || !EqualBytes(actual, reference))
                {
                    Console.WriteLine(names[item] + " real encode: " + error);
                    if (actual != null)
                    {
                        Console.WriteLine("length " + actual.Length +
                            " vs " + reference.Length);
                        for (int index = 0; index < Math.Min(actual.Length,
                            reference.Length); index++)
                            if (actual[index] != reference[index])
                            { Console.WriteLine("first difference " + index); break; }
                    }
                    return false;
                }
                JxrImage expected, decoded;
                if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtures,
                    names[item] + "-restored.bmp")), out expected) != JxrError.None)
                    return false;
                JxrDecoderOptions decodeOptions = new JxrDecoderOptions();
                decodeOptions.OutputFormat = JxrPixelFormat.Rgb24;
                error = JxrCodec.Decode(reference, decodeOptions, out decoded);
                if (error != JxrError.None || decoded == null ||
                    !EqualBytes(decoded.Pixels, expected.Pixels))
                {
                    Console.WriteLine(names[item] + " real decode: " + error);
                    if (decoded != null)
                        for (int index = 0; index < decoded.Pixels.Length; index++)
                            if (decoded.Pixels[index] != expected.Pixels[index])
                            { Console.WriteLine("first pixel difference " + index);
                              break; }
                    return false;
                }
            }
            return true;
        }

        private static bool TestSpatialTileNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\gray32-tiles2x2-q1-ol0.jxr")))
                directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            string[] names = { "gray32-tiles2x2-q1-ol0",
                "gray32-tiles2x2-q16-ol2", "rgb444-tiles2x2-q16-ol0",
                "rgb444-tiles2x2-q16-ol2", "rgb422-tiles2x2-q16-ol0",
                "rgb422-tiles2x2-q16-ol2", "rgb420-tiles2x2-q16-ol0",
                "rgb420-tiles2x2-q16-ol2", "rgb422-tiles-v1-2-q16-ol2",
                "rgb420-tiles31x19-q1-ol2" };
            for (int item = 0; item < names.Length; item++)
            {
                string name = names[item];
                byte[] jxr = File.ReadAllBytes(Path.Combine(fixtures, name + ".jxr"));
                JxrHeaders headers;
                if (JxrHeaders.Read(jxr, out headers) != JxrError.None ||
                    !headers.Main.HasIndexTable || headers.Main.BitstreamFormat != 0)
                {
                    Console.WriteLine(name + " header/index table");
                    return false;
                }
                if (name == "rgb422-tiles-v1-2-q16-ol2" &&
                    (headers.Main.VerticalSliceCountMinusOne != 1 ||
                     headers.Main.GetTileX(1) != 1))
                {
                    Console.WriteLine(name + " variable tile boundaries");
                    return false;
                }
                JxrPixelFormat format = name.StartsWith("gray") ?
                    JxrPixelFormat.Gray8 : JxrPixelFormat.Rgb24;
                JxrImage expected, actual;
                byte[] restored = File.ReadAllBytes(Path.Combine(fixtures,
                    name + "-restored.bmp"));
                JxrError error = format == JxrPixelFormat.Gray8 ?
                    JxrBmpAdapter.ReadGray8(restored, out expected) :
                    JxrBmpAdapter.ReadRgb24(restored, out expected);
                if (error != JxrError.None) return false;
                JxrDecoderOptions options = new JxrDecoderOptions();
                options.OutputFormat = format;
                error = JxrCodec.Decode(jxr, options, out actual);
                if (error != JxrError.None || actual == null ||
                    actual.Width != expected.Width || actual.Height != expected.Height ||
                    !EqualBytes(actual.Pixels, expected.Pixels))
                {
                    Console.WriteLine(name + " decode: " + error);
                    if (actual != null)
                        for (int index = 0; index < Math.Min(actual.Pixels.Length,
                            expected.Pixels.Length); index++)
                            if (actual.Pixels[index] != expected.Pixels[index])
                            { Console.WriteLine("first pixel difference " + index); break; }
                    return false;
                }
            }
            return true;
        }

        private static bool TestSpatialTileEncodeNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\gray32-tiles2x2-q1-ol0.jxr")))
                directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            string[] names = { "gray32-tiles2x2-q1-ol0",
                "gray32-tiles2x2-q16-ol2", "rgb444-tiles2x2-q16-ol0",
                "rgb444-tiles2x2-q16-ol2", "rgb422-tiles2x2-q16-ol0",
                "rgb422-tiles2x2-q16-ol2", "rgb420-tiles2x2-q16-ol0",
                "rgb420-tiles2x2-q16-ol2", "rgb422-tiles-v1-2-q16-ol2",
                "rgb420-tiles31x19-q1-ol2" };
            for (int item = 0; item < names.Length; item++)
            {
                string name = names[item];
                bool gray = name.StartsWith("gray");
                string sourceName = gray ? "gray-32x32.bmp" :
                    name.StartsWith("rgb422-tiles-v1") ? "rgb-48x32-tiles.bmp" :
                    name.StartsWith("rgb420-tiles31x19") ? "rgb-overlap-31x19.bmp" :
                    "rgb-overlap-32x32.bmp";
                JxrImage image;
                byte[] source = File.ReadAllBytes(Path.Combine(fixtures, sourceName));
                JxrError error = gray ? JxrBmpAdapter.ReadGray8(source, out image) :
                    JxrBmpAdapter.ReadRgb24(source, out image);
                if (error != JxrError.None) return false;

                JxrEncoderOptions options = new JxrEncoderOptions();
                options.QualityIndex = name.Contains("q1-") ? 1 : 16;
                options.Overlap = name.Contains("ol0") ? 0 : 2;
                options.ChromaSubsampling = gray || name.StartsWith("rgb444") ?
                    JxrChromaSubsampling.Yuv444 :
                    name.StartsWith("rgb422") ? JxrChromaSubsampling.Yuv422 :
                    JxrChromaSubsampling.Yuv420;
                options.TileLayout = name.StartsWith("rgb422-tiles-v1") ?
                    new JxrTileLayout(new int[] { 1, 2 }, new int[] { 2 }) :
                    new JxrTileLayout(new int[] { 1, 1 }, new int[] { 1, 1 });
                byte[] actual;
                error = JxrCodec.Encode(image, options, out actual);
                byte[] expected = File.ReadAllBytes(Path.Combine(fixtures, name + ".jxr"));
                if (error != JxrError.None || !EqualBytes(actual, expected))
                {
                    Console.WriteLine(name + " encode parity: " + error);
                    if (actual != null)
                    {
                        Console.WriteLine("length " + actual.Length + " vs " + expected.Length);
                        for (int index = 0; index < Math.Min(actual.Length,
                            expected.Length); index++)
                            if (actual[index] != expected[index])
                            { Console.WriteLine("first difference " + index); break; }
                    }
                    return false;
                }
            }
            return true;
        }

        private static bool TestFrequencyLayoutNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\frequency-gray-16x16.jxr")))
                directory = directory.Parent;
            if (directory == null) return false;
            string fixtureDir = Path.Combine(directory.FullName, "managed\\fixtures");
            string[] names = { "frequency-gray-16x16", "frequency-gray32-tile2x2",
                "frequency-gray32-tile2x2-sequential", "frequency-rgb444-tile2x2",
                "frequency-rgb422-tile2x2", "frequency-rgb420-tile2x2",
                "frequency-gray32-q16", "frequency-gray32-q16-trim2",
                "frequency-gray32-q16-noflex", "frequency-gray32-q16-nohp",
                "frequency-gray32-q16-dconly" };
            string[] sourceNames = { null, "gray-32x32.bmp", "gray-32x32.bmp",
                "rgb-overlap-32x32.bmp", "rgb-overlap-32x32.bmp",
                "rgb-overlap-32x32.bmp", "gray-32x32.bmp", "gray-32x32.bmp",
                "gray-32x32.bmp", "gray-32x32.bmp", "gray-32x32.bmp" };
            JxrChromaSubsampling[] chroma = { JxrChromaSubsampling.Yuv444,
                JxrChromaSubsampling.Yuv444, JxrChromaSubsampling.Yuv444,
                JxrChromaSubsampling.Yuv444, JxrChromaSubsampling.Yuv422,
                JxrChromaSubsampling.Yuv420 };
            for (int item = 0; item < names.Length; item++)
            {
                bool gray = item < 3 || item >= 6;
                JxrImage input;
                byte[] sourceBmp = File.ReadAllBytes(item == 0 ?
                    Path.Combine(directory.FullName,
                        "minimal-profile\\minimal-gray-16x16.bmp") :
                    Path.Combine(fixtureDir, sourceNames[item]));
                JxrError error = gray ?
                    JxrBmpAdapter.ReadGray8(sourceBmp, out input) :
                    JxrBmpAdapter.ReadRgb24(sourceBmp, out input);
                if (error != JxrError.None) return false;
                JxrEncoderOptions options = new JxrEncoderOptions();
                options.Layout = JxrBitstreamLayout.Frequency;
                options.ChromaSubsampling = chroma[item < 6 ? item : 0];
                if (item >= 6) options.QualityIndex = 16;
                if (item == 7) options.TrimFlexbits = 2;
                if (item == 8) options.Subbands = JxrGraySubbandMode.NoFlexbits;
                if (item == 9) options.Subbands = JxrGraySubbandMode.NoHighpass;
                if (item == 10) options.Subbands = JxrGraySubbandMode.DcOnly;
                if (item >= 1 && item <= 5)
                    options.TileLayout = new JxrTileLayout(
                        new int[] { 1, 1 }, new int[] { 1, 1 });
                if (item == 2) options.Progressive = false;
                byte[] actual;
                error = JxrCodec.Encode(input, options, out actual);
                byte[] expected = File.ReadAllBytes(Path.Combine(fixtureDir,
                    names[item] + ".jxr"));
                if (error != JxrError.None || !EqualBytes(actual, expected))
                {
                    Console.WriteLine(names[item] + " encode parity: " + error);
                    return false;
                }
                JxrDecoderOptions decodeOptions = new JxrDecoderOptions();
                decodeOptions.OutputFormat = gray ? JxrPixelFormat.Gray8 :
                    JxrPixelFormat.Rgb24;
                JxrImage actualImage;
                error = JxrCodec.Decode(expected, decodeOptions, out actualImage);
                if (error != JxrError.None) return false;
                byte[] restoredBmp = File.ReadAllBytes(Path.Combine(fixtureDir,
                    names[item] + "-restored.bmp"));
                JxrImage expectedImage;
                error = gray ? JxrBmpAdapter.ReadGray8(restoredBmp,
                    out expectedImage) : JxrBmpAdapter.ReadRgb24(restoredBmp,
                    out expectedImage);
                if (error != JxrError.None ||
                    !EqualBytes(actualImage.Pixels, expectedImage.Pixels))
                {
                    Console.WriteLine(names[item] + " decode parity: " + error);
                    return false;
                }
            }
            return true;
        }

        private static bool TestSpatialTileLayoutValidation()
        {
            JxrImage image = new JxrImage(32, 32, JxrPixelFormat.Gray8,
                new byte[32 * 32], 32);
            JxrEncoderOptions options = new JxrEncoderOptions();
            byte[] encoded;
            options.TileLayout = new JxrTileLayout(new int[] { 1 },
                new int[] { 1, 1 });
            if (JxrCodec.Encode(image, options, out encoded) !=
                JxrError.InvalidArgument || encoded != null) return false;
            options.TileLayout = new JxrTileLayout(new int[] { 0, 2 },
                new int[] { 1, 1 });
            if (JxrCodec.Encode(image, options, out encoded) !=
                JxrError.InvalidArgument || encoded != null) return false;
            options.TileLayout = new JxrTileLayout(new int[] { 1, 1 },
                new int[] { 1, 1 });
            return JxrCodec.Encode(image, options, out encoded) == JxrError.None &&
                encoded != null;
        }

        private static bool TestSpatialTileInvalidTables()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb444-tiles2x2-q16-ol0.jxr")))
                directory = directory.Parent;
            if (directory == null) return false;
            byte[] reference = File.ReadAllBytes(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb444-tiles2x2-q16-ol0.jxr"));
            JxrHeaders headers;
            if (JxrHeaders.Read(reference, out headers) != JxrError.None) return false;
            int table = headers.CodestreamOffset + headers.ByteCount;
            JxrDecoderOptions options = new JxrDecoderOptions();
            options.OutputFormat = JxrPixelFormat.Rgb24;
            byte[] invalid = (byte[])reference.Clone();
            invalid[table + 1] = 0;
            if (!ExpectInvalidSpatialStream(invalid, options, "bad table marker")) return false;

            invalid = (byte[])reference.Clone();
            // The first packet offset follows the two-byte table marker.
            invalid[table + 2] = 0x7f;
            invalid[table + 3] = 0xff;
            if (!ExpectInvalidSpatialStream(invalid, options, "bad packet offset")) return false;

            // The fixture uses four 16-bit offsets followed by an escaped
            // trailing length word, then the first packet.
            if (reference[table + 10] < 0xfd)
            {
                Console.WriteLine("unexpected table trailer: table=" + table +
                    " trailer=" + reference[table + 10]);
                return false;
            }
            invalid = (byte[])reference.Clone();
            invalid[table + 11 + 3] = 0x08;
            if (!ExpectInvalidSpatialStream(invalid, options, "bad packet tile id")) return false;
            return true;
        }

        private static bool ExpectInvalidSpatialStream(byte[] jxr,
            JxrDecoderOptions options, string label)
        {
            JxrImage image;
            JxrError error = JxrCodec.Decode(jxr, options, out image);
            if (error != JxrError.InvalidBitstream || image != null)
            {
                Console.WriteLine(label + ": " + error);
                return false;
            }
            return true;
        }

        private static bool TestSubsampledDecodeNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb422-32x32-ol0.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtures = Path.Combine(directory.FullName, "managed\\fixtures");
            string[] names = { "rgb422-32x32-ol0", "rgb422-32x32-ol1",
                "rgb422-32x32-ol2", "rgb420-32x32-ol0",
                "rgb420-32x32-ol1", "rgb420-32x32-ol2",
                "rgb422-32x32-q1-ol0", "rgb422-32x32-q1-ol1",
                "rgb422-32x32-q1-ol2", "rgb420-32x32-q1-ol0",
                "rgb420-32x32-q1-ol1", "rgb420-32x32-q1-ol2",
                "rgb422-31x19-q1-ol1", "rgb422-31x19-q1-ol2",
                "rgb420-31x19-q1-ol1", "rgb420-31x19-q1-ol2" };
            JxrDecoderOptions options = new JxrDecoderOptions();
            options.OutputFormat = JxrPixelFormat.Rgb24;
            for (int item = 0; item < names.Length; item++)
            {
                JxrImage expected, actual;
                if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtures,
                    names[item] + "-restored.bmp")), out expected) != JxrError.None)
                    return false;
                JxrError error = JxrCodec.Decode(File.ReadAllBytes(
                    Path.Combine(fixtures, names[item] + ".jxr")), options,
                    out actual);
                if (error != JxrError.None || actual == null ||
                    !EqualBytes(actual.Pixels, expected.Pixels))
                {
                    Console.WriteLine(names[item] + " decode: " + error);
                    if (actual != null)
                        for (int index = 0; index < actual.Pixels.Length; index++)
                            if (actual.Pixels[index] != expected.Pixels[index])
                            { Console.WriteLine("first pixel difference " + index +
                                ": " + actual.Pixels[index] + " vs " + expected.Pixels[index]);
                              break; }
                    return false;
                }
            }
            return true;
        }

        private static bool TestBgr444EncoderRoundTrip()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "real-image-profile\\test-sign-334x330.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string root = directory.FullName;
            JxrImage rgb;
            if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(root,
                "real-image-profile\\test-sign-334x330.bmp")), out rgb) != JxrError.None)
                return false;
            byte[] bgrPixels = (byte[])rgb.Pixels.Clone();
            for (int index = 0; index < bgrPixels.Length; index += 3)
            {
                byte red = bgrPixels[index];
                bgrPixels[index] = bgrPixels[index + 2];
                bgrPixels[index + 2] = red;
            }
            JxrImage bgr = new JxrImage(rgb.Width, rgb.Height,
                JxrPixelFormat.Bgr24, bgrPixels, rgb.Stride);
            byte[] bitmap;
            JxrImage reread;
            if (JxrBmpAdapter.WriteRgb24(bgr, out bitmap) != JxrError.None ||
                JxrBmpAdapter.ReadRgb24(bitmap, out reread) != JxrError.None ||
                !EqualBytes(reread.Pixels, rgb.Pixels)) return false;
            byte[] encoded;
            JxrError error = JxrCodec.Encode(bgr, new JxrEncoderOptions(),
                out encoded);
            if (error != JxrError.None || !EqualBytes(encoded,
                File.ReadAllBytes(Path.Combine(root,
                    "real-image-profile\\test-sign-334x330.jxr"))))
                return false;
            JxrDecoderOptions options = new JxrDecoderOptions();
            options.OutputFormat = JxrPixelFormat.Bgr24;
            JxrImage restored;
            error = JxrCodec.Decode(encoded, options, out restored);
            return error == JxrError.None && restored != null &&
                restored.Format == JxrPixelFormat.Bgr24 &&
                EqualBytes(restored.Pixels, bgrPixels);
        }

        private static bool TestRgb444SizesRoundTrip()
        {
            int[,] dimensions = { { 1, 1 }, { 15, 17 }, { 16, 16 },
                { 31, 19 }, { 32, 32 } };
            for (int sample = 0; sample < dimensions.GetLength(0); sample++)
            {
                int width = dimensions[sample, 0];
                int height = dimensions[sample, 1];
                int stride = width * 3 + 5;
                byte[] pixels = new byte[stride * height];
                for (int y = 0; y < height; y++)
                    for (int x = 0; x < width; x++)
                    {
                        int index = y * stride + x * 3;
                        pixels[index] = (byte)((x * 37 + y * 11) & 255);
                        pixels[index + 1] = (byte)((x * 13 + y * 29) & 255);
                        pixels[index + 2] = (byte)((x * 7 + y * 53) & 255);
                    }
                JxrImage image = new JxrImage(width, height,
                    JxrPixelFormat.Rgb24, pixels, stride);
                byte[] jxr;
                JxrError error = JxrCodec.Encode(image,
                    new JxrEncoderOptions(), out jxr);
                if (error != JxrError.None) return false;
                if (width == 15 && height == 17)
                {
                    DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
                    while (directory != null && !File.Exists(Path.Combine(
                        directory.FullName, "managed\\fixtures\\rgb444-15x17.jxr")))
                        directory = directory.Parent;
                    if (directory == null || !EqualBytes(jxr,
                        File.ReadAllBytes(Path.Combine(directory.FullName,
                            "managed\\fixtures\\rgb444-15x17.jxr"))))
                        return false;
                    JxrHeaders headers;
                    if (JxrHeaders.Read(jxr, out headers) != JxrError.None)
                        return false;
                    int shortLength = jxr.Length - headers.CodestreamOffset - 1;
                    byte[] truncated = new byte[shortLength];
                    Array.Copy(jxr, headers.CodestreamOffset, truncated,
                        0, shortLength);
                    JxrDecoderOptions truncatedOptions = new JxrDecoderOptions();
                    truncatedOptions.OutputFormat = JxrPixelFormat.Rgb24;
                    JxrImage truncatedImage;
                    if (JxrCodec.Decode(truncated, truncatedOptions,
                        out truncatedImage) == JxrError.None)
                    { Console.WriteLine("Truncated RGB444 stream accepted"); return false; }
                }
                JxrDecoderOptions decodeOptions = new JxrDecoderOptions();
                decodeOptions.OutputFormat = JxrPixelFormat.Rgb24;
                JxrImage decoded;
                error = JxrCodec.Decode(jxr, decodeOptions, out decoded);
                if (error != JxrError.None || decoded == null ||
                    decoded.Width != width || decoded.Height != height)
                {
                    Console.WriteLine("RGB444 size " + width + "x" + height +
                        " decode: " + error);
                    return false;
                }
                for (int y = 0; y < height; y++)
                    for (int x = 0; x < width * 3; x++)
                        if (decoded.Pixels[y * decoded.Stride + x] !=
                            pixels[y * stride + x])
                        {
                            Console.WriteLine("RGB444 size " + width + "x" +
                                height + " differs at " + x + "," + y);
                            return false;
                        }
            }
            return true;
        }

        private static bool TestRgb444QualityNativeFixtures()
        {
            DirectoryInfo directory = new DirectoryInfo(Environment.CurrentDirectory);
            while (directory != null && !File.Exists(Path.Combine(directory.FullName,
                "managed\\fixtures\\rgb444-q16-all.jxr"))) directory = directory.Parent;
            if (directory == null) return false;
            string fixtureRoot = Path.Combine(directory.FullName, "managed\\fixtures");
            string[] names = { "rgb444-q16-dc-only", "rgb444-q16-no-hp",
                "rgb444-q16-all", "rgb444-q16-no-flex", "rgb444-q16-trim3",
                "rgb444-city-q16-all" };
            JxrDecoderOptions options = new JxrDecoderOptions();
            options.OutputFormat = JxrPixelFormat.Rgb24;
            for (int item = 0; item < names.Length; item++)
            {
                JxrImage expected, actual;
                if (JxrBmpAdapter.ReadRgb24(File.ReadAllBytes(Path.Combine(fixtureRoot,
                    names[item] + "-restored.bmp")), out expected) != JxrError.None)
                    return false;
                JxrError error = JxrCodec.Decode(File.ReadAllBytes(Path.Combine(fixtureRoot,
                    names[item] + ".jxr")), options, out actual);
                if (error != JxrError.None || actual == null ||
                    !EqualBytes(actual.Pixels, expected.Pixels))
                {
                    Console.WriteLine("RGB444 quality decode " + names[item] + ": " + error);
                    if (actual != null)
                        for (int index = 0; index < actual.Pixels.Length; index++)
                            if (actual.Pixels[index] != expected.Pixels[index])
                            {
                                Console.WriteLine("First mismatch at " + index + ": " +
                                    actual.Pixels[index] + "/" + expected.Pixels[index]);
                                break;
                            }
                    return false;
                }
            }
            return true;
        }

        // Direct counterpart of native bit_math_vectors, including count
        // normalization beyond the 32-bit rotation boundary.
        private static bool TestBitMathVectors()
        {
            uint value = 0x12345678U;
            return JxrBitMath.RotateLeft32(value, 0) == 0x12345678U &&
                JxrBitMath.RotateLeft32(value, 1) == 0x2468acf0U &&
                JxrBitMath.RotateLeft32(value, 14) == 0x159e048dU &&
                JxrBitMath.RotateLeft32(value, 16) == 0x56781234U &&
                JxrBitMath.RotateLeft32(value, 31) == 0x091a2b3cU &&
                JxrBitMath.RotateLeft32(value, 32) == 0x12345678U &&
                JxrBitMath.RotateLeft32(value, 33) == 0x2468acf0U &&
                JxrBitMath.RotateLeft32(value, 63) == 0x091a2b3cU &&
                JxrBitMath.LowMask32(0) == 0U &&
                JxrBitMath.LowMask32(1) == 0x1U &&
                JxrBitMath.LowMask32(14) == 0x3fffU &&
                JxrBitMath.LowMask32(16) == 0xffffU &&
                JxrBitMath.LowMask32(31) == 0x7fffffffU &&
                JxrBitMath.LowMask32(32) == 0xffffffffU &&
                JxrBitMath.LowMask32(33) == 0xffffffffU;
        }

        // Direct counterpart of native bit_reader_vectors.  The final read
        // deliberately consumes the padded nibble before reporting EOF.
        private static bool TestBitReaderVectors()
        {
            uint value;
            JxrBitReader reader = new JxrBitReader(new byte[] { 0xb1, 0xab, 0xcd, 0xf0 });
            if (reader.ReadBits(0, out value) != JxrError.None || value != 0 ||
                reader.ReadBits(3, out value) != JxrError.None || value != 5 ||
                reader.ReadBits(5, out value) != JxrError.None || value != 17 ||
                reader.ReadBits(16, out value) != JxrError.None || value != 0xabcdU ||
                reader.ReadBits(4, out value) != JxrError.None || value != 15 ||
                reader.ReadBits(16, out value) != JxrError.UnexpectedEndOfStream ||
                !reader.HasFailed || reader.ByteIndex != 4 || reader.BufferedBitCount != 0 ||
                reader.BitPosition != 32) return false;

            reader = new JxrBitReader(new byte[] { 0xde, 0xad, 0xbe, 0xef });
            return reader.ReadBits(32, out value) == JxrError.None && value == 0xdeadbeefU &&
                reader.ByteIndex == 4 && reader.BufferedBitCount == 0 &&
                reader.ReadBits(33, out value) == JxrError.InvalidArgument && reader.HasFailed;
        }

        // Direct counterpart of native packet_header_syntax_reader_vectors.
        private static bool TestPacketHeaderSyntaxReaderVectors()
        {
            JxrPacketHeader header;
            JxrBitReader reader = new JxrBitReader(new byte[] { 0x00, 0x00, 0x01, 0xad });
            if (JxrPacketReader.ReadHeader(reader, out header) != JxrError.None ||
                header == null || !header.IsValid || header.TileId != 21 || header.PacketType != 5)
                return false;

            reader = new JxrBitReader(new byte[] { 0x00, 0x02, 0x01, 0xad });
            if (JxrPacketReader.ReadHeader(reader, out header) != JxrError.None || header.IsValid)
                return false;

            reader = new JxrBitReader(new byte[] { 0x00, 0x00, 0x01 });
            return JxrPacketReader.ReadHeader(reader, out header) == JxrError.UnexpectedEndOfStream &&
                header != null && header.Prefix0 == 0 && header.Prefix1 == 0 && header.Marker == 1 &&
                reader.BitPosition == 24 && reader.HasFailed &&
                JxrPacketReader.ReadHeader(null, out header) == JxrError.InvalidArgument && header == null;
        }

        // Direct counterpart of native adaptive_scan_vectors.
        private static bool TestAdaptiveScanVectors()
        {
            JxrAdaptiveScan scan = new JxrAdaptiveScan(new uint[] { 0, 1, 2, 3 });
            uint value;
            if (scan.ResetTotals(4) != JxrError.None ||
                scan.GetTotal(0, out value) != JxrError.None || value != JxrAdaptiveScan.MaximumTotal ||
                scan.GetTotal(1, out value) != JxrError.None || value != 32 ||
                scan.GetTotal(2, out value) != JxrError.None || value != 30 ||
                scan.GetCoefficientIndex(2, out value) != JxrError.None || value != 2) return false;
            return scan.ObserveNonZero(2) == JxrError.None &&
                scan.ObserveNonZero(2) == JxrError.None &&
                scan.ObserveNonZero(2) == JxrError.None &&
                scan.GetCoefficientIndex(1, out value) == JxrError.None && value == 2 &&
                scan.GetTotal(1, out value) == JxrError.None && value == 33 &&
                scan.GetCoefficientIndex(2, out value) == JxrError.None && value == 1 &&
                scan.GetTotal(2, out value) == JxrError.None && value == 32;
        }

        // Direct counterpart of native adaptive_scan_state_vectors.
        private static bool TestAdaptiveScanStateVectors()
        {
            JxrAdaptiveScan scan = new JxrAdaptiveScan(new uint[] { 3, 2, 1, 0 });
            uint value;
            if (scan.ResetTotals(4) != JxrError.None ||
                scan.GetCoefficientIndex(2, out value) != JxrError.None || value != 1) return false;
            return scan.ObserveNonZero(2) == JxrError.None &&
                scan.ObserveNonZero(2) == JxrError.None &&
                scan.ObserveNonZero(2) == JxrError.None &&
                scan.GetCoefficientIndex(1, out value) == JxrError.None && value == 1 &&
                scan.GetCoefficientIndex(2, out value) == JxrError.None && value == 2 &&
                scan.ResetTotals(0) == JxrError.None &&
                scan.ObserveNonZero(4) == JxrError.InvalidArgument;
        }

        // Mirrors InitZigzagScan's LP, horizontal and vertical defaults.
        private static bool TestAdaptiveScanDefaultVectors()
        {
            JxrAdaptiveScanSet scans = JxrAdaptiveScanSet.CreateDefault();
            uint[] expectedLowpass = { 0, 1, 4, 5, 2, 8, 6, 9, 3, 12, 10, 7, 13, 11, 14, 15 };
            uint[] expectedHorizontal = { 0, 5, 10, 12, 1, 2, 8, 4, 6, 9, 3, 14, 13, 7, 11, 15 };
            uint[] expectedVertical = { 0, 10, 2, 12, 5, 9, 4, 8, 1, 13, 6, 15, 14, 3, 11, 7 };
            uint value;
            int index;
            for (index = 0; index < 16; index++)
            {
                if (scans.Lowpass.GetCoefficientIndex(index, out value) != JxrError.None || value != expectedLowpass[index] ||
                    scans.Horizontal.GetCoefficientIndex(index, out value) != JxrError.None || value != expectedHorizontal[index] ||
                    scans.Vertical.GetCoefficientIndex(index, out value) != JxrError.None || value != expectedVertical[index]) return false;
            }
            return true;
        }

        // Same reset vector as native explicit_entropy_context.  Totals and
        // configuration survive; model counters and scan indexes reset.
        private static bool TestExplicitEntropyContext()
        {
            JxrEntropyContext context = new JxrEntropyContext();
            JxrAdaptiveModel dc = context.DcModel;
            JxrAdaptiveModel lp = context.LpModel;
            JxrAdaptiveModel ac = context.AcModel;
            JxrAdaptiveScan scan = context.LowpassScan;
            JxrAdaptiveScan horizontal = context.HorizontalScan;
            JxrAdaptiveScan vertical = context.VerticalScan;
            JxrLowpassCbpState lowpassCbp = context.LowpassCbp;
            JxrCbpPredictionModel highpassCbp = context.HighpassCbp;
            int flcState, bits, zeroCount, oneCount, cbpState;
            uint index, total;

            if (dc.Set(0, 0, 2) != JxrError.None ||
                lp.Set(1, 7, 4) != JxrError.None ||
                ac.Set(1, 0, 9) != JxrError.None ||
                highpassCbp.Set(0, 2, 4, 1) != JxrError.None ||
                highpassCbp.Set(1, -4, -2, 0) != JxrError.None ||
                scan.ResetTotals(16) != JxrError.None ||
                scan.ObserveNonZero(2) != JxrError.None ||
                scan.ObserveNonZero(2) != JxrError.None ||
                scan.ObserveNonZero(2) != JxrError.None ||
                scan.SetCoefficientIndex(2, 14) != JxrError.None)
                return false;
            lowpassCbp.Observe(0, 3);
            context.TrimFlexBits = 3;
            context.InRoi = true;
            context.Reset();

            if (context.DcModel != dc || context.LpModel != lp || context.AcModel != ac ||
                context.LowpassScan != scan || context.HorizontalScan != horizontal ||
                context.VerticalScan != vertical || context.LowpassCbp != lowpassCbp ||
                context.HighpassCbp != highpassCbp ||
                scan.GetCoefficientIndex(1, out index) != JxrError.None || index != 1 ||
                scan.GetCoefficientIndex(2, out index) != JxrError.None || index != 4 ||
                scan.GetTotal(1, out total) != JxrError.None || total != 33 ||
                horizontal.GetCoefficientIndex(1, out index) != JxrError.None || index != 5 ||
                vertical.GetCoefficientIndex(1, out index) != JxrError.None || index != 10 ||
                dc.Band != JxrAdaptiveBand.Dc || lp.Band != JxrAdaptiveBand.Lowpass ||
                ac.Band != JxrAdaptiveBand.Highpass ||
                dc.Get(0, out flcState, out bits) != JxrError.None || flcState != 0 || bits != 8 ||
                dc.Get(1, out flcState, out bits) != JxrError.None || bits != 8 ||
                lp.Get(0, out flcState, out bits) != JxrError.None || bits != 4 ||
                lp.Get(1, out flcState, out bits) != JxrError.None || flcState != 0 || bits != 4 ||
                ac.Get(0, out flcState, out bits) != JxrError.None || bits != 0 ||
                ac.Get(1, out flcState, out bits) != JxrError.None || bits != 0 ||
                lowpassCbp.ZeroCount != 1 || lowpassCbp.MaxCount != 1 ||
                highpassCbp.Get(0, out zeroCount, out oneCount, out cbpState) != JxrError.None ||
                zeroCount != -4 || oneCount != 4 || cbpState != 0 ||
                highpassCbp.Get(1, out zeroCount, out oneCount, out cbpState) != JxrError.None ||
                zeroCount != -4 || oneCount != 4 || cbpState != 0 ||
                context.TrimFlexBits != 3 || !context.InRoi) return false;
            return true;
        }

        // Direct counterpart of native coefficient_buffer_vectors.
        private static bool TestCoefficientBufferVectors()
        {
            int[] values = { 3, 5, 7, 11, 13 };
            int value;
            JxrCoefficientBuffer buffer = new JxrCoefficientBuffer(values, 1, 3);
            if (buffer.Get(0, out value) != JxrError.None || value != 5 ||
                buffer.Get(2, out value) != JxrError.None || value != 11 ||
                buffer.Set(1, -2) != JxrError.None || buffer.Add(2, 4) != JxrError.None ||
                values[0] != 3 || values[1] != 5 || values[2] != -2 ||
                values[3] != 15 || values[4] != 13) return false;
            buffer.Clear();
            return values[0] == 3 && values[1] == 0 && values[2] == 0 &&
                values[3] == 0 && values[4] == 13 &&
                buffer.Get(3, out value) == JxrError.InvalidArgument;
        }

        // Direct counterpart of native coefficient_plane_state_vectors.
        private static bool TestCoefficientPlaneStateVectors()
        {
            int[] plane0 = new int[256];
            int[] plane1 = new int[256];
            int[] values;
            int length;
            JxrCoefficientBuffer block;
            JxrCoefficientPlaneState state = new JxrCoefficientPlaneState(
                new int[][] { plane0, plane1 }, JxrCoefficientColorFormat.Yuv444, 2);
            if (state.GetBlock(1, 4, 16, out block) != JxrError.None ||
                state.GetPlane(0, out values) != JxrError.None || values != plane0 ||
                state.GetLength(0, out length) != JxrError.None || length != 256 ||
                state.GetLength(1, out length) != JxrError.None || length != 256 ||
                block.Offset != 4 || block.Count != 16) return false;
            return block.Set(0, 42) == JxrError.None && plane1[4] == 42 &&
                state.GetBlock(1, 250, 7, out block) == JxrError.InvalidArgument;
        }

        // Direct counterpart of native macroblock_state_vectors.  Snapshots
        // explicitly replace the native load/commit bridge.
        private static bool TestMacroblockStateVectors()
        {
            int channel;
            int coefficient;
            JxrMacroblockSnapshot snapshot = new JxrMacroblockSnapshot(16);
            JxrMacroblockState state = new JxrMacroblockState(16);
            for (channel = 0; channel < 16; channel++)
            {
                int index;
                for (index = 0; index < JxrMacroblockState.CoefficientsPerChannel; index++)
                    snapshot.SetDcCoefficient(channel, index, -1);
            }
            snapshot.Orientation = 1;
            if (state.LoadFrom(snapshot) != JxrError.None || state.ClearDc(2) != JxrError.None ||
                state.GetDcCoefficient(0, 0, out coefficient) != JxrError.None || coefficient != 0 ||
                state.GetDcCoefficient(1, 15, out coefficient) != JxrError.None || coefficient != 0 ||
                state.GetDcCoefficient(2, 0, out coefficient) != JxrError.None || coefficient != -1 ||
                state.SetDcCoefficient(1, 5, 42) != JxrError.None ||
                state.GetDcCoefficient(1, 5, out coefficient) != JxrError.None || coefficient != 42)
                return false;
            state.ResetQuantizerIndices();
            state.SetLowpassQuantizerIndex(3);
            state.SetHighpassQuantizerIndex(7);
            if (state.LowpassQuantizerIndex != 3 || state.HighpassQuantizerIndex != 7 ||
                state.Orientation != 1 || snapshot.LowpassQuantizerIndex == 3 ||
                state.CopyTo(snapshot) != JxrError.None ||
                snapshot.GetDcCoefficient(0, 0, out coefficient) != JxrError.None || coefficient != 0 ||
                snapshot.GetDcCoefficient(1, 5, out coefficient) != JxrError.None || coefficient != 42 ||
                snapshot.LowpassQuantizerIndex != 3 || snapshot.HighpassQuantizerIndex != 7 ||
                state.Orientation != 1) return false;
            snapshot.SetDcCoefficient(0, 0, 7);
            snapshot.LowpassQuantizerIndex = 2;
            snapshot.HighpassQuantizerIndex = 4;
            snapshot.Orientation = 9;
            return state.LoadFrom(snapshot) == JxrError.None &&
                state.GetDcCoefficient(0, 0, out coefficient) == JxrError.None && coefficient == 7 &&
                state.LowpassQuantizerIndex == 2 && state.HighpassQuantizerIndex == 4 &&
                state.Orientation == 9;
        }

        // Direct counterpart of native macroblock_cbp_state_vectors.
        private static bool TestMacroblockCbpStateVectors()
        {
            int[] cbp = new int[16];
            int[] differential = new int[16];
            int value;
            JxrMacroblockCbpState state = new JxrMacroblockCbpState(16);
            if (state.LoadFrom(cbp, differential) != JxrError.None ||
                state.SetCbp(0, 0x1234) != JxrError.None || state.SetCbp(1, 0x3f) != JxrError.None ||
                state.SetCbp(2, 0x55) != JxrError.None || state.SetCbp(15, 0x7a) != JxrError.None ||
                state.SetDifferential(0, 0x4321) != JxrError.None ||
                state.SetDifferential(1, 0x2a) != JxrError.None ||
                state.SetDifferential(2, 0x15) != JxrError.None ||
                state.GetCbp(0, out value) != JxrError.None || value != 0x1234 ||
                state.GetCbp(1, out value) != JxrError.None || value != 0x3f ||
                state.GetCbp(2, out value) != JxrError.None || value != 0x55 ||
                state.GetCbp(15, out value) != JxrError.None || value != 0x7a ||
                state.GetDifferential(0, out value) != JxrError.None || value != 0x4321 ||
                state.GetDifferential(1, out value) != JxrError.None || value != 0x2a ||
                state.GetDifferential(2, out value) != JxrError.None || value != 0x15 || cbp[0] != 0)
                return false;
            if (state.CopyTo(cbp, differential) != JxrError.None || cbp[0] != 0x1234 ||
                cbp[1] != 0x3f || cbp[2] != 0x55 || cbp[15] != 0x7a ||
                differential[0] != 0x4321 || differential[1] != 0x2a || differential[2] != 0x15)
                return false;
            cbp[0] = 7;
            differential[0] = 9;
            return state.LoadFrom(cbp, differential) == JxrError.None &&
                state.GetCbp(0, out value) == JxrError.None && value == 7 &&
                state.GetDifferential(0, out value) == JxrError.None && value == 9;
        }

        // Direct counterpart of native lowpass_cbp_state_vectors.
        private static bool TestLowpassCbpStateVectors()
        {
            JxrLowpassCbpState state = new JxrLowpassCbpState(1, 1);
            state.Observe(0, 3);
            if (state.ZeroCount != -2 || state.MaxCount != 2) return false;
            state.Observe(3, 3);
            if (state.ZeroCount != -1 || state.MaxCount != -1) return false;
            state = new JxrLowpassCbpState(-8, 7);
            state.Observe(0, 3);
            return state.ZeroCount == -8 && state.MaxCount == 7;
        }

        // Direct counterpart of native highpass_cbp_state_vectors.
        private static bool TestHighpassCbpStateVectors()
        {
            JxrAdaptiveHuffman pattern = new JxrAdaptiveHuffman(4, null, null);
            JxrAdaptiveHuffman count = new JxrAdaptiveHuffman(4, null, null);
            JxrCbpPredictionModel model = new JxrCbpPredictionModel();
            JxrHighpassCbpState state = new JxrHighpassCbpState(pattern, count, model);
            return state.PatternHuffman == pattern && state.CountHuffman == count &&
                state.PredictionModel == model && state.Adapt() == JxrError.None &&
                pattern.IsInitialized && count.IsInitialized;
        }

        // Direct counterpart of native huffman_state_set_vectors.
        private static bool TestHuffmanStateSetVectors()
        {
            JxrAdaptiveHuffman huffman = new JxrAdaptiveHuffman(2, new int[] { 3, 7 }, null);
            JxrAdaptiveHuffman[] states = new JxrAdaptiveHuffman[8];
            JxrHuffmanStateSet stateSet;
            huffman.Discriminant = 5;
            states[3] = huffman;
            stateSet = new JxrHuffmanStateSet(states);
            return stateSet.Get(3) == huffman &&
                stateSet.Observe(3, 1) == JxrError.None && huffman.Discriminant == 12;
        }

        // Direct counterpart of native adaptive_huffman_vectors.
        private static bool TestAdaptiveHuffmanVectors()
        {
            JxrAdaptiveHuffman state = new JxrAdaptiveHuffman(3,
                new int[] { -2, 0, 5 }, new int[] { 4, -1, 2 });
            state.Discriminant = 7;
            state.SecondaryDiscriminant = -3;
            if (state.ObserveSymbol(2) != JxrError.None ||
                state.Discriminant != 12 || state.SecondaryDiscriminant != -1 ||
                state.ObserveSymbol(3) != JxrError.InvalidArgument) return false;

            state = new JxrAdaptiveHuffman(6,
                new int[6], new int[6]);
            if (state.Adapt() != JxrError.None || !state.IsInitialized ||
                state.TableIndex != 1 || state.Discriminant != 0 ||
                state.LowerBound != -8 || state.UpperBound != 8) return false;
            state.SecondaryDiscriminant = 9;
            if (state.Adapt() != JxrError.None || state.TableIndex != 2 ||
                state.Discriminant != 0 || state.SecondaryDiscriminant != 0) return false;

            state = new JxrAdaptiveHuffman(5, new int[5], null);
            if (state.Adapt() != JxrError.None) return false;
            state.Discriminant = 9;
            if (state.Adapt() != JxrError.None || state.TableIndex != 1) return false;
            state.Discriminant = 100;
            return state.Adapt() == JxrError.None && state.Discriminant == 64 &&
                state.UpperBound == (1 << 30);
        }

        // Direct counterpart of native huffman_decoder_vectors, using an
        // explicit MSB-first byte source instead of BitIOInfo compatibility state.
        private static bool TestHuffmanDecoderVectors()
        {
            short[] root = new short[32];
            short[] branch = new short[JxrHuffmanTable.BranchOffset + 1];
            int index;
            int symbol;
            JxrHuffmanTable table;
            JxrBitReader reader;
            for (index = 0; index < root.Length; index++) root[index] = (short)((2 << 3) | 5);
            table = new JxrHuffmanTable(root);
            reader = new JxrBitReader(new byte[] { 0, 0 });
            if (JxrHuffmanDecoder.DecodeSymbol(table, reader, out symbol) != JxrError.None ||
                symbol != 2 || reader.BitPosition != 5) return false;

            for (index = 0; index < 32; index++) branch[index] = -8;
            branch[JxrHuffmanTable.BranchOffset - 8] = 4;
            branch[JxrHuffmanTable.BranchOffset - 7] = 6;
            table = new JxrHuffmanTable(branch);
            reader = new JxrBitReader(new byte[] { 0x04, 0 });
            return JxrHuffmanDecoder.DecodeSymbol(table, reader, out symbol) == JxrError.None &&
                symbol == 6 && reader.BitPosition == 6;
        }

        private static bool TestHuffmanShortTailVectors()
        {
            short[] oneBit = new short[32];
            short[] twoBit = new short[32];
            for (int index = 0; index < 32; index++)
            {
                oneBit[index] = (short)(((index < 16 ? 1 : 2) << 3) | 1);
                twoBit[index] = (short)((3 << 3) | 2);
            }
            int symbol;
            JxrBitReader reader = new JxrBitReader(new byte[] { 0x01 });
            if (reader.ConsumeBits(7) != JxrError.None ||
                JxrHuffmanDecoder.DecodeSymbol(new JxrHuffmanTable(oneBit),
                    reader, out symbol) != JxrError.None ||
                symbol != 2 || reader.BitPosition != 8 || reader.HasFailed)
                return false;
            reader = new JxrBitReader(new byte[] { 0x01 });
            if (reader.ConsumeBits(7) != JxrError.None ||
                JxrHuffmanDecoder.DecodeShortSymbol(new JxrHuffmanTable(oneBit),
                    reader, out symbol) != JxrError.None ||
                symbol != 2 || reader.BitPosition != 8 || reader.HasFailed)
                return false;
            reader = new JxrBitReader(new byte[] { 0 });
            if (reader.ConsumeBits(7) != JxrError.None ||
                JxrHuffmanDecoder.DecodeSymbol(new JxrHuffmanTable(twoBit),
                    reader, out symbol) != JxrError.UnexpectedEndOfStream ||
                reader.BitPosition != 7 || !reader.HasFailed)
                return false;
            reader = new JxrBitReader(new byte[] { 0 });
            return reader.ConsumeBits(7) == JxrError.None &&
                JxrHuffmanDecoder.DecodeShortSymbol(new JxrHuffmanTable(twoBit),
                    reader, out symbol) == JxrError.UnexpectedEndOfStream &&
                reader.BitPosition == 7 && reader.HasFailed;
        }

        // Uses the built-in JPEG XR alphabet-5 catalog selected by Adapt().
        // The native counterpart checks the same symbol, delta and bit length.
        private static bool TestAdaptiveHuffmanTableCatalogVectors()
        {
            JxrAdaptiveHuffman state = new JxrAdaptiveHuffman(5, null, null);
            int symbol;
            int bitCount;
            uint code;
            if (state.Adapt() != JxrError.None || state.DecoderTable == null) return false;
            if (state.GetCodeWord(3, out code, out bitCount) != JxrError.None ||
                code != 0 || bitCount != 4) return false;
            return state.DecodeSymbol(new JxrBitReader(new byte[] { 0, 0 }), out symbol) == JxrError.None &&
                symbol == 3 && state.Discriminant == 1;
        }

        private static bool TestAdaptiveHuffmanCatalogSignatureVectors()
        {
            return HasRootEntry(4, 19) && HasRootEntry(5, 28) &&
                HasRootEntry(6, 12) && HasRootEntry(7, 45) &&
                HasRootEntry(8, 53) && HasRootEntry(9, 13) &&
                HasRootEntry(12, -32736) && HasSecondaryTransition(6, 4) &&
                HasSecondaryTransition(12, -32736) && HasPrimaryTransition(8, 53);
        }

        private static bool HasRootEntry(int symbols, int expectedEntry)
        {
            JxrAdaptiveHuffman state = new JxrAdaptiveHuffman(symbols, null, null);
            int entry;
            return state.Adapt() == JxrError.None && state.DecoderTable != null &&
                state.DecoderTable.TryGetEntry(0, out entry) && entry == expectedEntry;
        }

        private static bool HasSecondaryTransition(int symbols, int expectedEntry)
        {
            JxrAdaptiveHuffman state = new JxrAdaptiveHuffman(symbols, null, null);
            int entry;
            if (state.Adapt() != JxrError.None) return false;
            state.SecondaryDiscriminant = 9;
            return state.Adapt() == JxrError.None && state.DecoderTable.TryGetEntry(0, out entry) &&
                entry == expectedEntry;
        }

        private static bool HasPrimaryTransition(int symbols, int expectedEntry)
        {
            JxrAdaptiveHuffman state = new JxrAdaptiveHuffman(symbols, null, null);
            int entry;
            if (state.Adapt() != JxrError.None) return false;
            state.Discriminant = 9;
            return state.Adapt() == JxrError.None && state.TableIndex == 1 &&
                state.DecoderTable.TryGetEntry(0, out entry) && entry == expectedEntry;
        }

        private static bool TestAdaptiveDecodeVectors()
        {
            short[] entries = new short[32];
            int index;
            int symbol;
            JxrAdaptiveHuffman state = new JxrAdaptiveHuffman(3, new int[] { 1, 2, 3 }, null);
            for (index = 0; index < entries.Length; index++) entries[index] = (short)((2 << 3) | 5);
            return state.DecodeSymbol(new JxrHuffmanTable(entries),
                new JxrBitReader(new byte[] { 0 }), out symbol) == JxrError.None &&
                symbol == 2 && state.Discriminant == 3;
        }

        private static bool TestErrorVectors()
        {
            int symbol;
            JxrBitReader reader = new JxrBitReader(new byte[0]);
            short[] entries = new short[32];
            int index;
            for (index = 0; index < entries.Length; index++) entries[index] = (short)((1 << 3) | 5);
            return JxrHuffmanDecoder.DecodeSymbol(new JxrHuffmanTable(entries), reader, out symbol) ==
                JxrError.UnexpectedEndOfStream;
        }
    }
}
