using System;
using System.Collections.Generic;

namespace Jxr.Managed.Core
{
    // MSB-first counterpart of JxrBitReader. A partial final byte is zero-padded.
    public sealed class JxrBitWriter
    {
        private readonly List<byte> bytes = new List<byte>();
        private int bitCount;

        public int BitCount { get { return bitCount; } }

        public JxrError Write(uint value, int count)
        {
            if (count < 0 || count > 32) return JxrError.InvalidArgument;
            for (int bit = count - 1; bit >= 0; bit--)
            {
                if ((bitCount & 7) == 0) bytes.Add(0);
                if (((value >> bit) & 1U) != 0)
                    bytes[bytes.Count - 1] |= (byte)(1 << (7 - (bitCount & 7)));
                bitCount++;
            }
            return JxrError.None;
        }

        public void AlignByte()
        {
            if ((bitCount & 7) != 0) Write(0, 8 - (bitCount & 7));
        }

        public byte[] ToArray() { return bytes.ToArray(); }
    }
}
