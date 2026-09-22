using System;

namespace Jxr.Managed.Core
{
    // Deliberately small, in-memory reader for the first entropy-module port.
    // Packet refill is a later module and is not hidden behind this class.
    public sealed class JxrBitReader
    {
        private readonly byte[] buffer;
        private readonly int bitCount;
        private int bitPosition;

        public JxrBitReader(byte[] source)
        {
            if (source == null) throw new ArgumentNullException("source");
            buffer = source;
            bitCount = source.Length * 8;
        }

        public int BitPosition { get { return bitPosition; } }

        public JxrError PeekBits(int count, out uint value)
        {
            int index;
            int bit;
            value = 0;
            if (count < 0 || count > 32) return JxrError.InvalidArgument;
            if (count > bitCount - bitPosition) return JxrError.UnexpectedEndOfStream;
            for (index = 0; index < count; index++)
            {
                bit = bitPosition + index;
                value = (value << 1) | (uint)((buffer[bit >> 3] >> (7 - (bit & 7))) & 1);
            }
            return JxrError.None;
        }

        public JxrError ConsumeBits(int count)
        {
            if (count < 0 || count > 32) return JxrError.InvalidArgument;
            if (count > bitCount - bitPosition) return JxrError.UnexpectedEndOfStream;
            bitPosition += count;
            return JxrError.None;
        }

        public JxrError ReadBits(int count, out uint value)
        {
            JxrError error = PeekBits(count, out value);
            if (error != JxrError.None) return error;
            return ConsumeBits(count);
        }
    }
}
