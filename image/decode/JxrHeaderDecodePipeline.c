#include "JxrHeaderDecodePipeline.h"

Bool JxrHeaderDecodePipelineReadImagePlane(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters,
    SimpleBitIO* bitInput)
{
    JxrImagePlaneDescriptor descriptor;
    JxrImagePlaneQuantizerHeader quantizers;
    if (imageInfo == NULL || codecParameters == NULL || coreParameters == NULL ||
        bitInput == NULL || !JxrImagePlaneDescriptorReaderRead(bitInput,
        imageInfo->bdBitDepth, &descriptor) || !JxrHeaderStateApplierApplyImagePlane(
        &descriptor, imageInfo, codecParameters, coreParameters) ||
        !JxrImagePlaneQuantizerHeaderReaderRead(bitInput, coreParameters->cNumChannels,
        codecParameters->sbSubband, &quantizers) ||
        !JxrHeaderStateApplierApplyImagePlaneQuantizers(&quantizers, coreParameters) ||
        JxrHeaderValidationValidateImagePlaneQuantizers(coreParameters) != JXR_HEADER_VALID)
        return FALSE;
    flushToByte_SB(bitInput);
    return TRUE;
}

Bool JxrHeaderDecodePipelineRead(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters)
{
    JxrHeaderStreamReader streamReader;
    JxrMainHeaderDescriptor header;
    SimpleBitIO* bitInput;
    U32 headerBytesRead = 0;
    Bool readSucceeded;
    if (imageInfo == NULL || codecParameters == NULL || coreParameters == NULL ||
        !JxrHeaderStreamReaderOpen(&streamReader, codecParameters->pWStream)) return FALSE;
    bitInput = JxrHeaderStreamReaderGetBitInput(&streamReader);
    readSucceeded = JxrMainHeaderReaderRead(bitInput, &header) &&
        JxrHeaderStateApplierApplyMain(&header, imageInfo, codecParameters,
        coreParameters) && JxrHeaderStreamReaderAlignToByte(&streamReader) &&
        JxrHeaderDecodePipelineReadImagePlane(imageInfo, codecParameters,
        coreParameters, bitInput);
    if (!JxrHeaderStreamReaderClose(&streamReader, &headerBytesRead)) return FALSE;
    return readSucceeded && JxrHeaderMetadataFinalizerApply(headerBytesRead,
        coreParameters, codecParameters) && JxrHeaderValidationValidateSourceFormat(
        imageInfo, codecParameters) == JXR_HEADER_VALID;
}
