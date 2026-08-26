#include "JxrSecondaryPlaneFactory.h"
#include "JxrDecoderCodecStateInitializer.h"
#include "JxrSecondaryPlaneMemoryLayoutPlan.h"
#include "JxrSecondaryPlaneBufferRegionLayout.h"
#include <stdlib.h>
#include <string.h>

Int JxrSecondaryPlaneFactoryCreate(const CCoreParameters* parameters,
    const CWMImageStrCodec* templateCodec, size_t channelBytes,
    size_t macroblockCount, CWMImageStrCodec** secondaryCodec)
{
    JxrSecondaryPlaneMemoryLayoutPlan memoryLayout;
    JxrSecondaryPlaneBufferRegionLayout bufferLayout;
    U8* storage;
    CWMImageStrCodec* codec;

    if (secondaryCodec != NULL) *secondaryCodec = NULL;
    if (parameters == NULL || templateCodec == NULL || secondaryCodec == NULL)
        return ICERR_ERROR;

    JxrSecondaryPlaneMemoryLayoutPlanInitialize(&memoryLayout, channelBytes,
        macroblockCount, sizeof(*codec));
    storage = (U8*)malloc(memoryLayout.allocationBytes);
    if (storage == NULL) return WMP_errOutOfMemory;
    memset(storage, 0, memoryLayout.allocationBytes);
    JxrSecondaryPlaneBufferRegionLayoutInitialize(&bufferLayout, (UINTPTR_T)storage,
        sizeof(*codec), &memoryLayout);
    codec = (CWMImageStrCodec*)storage;
    JxrDecoderCodecStateInitializerInitialize(codec, parameters, templateCodec);
    JxrSecondaryPlaneBufferRegionLayoutBind(codec, storage, &bufferLayout,
        memoryLayout.macroblockStride);
    *secondaryCodec = codec;
    return ICERR_OK;
}

Void JxrSecondaryPlaneFactoryRelease(CWMImageStrCodec* secondaryCodec)
{
    free(secondaryCodec);
}
