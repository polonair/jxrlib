#include "JxrHpCoefficientBlockResolver.h"

JxrCoefficientBuffer JxrHpCoefficientBlockResolverResolve(
    const JxrDecoderSubbandContext* state, Int plane, Int block,
    Int subblock, Int coefficientIndex)
{
    CWMImageStrCodec* codec = state->codec;
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    Int bufferIndex = plane;
    size_t offset = (size_t)blkOffset[coefficientIndex & 15];

    if (block >= 4) {
        if (colorFormat == YUV_420) {
            bufferIndex = block - 3;
            offset = (size_t)blkOffsetUV[subblock];
        }
        else {
            bufferIndex = 1 + (1 & (block >> 1));
            offset = (size_t)((block & 1) * 32 + blkOffsetUV_422[subblock]);
        }
    }

    return JxrCoefficientBufferCreate(codec->p1MBbuffer[bufferIndex], offset, 16);
}
