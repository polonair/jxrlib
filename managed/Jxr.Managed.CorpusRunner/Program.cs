using System;
using System.IO;
using System.Text;
using Jxr.Managed.Core;

namespace Jxr.Managed.CorpusRunner
{
    internal static class Program
    {
        private static int Main(string[] args)
        {
            if (args.Length == 3 && args[0] == "profile")
            {
                try
                {
                    byte[] input = File.ReadAllBytes(args[1]);
                    JxrSourceProfile profile;
                    JxrError error = JxrSourceProfileReader.Read(input, out profile);
                    if (error != JxrError.None)
                    {
                        Console.Error.WriteLine(error.ToString());
                        return 1;
                    }
                    File.WriteAllText(args[2], ProfileJson(profile),
                        new UTF8Encoding(false));
                    return 0;
                }
                catch (Exception exception)
                {
                    Console.Error.WriteLine(exception.GetType().Name + ": " +
                        exception.Message);
                    return 1;
                }
            }
            if (args.Length != 5 || args[0] != "decode" ||
                (args[2] != "color" && args[2] != "alpha"))
            {
                Console.Error.WriteLine(
                    "Usage: Jxr.Managed.CorpusRunner.exe decode <input.jxr> <color|alpha> <pixels.bin> <metadata.txt>");
                return 2;
            }

            try
            {
                byte[] bytes = File.ReadAllBytes(args[1]);
                JxrDecoderOptions options = new JxrDecoderOptions();
                if (args[2] == "alpha")
                {
                    options.OutputFormat = JxrPixelFormat.Gray8;
                    options.AlphaMode = JxrAlphaDecodeMode.AlphaOnly;
                }
                else
                {
                    options.OutputFormat = JxrPixelFormat.Bgr24;
                    options.AlphaMode = JxrAlphaDecodeMode.ColorOnly;
                }

                JxrImage image;
                JxrError error = JxrCodec.Decode(bytes, options, out image);
                if (error != JxrError.None)
                {
                    Console.Error.WriteLine(error.ToString());
                    return 1;
                }

                using (FileStream output = new FileStream(args[3],
                    FileMode.Create, FileAccess.Write, FileShare.None))
                    output.Write(image.Pixels, 0, image.Pixels.Length);

                using (StreamWriter metadata = new StreamWriter(args[4], false))
                {
                    metadata.WriteLine("width=" + image.Width);
                    metadata.WriteLine("height=" + image.Height);
                    metadata.WriteLine("stride=" + image.Stride);
                    metadata.WriteLine("format=" + image.Format);
                }
                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(exception.GetType().Name + ": " +
                    exception.Message);
                return 1;
            }
        }

        private static string ProfileJson(JxrSourceProfile profile)
        {
            StringBuilder json = new StringBuilder(4096);
            JxrHeaders h = profile.Headers;
            json.Append("{\"schema_version\":2,\"container\":");
            JsonString(json, h.ContainerKind.ToString());
            json.Append(",\"pixel_format_guid\":"); JsonString(json, h.PixelFormatGuid);
            json.Append(",\"container_width\":"); json.Append(h.ContainerWidth);
            json.Append(",\"container_height\":"); json.Append(h.ContainerHeight);
            json.Append(",\"orientation_tag\":"); json.Append(h.OrientationTag);
            json.Append(",\"horizontal_dpi\":"); json.Append(h.HorizontalDpi.ToString(
                System.Globalization.CultureInfo.InvariantCulture));
            json.Append(",\"vertical_dpi\":"); json.Append(h.VerticalDpi.ToString(
                System.Globalization.CultureInfo.InvariantCulture));
            json.Append(",\"alpha_offset\":");
            NullableNumber(json, h.HasPlanarAlpha ? (long?)h.AlphaOffset : null);
            json.Append(",\"alpha_byte_count\":");
            NullableNumber(json, h.HasPlanarAlpha ? (long?)h.AlphaByteCount : null);
            json.Append(",\"alpha_range_tag_value\":");
            NullableNumber(json, h.HasPlanarAlpha ? (long?)h.AlphaRangeTagValue : null);
            json.Append(",\"alpha_range_interpretation\":");
            JsonString(json, h.AlphaRangeInterpretation.ToString());
            json.Append(",\"color_plane\":"); PlaneJson(json, profile.ColorPlane);
            json.Append(",\"alpha_plane\":");
            if (profile.AlphaPlane == null) json.Append("null");
            else PlaneJson(json, profile.AlphaPlane);
            json.Append(",\"packet_syntax_complete\":");
            json.Append(profile.PacketSyntaxComplete ? "true" : "false");
            json.Append(",\"macroblock_quantizer_map_complete\":");
            json.Append(profile.MacroblockQuantizerMapComplete ? "true" : "false");
            json.Append("}");
            return json.ToString();
        }

