#ifndef JXR_TRANSCODE_PLANE_PAIR_H
#define JXR_TRANSCODE_PLANE_PAIR_H

#include "strcodec.h"

/* Managed-port representation of primary and optional alpha codec state. */
typedef struct JxrTranscodePlanePair {
    CWMImageStrCodec* primaryCodec;
    CWMImageStrCodec* alphaCodec;
    Bool hasAlpha;
} JxrTranscodePlanePair;

Bool JxrTranscodePlanePairResolveLegacy(JxrTranscodePlanePair* pair,
    CWMImageStrCodec* primaryCodec, Bool hasAlpha);

#endif
