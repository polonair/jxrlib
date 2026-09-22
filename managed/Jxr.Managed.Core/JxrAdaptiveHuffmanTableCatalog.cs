namespace Jxr.Managed.Core
{
    // Direct managed representation of the fixed adaptive-Huffman catalogs in
    // image/sys/adapthuff.c.  Decoder lookup tables are derived once from the
    // same (code,length) pairs, avoiding native pointer-layout assumptions.
    internal static class JxrAdaptiveHuffmanTableCatalog
    {
        private static readonly int[] Code4 = { 4, 1, 1, 1, 2, 0, 3, 1, 3 };
        private static readonly int[] Code5 = {
            5, 1, 1, 1, 2, 1, 3, 0, 4, 1, 4,
            5, 1, 1, 0, 3, 1, 3, 2, 3, 3, 3 };
        private static readonly int[] Delta5 = { 0, -1, 0, 1, 1 };
        private static readonly int[] Code6 = {
            6, 1, 1, 0, 5, 1, 3, 1, 5, 1, 2, 1, 4,
            6, 1, 2, 0, 4, 2, 2, 1, 4, 3, 2, 1, 3,
            6, 0, 4, 1, 4, 1, 2, 2, 2, 3, 2, 1, 3,
            6, 0, 5, 1, 5, 1, 2, 1, 1, 1, 4, 1, 3 };
        private static readonly int[] Delta6 = {
            -1, 1, 1, 1, 0, 1,
            -2, 0, 0, 2, 0, 0,
            -1, -1, 0, 1, -2, 0 };
        private static readonly int[] Code7 = {
            7, 1, 2, 2, 2, 3, 2, 1, 3, 1, 4, 0, 5, 1, 5,
            7, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 0, 6, 1, 6 };
        private static readonly int[] Delta7 = { 1, 0, -1, -1, -1, -1, -1 };
        private static readonly int[] Code8 = {
            8, 2, 2, 1, 3, 1, 5, 1, 4, 3, 2, 2, 3, 0, 5, 3, 3,
            8, 1, 3, 2, 3, 1, 4, 3, 3, 4, 3, 5, 3, 0, 4, 3, 2 };
        private static readonly int[] Code9 = {
            9, 2, 3, 0, 5, 2, 4, 1, 5, 2, 5, 1, 1, 3, 3, 3, 5, 3, 4,
            9, 1, 1, 1, 3, 2, 3, 1, 4, 1, 6, 3, 3, 1, 5, 0, 7, 1, 7 };
        private static readonly int[] Delta9 = { 2, 2, 1, 1, -1, -2, -2, -2, -3 };
        private static readonly int[] Code12 = {
            12, 1, 5, 1, 6, 0, 7, 1, 7, 4, 5, 2, 3, 5, 5, 1, 1, 6, 5, 1, 4, 7, 5, 3, 3,
            12, 2, 4, 2, 5, 0, 6, 1, 6, 3, 4, 2, 3, 3, 5, 3, 2, 3, 3, 4, 3, 1, 5, 5, 3,
            12, 3, 2, 1, 3, 0, 7, 1, 7, 1, 5, 2, 3, 2, 7, 3, 3, 4, 3, 5, 3, 3, 7, 1, 4,
            12, 1, 3, 3, 2, 0, 7, 1, 5, 2, 5, 2, 3, 1, 7, 3, 3, 3, 5, 4, 3, 1, 6, 5, 3,
            12, 2, 3, 1, 1, 1, 7, 1, 4, 2, 7, 3, 3, 0, 8, 2, 4, 3, 7, 3, 4, 1, 8, 1, 5 };
        private static readonly int[] Delta12 = {
            1, 1, 1, 1, 1, 0, 0, -1, 2, 1, 0, 0,
            2, 2, -1, -1, -1, 0, -2, -1, 0, 0, -2, -1,
            -1, 1, 0, 2, 0, 0, 0, 0, -2, 0, 1, 1,
            0, 1, 0, 1, -2, 0, -1, -1, -2, -1, -2, -2 };

        internal static bool TryGet(int symbolCount, int tableIndex,
            out int[] codeTable, out int[] primaryDelta, out int[] secondaryDelta,
            out JxrHuffmanTable decoderTable)
        {
            codeTable = null;
            primaryDelta = null;
            secondaryDelta = null;
            decoderTable = null;
            switch (symbolCount)
            {
                case 4:
                    if (tableIndex != 0) return false;
                    codeTable = CopySegment(Code4, 0, 9);
                    break;
                case 5:
                    if (tableIndex < 0 || tableIndex >= 2) return false;
                    codeTable = CopySegment(Code5, tableIndex * 11, 11);
                    primaryDelta = CopySegment(Delta5, 0, 5);
                    break;
                case 6:
                    if (tableIndex < 0 || tableIndex >= 4) return false;
                    codeTable = CopySegment(Code6, tableIndex * 13, 13);
                    primaryDelta = CopySegment(Delta6, (tableIndex == 0 ? 0 : tableIndex - 1) * 6, 6);
                    secondaryDelta = CopySegment(Delta6, (tableIndex == 3 ? 2 : tableIndex) * 6, 6);
                    break;
                case 7:
                    if (tableIndex < 0 || tableIndex >= 2) return false;
                    codeTable = CopySegment(Code7, tableIndex * 15, 15);
                    primaryDelta = CopySegment(Delta7, 0, 7);
                    break;
                case 8:
                    // The native state machine has two indexes for alphabet 8,
                    // but both deliberately select the first fixed table.
                    if (tableIndex < 0 || tableIndex >= 2) return false;
                    codeTable = CopySegment(Code8, 0, 17);
                    break;
                case 9:
                    if (tableIndex < 0 || tableIndex >= 2) return false;
                    codeTable = CopySegment(Code9, tableIndex * 19, 19);
                    primaryDelta = CopySegment(Delta9, 0, 9);
                    break;
                case 12:
                    if (tableIndex < 0 || tableIndex >= 5) return false;
                    codeTable = CopySegment(Code12, tableIndex * 25, 25);
                    primaryDelta = CopySegment(Delta12, (tableIndex == 0 ? 0 : tableIndex - 1) * 12, 12);
                    secondaryDelta = CopySegment(Delta12, (tableIndex == 4 ? 3 : tableIndex) * 12, 12);
                    break;
                default:
                    return false;
            }
            decoderTable = JxrHuffmanTable.CreateFromCodePairs(codeTable);
            return decoderTable != null;
        }

        private static int[] CopySegment(int[] source, int offset, int count)
        {
            int[] result = new int[count];
            int index;
            for (index = 0; index < count; index++) result[index] = source[offset + index];
            return result;
        }
    }
}
