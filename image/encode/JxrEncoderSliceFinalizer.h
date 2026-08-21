#ifndef JXR_ENCODER_SLICE_FINALIZER_H
#define JXR_ENCODER_SLICE_FINALIZER_H

#include "strcodec.h"

/* Immutable end-of-slice decisions for one encoded macroblock. */
typedef struct JxrEncoderSliceFinalizationPlan {
    Bool completesHorizontalSlice;
    Bool updatesPacketIndex;
    Bool resetsCodingContexts;
} JxrEncoderSliceFinalizationPlan;

Void JxrEncoderSliceFinalizationPlanInitialize(
    JxrEncoderSliceFinalizationPlan* plan,
    const CWMImageStrCodec* codec,
    Int macroblockX,
    Int macroblockY);

/* Aligns completed packets, records their offsets, and resets contexts when needed. */
Void JxrEncoderSliceFinalizerFinalize(
    CWMImageStrCodec* codec,
    Int macroblockX,
    Int macroblockY);

#endif
