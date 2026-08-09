#ifndef JXR_HP_COEFFICIENT_BLOCK_RESOLVER_H
#define JXR_HP_COEFFICIENT_BLOCK_RESOLVER_H

#include "JxrCoefficientBuffer.h"
#include "JxrDecoderSubbandContext.h"

JxrCoefficientBuffer JxrHpCoefficientBlockResolverResolve(
    const JxrDecoderSubbandContext* state, Int plane, Int block,
    Int subblock, Int coefficientIndex);

#endif
