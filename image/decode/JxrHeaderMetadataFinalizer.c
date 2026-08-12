#include "JxrHeaderMetadataFinalizer.h"

Bool JxrHeaderMetadataFinalizerApply(U32 headerBytesRead,
    const CCoreParameters* coreParameters, CWMIStrCodecParam* codecParameters)
{
    if (coreParameters == NULL || codecParameters == NULL) return FALSE;
    /* Preserve legacy unsigned 32-bit offset semantics for cbStream. */
    codecParameters->cbStream = (U32)0 - headerBytesRead;
    if (!coreParameters->bAlphaChannel) codecParameters->uAlphaMode = 0;
    codecParameters->cChannel = coreParameters->cNumChannels;
    return TRUE;
}
