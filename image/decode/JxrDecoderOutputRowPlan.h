#ifndef JXR_DECODER_OUTPUT_ROW_PLAN_H
#define JXR_DECODER_OUTPUT_ROW_PLAN_H

#include "strcodec.h"

/*
 * Values shared by one standard or thumbnail output macroblock row.
 * The plan has no ownership of codec data and is rebuilt for each row.
 */
typedef struct JxrDecoderOutputRowPlan {
    COLORFORMAT internalColorFormat;
    COLORFORMAT outputColorFormat;
    BITDEPTH_BITS bitDepth;
    size_t outputHeight;
    size_t outputWidth;
    size_t firstRow;
    size_t firstColumn;
    size_t thumbnailScale;
    size_t thumbnailBits;
} JxrDecoderOutputRowPlan;

Void JxrDecoderOutputRowPlanInitializeStandard(JxrDecoderOutputRowPlan* plan,
    const CWMImageStrCodec* codec);
Void JxrDecoderOutputRowPlanInitializeThumbnail(JxrDecoderOutputRowPlan* plan,
    const CWMImageStrCodec* codec);
Void JxrDecoderOutputRowPlanInitializeThumbnailNChannel(JxrDecoderOutputRowPlan* plan,
    const CWMImageStrCodec* codec);

#endif
