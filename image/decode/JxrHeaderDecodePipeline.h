#ifndef JXR_HEADER_DECODE_PIPELINE_H
#define JXR_HEADER_DECODE_PIPELINE_H

#include "JxrHeaderMetadataFinalizer.h"

Bool JxrHeaderDecodePipelineReadImagePlane(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters,
    SimpleBitIO* bitInput);
Bool JxrHeaderDecodePipelineRead(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters);

#endif
