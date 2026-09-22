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
