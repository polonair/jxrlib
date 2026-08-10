#include "JxrHpCoefficientBlockResolver.h"

JxrHpBlockAddress JxrHpCoefficientBlockResolverResolveAddress(COLORFORMAT colorFormat,
    Int plane, Int block, Int subblock, Int coefficientIndex)
{
    JxrHpBlockAddress address;

    address.planeIndex = plane;
    address.coefficientOffset = (size_t)blkOffset[coefficientIndex & 15];
    if (block >= 4) {
        if (colorFormat == YUV_420) {
            address.planeIndex = block - 3;
            address.coefficientOffset = (size_t)blkOffsetUV[subblock];
        }
        else {
            address.planeIndex = 1 + (1 & (block >> 1));
            address.coefficientOffset = (size_t)((block & 1) * 32 + blkOffsetUV_422[subblock]);
        }
    }
    return address;
}

JxrCoefficientBuffer JxrHpCoefficientBlockResolverResolve(
    const JxrDecoderSubbandContext* state, Int plane, Int block,
    Int subblock, Int coefficientIndex)
{
    JxrHpBlockAddress address = JxrHpCoefficientBlockResolverResolveAddress(
        JxrDecoderFormatStateGetColorFormat(&state->formatState), plane, block,
        subblock, coefficientIndex);

    return JxrCoefficientPlaneStateGetBlock(&state->coefficientPlanes,
        address.planeIndex, address.coefficientOffset, 16);
}
