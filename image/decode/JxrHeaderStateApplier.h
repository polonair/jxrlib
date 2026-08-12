#ifndef JXR_HEADER_STATE_APPLIER_H
#define JXR_HEADER_STATE_APPLIER_H

#include "JxrMainHeaderReader.h"

Bool JxrHeaderStateApplierApplyMain(const JxrMainHeaderDescriptor* source,
    CWMImageInfo* imageInfo, CWMIStrCodecParam* codecParameters,
    CCoreParameters* coreParameters);
Bool JxrHeaderStateApplierApplyImagePlane(const JxrImagePlaneDescriptor* source,
    CWMImageInfo* imageInfo, CWMIStrCodecParam* codecParameters,
    CCoreParameters* coreParameters);
Bool JxrHeaderStateApplierApplyImagePlaneQuantizers(
    const JxrImagePlaneQuantizerHeader* source, CCoreParameters* coreParameters);

#endif
