#ifndef JXR_TRANSCODE_SECONDARY_PLANE_LINK_H
#define JXR_TRANSCODE_SECONDARY_PLANE_LINK_H

#include "strcodec.h"

/* Compatibility link for the legacy primary/secondary codec pointers. */
typedef struct JxrTranscodeSecondaryPlaneLink {
    CWMImageStrCodec* primaryCodec;
    CWMImageStrCodec* secondaryCodec;
} JxrTranscodeSecondaryPlaneLink;

Void JxrTranscodeSecondaryPlaneLinkInitialize(
    JxrTranscodeSecondaryPlaneLink* link);
Bool JxrTranscodeSecondaryPlaneLinkAttach(JxrTranscodeSecondaryPlaneLink* link,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec);
Void JxrTranscodeSecondaryPlaneLinkDetachAndRelease(
    JxrTranscodeSecondaryPlaneLink* link);
Void JxrTranscodeSecondaryPlaneLinkReleaseAttached(CWMImageStrCodec* primaryCodec);

#endif