        private static void PlaneJson(StringBuilder json,
            JxrSourcePlaneProfile profile)
        {
            JxrHeaders h = profile.Headers;
            JxrMainHeader m = h.Main;
            JxrImagePlaneHeader p = h.Plane;
            JxrImagePlaneQuantizerHeader q = h.Quantizers;
            json.Append("{\"codestream_offset\":"); json.Append(h.CodestreamOffset);
            json.Append(",\"codestream_length\":"); json.Append(h.CodestreamLength);
            json.Append(",\"header_bytes\":"); json.Append(h.ByteCount);
            json.Append(",\"width\":"); json.Append(m.Width);
            json.Append(",\"height\":"); json.Append(m.Height);
            json.Append(",\"version\":"); json.Append(m.Version);
            json.Append(",\"subversion\":"); json.Append(m.Subversion);
            json.Append(",\"bitstream_format\":"); json.Append(m.BitstreamFormat);
            json.Append(",\"orientation\":"); json.Append(m.Orientation);
            json.Append(",\"overlap\":"); json.Append(m.Overlap);
            json.Append(",\"coded_bit_depth\":"); json.Append(m.CodedBitDepth);
            json.Append(",\"source_color_format\":"); json.Append(m.SourceColorFormat);
            json.Append(",\"source_bit_depth\":"); json.Append(m.SourceBitDepth);
            json.Append(",\"trim_flexbits\":"); json.Append(m.TrimFlexbits ? "true" : "false");
            json.Append(",\"red_blue_swapped\":"); json.Append(m.RedBlueSwapped ? "true" : "false");
            json.Append(",\"has_alpha\":"); json.Append(m.HasAlpha ? "true" : "false");
            json.Append(",\"hard_tiles\":"); json.Append(m.HasHardTileBoundaries ? "true" : "false");
            json.Append(",\"index_table\":"); json.Append(m.HasIndexTable ? "true" : "false");
            json.Append(",\"extra_top\":"); json.Append(m.ExtraTop);
            json.Append(",\"extra_left\":"); json.Append(m.ExtraLeft);
            json.Append(",\"extra_bottom\":"); json.Append(m.ExtraBottom);
            json.Append(",\"extra_right\":"); json.Append(m.ExtraRight);
            json.Append(",\"tile_columns\":"); json.Append(m.VerticalSliceCountMinusOne + 1);
            json.Append(",\"tile_rows\":"); json.Append(m.HorizontalSliceCountMinusOne + 1);
            json.Append(",\"tile_column_boundaries\":[");
            for (int i = 1; i <= m.VerticalSliceCountMinusOne; i++)
            { if (i > 1) json.Append(','); json.Append(m.GetTileX(i)); }
            json.Append("],\"tile_row_boundaries\":[");
            for (int i = 1; i <= m.HorizontalSliceCountMinusOne; i++)
            { if (i > 1) json.Append(','); json.Append(m.GetTileY(i)); }
            json.Append("],\"plane_color_format\":"); json.Append(p.ColorFormat);
            json.Append(",\"channel_count\":"); json.Append(p.ChannelCount);
            json.Append(",\"scaled_arithmetic\":"); json.Append(p.ScaledArithmetic ? "true" : "false");
            json.Append(",\"subbands\":"); json.Append(p.Subband);
            json.Append(",\"chroma_centering_x\":"); json.Append(p.ChromaCenteringX);
            json.Append(",\"chroma_centering_y\":"); json.Append(p.ChromaCenteringY);
            json.Append(",\"sample_conversion\":"); json.Append(p.HasSampleConversion ? "true" : "false");
            json.Append(",\"mantissa_or_shift\":"); json.Append(p.MantissaOrShift);
            json.Append(",\"exponent_bias\":"); json.Append(p.ExponentBias);
            json.Append(",\"frame_quantizers\":"); FrameQuantizersJson(json, q, p.ChannelCount);
            json.Append(",\"tiles\":[");
            for (int i = 0; i < profile.TileCount; i++)
            {
                if (i > 0) json.Append(',');
                TileJson(json, profile.GetTile(i));
            }
            json.Append("],\"syntax_complete\":");
            json.Append(profile.SyntaxComplete ? "true" : "false");
            json.Append(",\"packet_error\":");
            JsonString(json, profile.PacketError.ToString());
            json.Append(",\"macroblock_quantizer_map_complete\":");
            json.Append(profile.MacroblockQuantizerMapComplete ? "true" : "false");
            json.Append("}");
        }

