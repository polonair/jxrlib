#include "JxrTranscodeSessionRunner.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrTranscodeTileQuantizerState.h"
#include "JxrTranscodeOrientationState.h"
#include "JxrTranscodeSession.h"
#include "JxrTranscodeSessionFactory.h"
#include "JxrTranscodeDecoderInitializer.h"
#include "JxrTranscodeEncoderInitializer.h"
#include "JxrTranscodeRoiInitializer.h"
#include "JxrTranscodeFrameBufferAllocator.h"
#include "JxrTranscodeAlphaPlaneInitializer.h"
#include "JxrTranscodeDecoderRuntimeInitializer.h"
#include "JxrTranscodeEncoderOutputInitializer.h"
#include "JxrTranscodeMacroblockProcessingPipeline.h"
#include "JxrTranscodeTileExtractionDecision.h"

static Bool JxrTranscodeSessionRunnerCanUseFastTileExtraction(
    CWMImageStrCodec* decoderCodec, CWMTranscodingParam* parameters)
{
    JxrTranscodeTileExtractionDecision decision = {0};
    Bool canUseFastPath;

    decision.tileColumns = decoderCodec->WMISCP.uiTileX;
    decision.tileColumnCount = decoderCodec->WMISCP.cNumOfSliceMinus1V + 1;
    decision.macroblockWidth = (U32)decoderCodec->cmbWidth;
    decision.tileRows = decoderCodec->WMISCP.uiTileY;
    decision.tileRowCount = decoderCodec->WMISCP.cNumOfSliceMinus1H + 1;
    decision.macroblockHeight = (U32)decoderCodec->cmbHeight;
    decision.roiLeftPixels = parameters->cLeftX;
    decision.roiTopPixels = parameters->cTopY;
    decision.roiWidthPixels = parameters->cWidth;
    decision.roiHeightPixels = parameters->cHeight;
    decision.extraLeftPixels = decoderCodec->m_param.cExtraPixelsLeft;
    decision.extraTopPixels = decoderCodec->m_param.cExtraPixelsTop;
    decision.sourceOverlap = decoderCodec->WMISCP.olOverlap;
    decision.ignoreOverlap = parameters->bIgnoreOverlap;
    decision.hasTransform = parameters->oOrientation != O_NONE;
    decision.sourceLayout = decoderCodec->WMISCP.bfBitstreamFormat;
    decision.targetLayout = parameters->bfBitstreamFormat;
    decision.sourceSubband = decoderCodec->WMISCP.sbSubband;
    decision.targetSubband = parameters->sbSubband;
    canUseFastPath = JxrTranscodeTileExtractionDecisionCanUseFastPath(&decision);
    parameters->bIgnoreOverlap = decision.ignoreOverlap;
    return canUseFastPath;
}

Int JxrTranscodeSessionRunnerRun(struct WMPStream* inputStream,
    struct WMPStream* outputStream, CWMTranscodingParam* parameters)
{
    PixelI* macroblockBuffer;
    PixelI alphaMacroblockBuffer[256];
    PixelI* primaryFrameBuffer = NULL;
    PixelI* alphaFrameBuffer = NULL;
    CWMIMBInfo* primaryFrameMacroblocks = NULL;
    CWMIMBInfo* alphaFrameMacroblocks = NULL;
    CWMImageStrCodec* decoderCodec;
    CWMImageStrCodec* encoderCodec;
    CWMDecoderParameters decoderParameters = {0};
    JxrTranscodeDecoderInitializationResult decoderInitialization;
    JxrTranscodeEncoderInitializationResult encoderInitialization;
    JxrTranscodeRoiInitializationResult roiInitialization;
    JxrTranscodeFrameBufferAllocation frameBuffers;
    JxrTranscodeAlphaPlaneInitializationResult alphaInitialization;
    JxrTranscodeDecoderRuntimeState decoderRuntime;
    JxrTranscodeEncoderOutputInitializationResult encoderOutputInitialization;
    U8* decoderIoHeader;
    U8* encoderIoHeader;
    JxrTranscodeTileQuantizerState* tileQuantizers = NULL;
    ORIENTATION orientationValue;
    JxrTranscodeOrientationState orientation;
    size_t alphaChannelIndex = 0;
    size_t coefficientUnit;
    size_t macroblockLeft;
    size_t macroblockRight;
    size_t macroblockTop;
    size_t macroblockBottom;
    size_t macroblockWidth;
    size_t macroblockHeight;

    if (inputStream == NULL || outputStream == NULL || parameters == NULL)
        return ICERR_ERROR;
    orientationValue = parameters->oOrientation;
    if (JxrTranscodeSessionFactoryCreateCodec(inputStream, &decoderCodec) != ICERR_OK ||
        JxrTranscodeDecoderInitializerInitialize(decoderCodec, parameters,
            &decoderParameters, &decoderInitialization) != ICERR_OK)
        return ICERR_ERROR;
    orientationValue = decoderInitialization.orientationValue;
    orientation = decoderInitialization.orientation;
    parameters->bIgnoreOverlap = JxrTranscodeSessionRunnerCanUseFastTileExtraction(
        decoderCodec, parameters);

