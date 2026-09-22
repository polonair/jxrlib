namespace Jxr.Managed.Core
{
    // Exact managed counterpart of image/decode/JxrBitMath.c.
    // All operations are explicitly UInt32 so the result never depends on
    // native word size or signed shift behaviour.
    public static class JxrBitMath
    {
        public static uint RotateLeft32(uint value, uint count)
        {
            count &= 31U;
            if (count == 0) return value;
            return (value << (int)count) | (value >> (int)(32U - count));
        }

        public static uint LowMask32(uint bitCount)
        {
            if (bitCount == 0) return 0U;
            if (bitCount >= 32U) return 0xffffffffU;
            return (1U << (int)bitCount) - 1U;
        }
    }
}
