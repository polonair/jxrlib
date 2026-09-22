using System;
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
            new TestCase("adaptive_scan_vectors", TestAdaptiveScanVectors),
            new TestCase("adaptive_scan_state_vectors", TestAdaptiveScanStateVectors),
            new TestCase("adaptive_scan_default_vectors", TestAdaptiveScanDefaultVectors),
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