        private static void FrameQuantizersJson(StringBuilder json,
            JxrImagePlaneQuantizerHeader q, int channels)
        {
            json.Append("{\"mode\":"); json.Append(q.Mode);
            json.Append(",\"dc\":"); FrameQuantizerJson(json, q.HasDc,
                q.DcMode, channels, q.GetDcIndex);
            json.Append(",\"lp\":"); FrameQuantizerJson(json, q.HasLowpass,
                q.LowpassMode, channels, q.GetLowpassIndex);
            json.Append(",\"hp\":"); FrameQuantizerJson(json, q.HasHighpass,
                q.HighpassMode, channels, q.GetHighpassIndex);
            json.Append("}");
        }

        private delegate byte FrameQuantizerIndex(int channel);
        private static void FrameQuantizerJson(StringBuilder json, bool present,
            int mode, int channels, FrameQuantizerIndex getIndex)
        {
            json.Append("{\"present\":"); json.Append(present ? "true" : "false");
            json.Append(",\"channel_mode\":"); json.Append(mode);
            json.Append(",\"indices\":[");
            for (int channel = 0; channel < channels; channel++)
            { if (channel > 0) json.Append(','); json.Append(getIndex(channel)); }
            json.Append("]}");
        }

        private static void TileJson(StringBuilder json, JxrProfileTile tile)
        {
            json.Append("{\"row\":"); json.Append(tile.Row);
            json.Append(",\"column\":"); json.Append(tile.Column);
            json.Append(",\"id\":"); json.Append(tile.Id);
            json.Append(",\"trim_present\":"); json.Append(tile.HasTrimFlexbits ? "true" : "false");
            json.Append(",\"trim_flexbits\":"); json.Append(tile.TrimFlexbits);
            json.Append(",\"dc_quantizers\":"); QuantizerArrayJson(json, tile.DcQuantizers);
            json.Append(",\"lp_quantizers\":"); QuantizerArrayJson(json, tile.LowpassQuantizers);
            json.Append(",\"hp_quantizers\":"); QuantizerArrayJson(json, tile.HighpassQuantizers);
            json.Append(",\"packets\":[");
            for (int i = 0; i < tile.PacketCount; i++)
            {
                if (i > 0) json.Append(',');
                JxrProfilePacket packet = tile.GetPacket(i);
                json.Append("{\"type\":"); json.Append(packet.Type);
                json.Append(",\"tile_id\":"); json.Append(packet.TileId);
                json.Append(",\"offset\":"); json.Append(packet.Offset);
                json.Append(",\"length\":"); json.Append(packet.Length);
                json.Append("}");
            }
            json.Append("]}");
        }

        private static void QuantizerArrayJson(StringBuilder json,
            JxrProfileQuantizer[] quantizers)
        {
            if (quantizers == null) { json.Append("null"); return; }
            json.Append('[');
            for (int i = 0; i < quantizers.Length; i++)
            {
                if (i > 0) json.Append(',');
                JxrProfileQuantizer quantizer = quantizers[i];
                json.Append("{\"copy_previous\":");
                json.Append(quantizer.CopyPrevious ? "true" : "false");
                json.Append(",\"channel_mode\":"); json.Append(quantizer.ChannelMode);
                json.Append(",\"indices\":[");
                for (int channel = 0; channel < quantizer.ChannelCount; channel++)
                { if (channel > 0) json.Append(','); json.Append(quantizer.GetIndex(channel)); }
                json.Append("]}");
            }
            json.Append(']');
        }

        private static void JsonString(StringBuilder output, string value)
        {
            if (value == null) { output.Append("null"); return; }
            output.Append('"');
            for (int i = 0; i < value.Length; i++)
            {
                char c = value[i];
                if (c == '"' || c == '\\') { output.Append('\\'); output.Append(c); }
                else if (c < 32) output.Append("\\u").Append(((int)c).ToString("x4"));
                else output.Append(c);
            }
            output.Append('"');
        }

        private static void NullableNumber(StringBuilder output, long? value)
        {
            if (value.HasValue) output.Append(value.Value);
            else output.Append("null");
        }
    }
}
