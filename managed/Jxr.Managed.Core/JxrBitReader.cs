using System;

namespace Jxr.Managed.Core
{
    // Managed counterpart of image/sys/JxrManagedBitIO.c's JxrBitReader.
    // Input is MSB-first.  byteIndex names the next byte to load, while the
    // accumulator retains the byte currently being consumed.
    public sealed class JxrBitReader
    {
        private readonly byte[] buffer;
        private int byteIndex;
        private uint accumulator;
        private int bufferedBitCount;
        private int bitPosition;
        private bool failed;

        public JxrBitReader(byte[] source)
        {
            if (source == null) throw new ArgumentNullException("source");
            buffer = source;
        }

        // Number of bits consumed, including a partial read that reached EOF.
        public int BitPosition { get { return bitPosition; } }
        public int ByteIndex { get { return byteIndex; } }
        public uint Accumulator { get { return accumulator; } }
        public int BufferedBitCount { get { return bufferedBitCount; } }
        public bool HasFailed { get { return failed; } }
        public int BitsRemaining { get { return bufferedBitCount + (buffer.Length - byteIndex) * 8; } }

        // Non-mutating helper used by the Huffman module.  Native JxrBitReader
        // has only Read; keeping Peek explicit avoids hidden cursor copies.
        public JxrError PeekBits(int count, out uint value)
        {
            int index;
            int bit;
            value = 0;
            if (count < 0 || count > 32) return Fail(JxrError.InvalidArgument);
            if (count > BitsRemaining) return Fail(JxrError.UnexpectedEndOfStream);
            for (index = 0; index < count; index++)
            {
                bit = bitPosition + index;
                value = (value << 1) | (uint)((buffer[bit >> 3] >> (7 - (bit & 7))) & 1);
            }
            return JxrError.None;
        }

        public JxrError ConsumeBits(int count)
        {
            uint ignored;
            if (count < 0 || count > 32) return Fail(JxrError.InvalidArgument);
            if (count > BitsRemaining) return Fail(JxrError.UnexpectedEndOfStream);
            return ReadBits(count, out ignored);
        }

        // Exact state transition of JxrBitReaderRead: it consumes available
        // bits until the requested field completes or a source byte is absent.
        public JxrError ReadBits(int count, out uint value)
        {
            uint result = 0;
            int remaining = count;
            value = 0;
            if (count < 0 || count > 32) return Fail(JxrError.InvalidArgument);

            while (remaining != 0)
            {
                int take;
                if (bufferedBitCount == 0)
                {
                    if (byteIndex == buffer.Length) return Fail(JxrError.UnexpectedEndOfStream);
                    accumulator = buffer[byteIndex++];
                    bufferedBitCount = 8;
                }
                take = remaining < bufferedBitCount ? remaining : bufferedBitCount;
                result = (result << take) |
                    ((accumulator >> (bufferedBitCount - take)) & LowMask(take));
                bufferedBitCount -= take;
                bitPosition += take;
                remaining -= take;
            }
            value = result;
            return JxrError.None;
        }

        public JxrError AlignToByte()
        {
            return ConsumeBits(bufferedBitCount);
        }

        private JxrError Fail(JxrError error)
        {
            failed = true;
            return error;
        }

        private static uint LowMask(int bitCount)
        {
            return (1U << bitCount) - 1U;
        }
    }
}
