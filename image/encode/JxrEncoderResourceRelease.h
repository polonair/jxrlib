#ifndef JXR_ENCODER_RESOURCE_RELEASE_H
#define JXR_ENCODER_RESOURCE_RELEASE_H

#include "strcodec.h"

typedef struct JxrEncoderResourceReleasePlan {
    Bool releasesChromaResiduals;
    Bool finalizesPrimaryOutput;
    Bool releasesPredictionState;
    Bool releasesCodingContexts;
    Bool releasesTileState;
} JxrEncoderResourceReleasePlan;

Void JxrEncoderResourceReleasePlanInitialize(JxrEncoderResourceReleasePlan* plan,
    size_t codecIndex, Bool changesUvResolution);
Int JxrEncoderResourceReleaseRelease(CWMImageStrCodec* codec);

#endif