    coefficientUnit = decoderInitialization.coefficientUnit;
    if (JxrTranscodeDecoderRuntimeInitializerAllocateMacroblockBuffer(decoderCodec,
        coefficientUnit, &decoderRuntime) != ICERR_OK)
        return ICERR_ERROR;
    macroblockBuffer = decoderRuntime.macroblockBuffer;
    if (JxrTranscodeAlphaPlaneInitializerInitializeDecoder(decoderCodec, parameters,
        alphaMacroblockBuffer, &alphaInitialization) != ICERR_OK)
        return ICERR_ERROR;
    alphaChannelIndex = alphaInitialization.channelIndex;
    if (JxrTranscodeDecoderRuntimeInitializerInitializePrimaryInput(decoderCodec,
        &decoderRuntime) != ICERR_OK ||
        JxrTranscodeAlphaPlaneInitializerFinalizeDecoder(decoderCodec) != ICERR_OK)
        return ICERR_ERROR;
    decoderIoHeader = decoderRuntime.ioHeaderAllocation;

    if (JxrTranscodeEncoderInitializerInitialize(decoderCodec, outputStream,
        parameters, &encoderInitialization) != ICERR_OK)
        return ICERR_ERROR;
    encoderCodec = encoderInitialization.encoderCodec;
    encoderIoHeader = encoderInitialization.ioHeaderAllocation;
    if (JxrTranscodeRoiInitializerInitialize(decoderCodec, encoderCodec, parameters,
        &orientation, &roiInitialization) != ICERR_OK)
        return ICERR_ERROR;
    macroblockLeft = roiInitialization.macroblockLeft;
    macroblockRight = roiInitialization.macroblockRight;
    macroblockTop = roiInitialization.macroblockTop;
    macroblockBottom = roiInitialization.macroblockBottom;
    macroblockWidth = roiInitialization.macroblockWidth;
    macroblockHeight = roiInitialization.macroblockHeight;

    if (JxrTranscodeFrameBufferAllocatorAllocate(encoderCodec, parameters,
        orientationValue, coefficientUnit, &frameBuffers) != ICERR_OK)
        return ICERR_ERROR;
    primaryFrameBuffer = frameBuffers.primaryCoefficients;
    alphaFrameBuffer = frameBuffers.alphaCoefficients;
    primaryFrameMacroblocks = frameBuffers.primaryMacroblocks;
    alphaFrameMacroblocks = frameBuffers.alphaMacroblocks;
    if (JxrTranscodeEncoderOutputInitializerInitialize(decoderCodec, encoderCodec,
        parameters, orientationValue, &orientation,
        &encoderOutputInitialization) != ICERR_OK)
        return ICERR_ERROR;
    tileQuantizers = encoderOutputInitialization.tileQuantizers;
    if (JxrTranscodeAlphaPlaneInitializerInitializeEncoder(decoderCodec,
        encoderCodec, parameters) != ICERR_OK)
        return ICERR_ERROR;

    {
        JxrTranscodeMacroblockProcessingPipeline pipeline = {0};

        pipeline.decoderCodec = decoderCodec;
        pipeline.encoderCodec = encoderCodec;
        pipeline.parameters = parameters;
        pipeline.macroblockBuffer = macroblockBuffer;
        pipeline.alphaMacroblockBuffer = alphaMacroblockBuffer;
        pipeline.coefficientUnit = coefficientUnit;
        pipeline.alphaChannelIndex = alphaChannelIndex;
        pipeline.macroblockLeft = macroblockLeft;
        pipeline.macroblockRight = macroblockRight;
        pipeline.macroblockTop = macroblockTop;
        pipeline.macroblockBottom = macroblockBottom;
        pipeline.macroblockWidth = macroblockWidth;
        pipeline.macroblockHeight = macroblockHeight;
        pipeline.orientationValue = orientationValue;
        pipeline.orientation = &orientation;
        pipeline.tileQuantizers = tileQuantizers;
        pipeline.tileQuantizerCount = encoderOutputInitialization.tileQuantizerCount;
        pipeline.primaryFrameBuffer = primaryFrameBuffer;
        pipeline.alphaFrameBuffer = alphaFrameBuffer;
        pipeline.primaryFrameMacroblocks = primaryFrameMacroblocks;
        pipeline.alphaFrameMacroblocks = alphaFrameMacroblocks;
        pipeline.usedFastTileExtraction =
            encoderOutputInitialization.usedFastTileExtraction;
        if (JxrTranscodeMacroblockProcessingPipelineExecute(&pipeline) != ICERR_OK)
            return ICERR_ERROR;
    }

    {
        JxrTranscodeSession session = {0};

        session.macroblockBuffer = macroblockBuffer;
        session.primaryFrameBuffer = primaryFrameBuffer;
        session.alphaFrameBuffer = alphaFrameBuffer;
        session.primaryFrameMacroblocks = primaryFrameMacroblocks;
        session.alphaFrameMacroblocks = alphaFrameMacroblocks;
        session.decoderCodec = decoderCodec;
        session.encoderCodec = encoderCodec;
        session.decoderIoHeader = decoderIoHeader;
        session.encoderIoHeader = encoderIoHeader;
        session.tileQuantizers = tileQuantizers;
        session.hasOrientation = orientationValue != O_NONE;
        session.hasAlphaFrame = parameters->uAlphaMode > 0;
        session.decoderHasAlpha = decoderCodec->m_param.bAlphaChannel;
        session.usedFastTileExtraction =
            encoderOutputInitialization.usedFastTileExtraction;
        JxrTranscodeSessionRelease(&session);
    }
    return ICERR_OK;
}
