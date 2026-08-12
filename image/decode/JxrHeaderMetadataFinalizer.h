#ifndef JXR_HEADER_METADATA_FINALIZER_H
#define JXR_HEADER_METADATA_FINALIZER_H

#include "JxrStreamPositionScope.h"

Bool JxrHeaderMetadataFinalizerApply(U32 headerBytesRead,
    const CCoreParameters* coreParameters, CWMIStrCodecParam* codecParameters);

#endif
