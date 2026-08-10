#include "JxrBitMath.h"

U32 JxrBitMathRotateLeft32(U32 value, U32 count)
{
    count &= 31;
    if (count == 0)
        return value;
    return (value << count) | (value >> (32 - count));
}

U32 JxrBitMathLowMask32(U32 bitCount)
{
    if (bitCount == 0)
        return 0;
    if (bitCount >= 32)
        return 0xffffffffU;
    return ((U32)1 << bitCount) - 1;
}
