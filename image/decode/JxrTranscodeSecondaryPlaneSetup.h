#ifndef JXR_TRANSCODE_SECONDARY_PLANE_SETUP_H
#define JXR_TRANSCODE_SECONDARY_PLANE_SETUP_H

#include "strcodec.h"

/* Creates the alpha-plane codec from the explicitly shared primary state. */
Int JxrTranscodeSecondaryPlaneSetupCreate(CWMImageStrCodec* primaryCodec,
    CWMImageStrCodec** secondaryCodec);

#endif
