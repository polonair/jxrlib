#ifndef JXR_HP_COEFFICIENT_BLOCK_RESOLVER_H
#define JXR_HP_COEFFICIENT_BLOCK_RESOLVER_H

#include "JxrCoefficientBuffer.h"
#include "JxrDecoderSubbandContext.h"

typedef struct JxrHpBlockAddress {
    Int planeIndex;
    size_t coefficientOffset;
} JxrHpBlockAddress;

JxrHpBlockAddress JxrHpCoefficientBlockResolverResolveAddress(COLORFORMAT colorFormat,
    Int plane, Int block, Int subblock, Int coefficientIndex);
JxrCoefficientBuffer JxrHpCoefficientBlockResolverResolve(
    const JxrDecoderSubbandContext* state, Int plane, Int block,
    Int subblock, Int coefficientIndex);

#endif
