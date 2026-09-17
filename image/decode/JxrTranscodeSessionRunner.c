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
#include "JxrTranscodePlanePair.h"

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
    PixelI alphaMacroblockBuffer[256];
    JxrTranscodeSession session = {0};
    CWMDecoderParameters decoderParameters = {0};
    JxrTranscodeDecoderInitializationResult decoderInitialization;
    JxrTranscodeEncoderInitializationResult encoderInitialization;
    JxrTranscodeRoiInitializationResult roiInitialization;
    JxrTranscodeFrameBufferAllocation frameBuffers;
    JxrTranscodeAlphaPlaneInitializationResult alphaInitialization;
    JxrTranscodeDecoderRuntimeState decoderRuntime;
    JxrTranscodeEncoderOutputInitializationResult encoderOutputInitialization;
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
    Int status = ICERR_ERROR;

    if (inputStream == NULL || outputStream == NULL || parameters == NULL)
        return ICERR_ERROR;
    orientationValue = parameters->oOrientation;
    if (JxrTranscodeSessionFactoryCreateCodec(inputStream,
        &session.decoderCodec) != ICERR_OK)
        goto cleanup;
    if (JxrTranscodeDecoderInitializerInitialize(session.decoderCodec, parameters,
        &decoderParameters, &decoderInitialization) != ICERR_OK)
        goto cleanup;
    orientationValue = decoderInitialization.orientationValue;
    orientation = decoderInitialization.orientation;
    parameters->bIgnoreOverlap = JxrTranscodeSessionRunnerCanUseFastTileExtraction(
        session.decoderCodec, parameters);

    coefficientUnit = decoderInitialization.coefficientUnit;
    if (JxrTranscodeDecoderRuntimeInitializerAllocateMacroblockBuffer(session.decoderCodec,
        coefficientUnit, &decoderRuntime) != ICERR_OK)
        goto cleanup;
    session.macroblockBuffer = decoderRuntime.macroblockBuffer;
    if (JxrTranscodeAlphaPlaneInitializerInitializeDecoder(session.decoderCodec, parameters,
        alphaMacroblockBuffer, &alphaInitialization) != ICERR_OK)
        goto cleanup;
    alphaChannelIndex = alphaInitialization.channelIndex;
    if (JxrTranscodeDecoderRuntimeInitializerInitializePrimaryInput(session.decoderCodec,
        &decoderRuntime) != ICERR_OK)
        goto cleanup;
    session.decoderIoHeader = decoderRuntime.ioHeaderAllocation;
    session.decoderPrimaryResourcesInitialized = TRUE;
    if (JxrTranscodeAlphaPlaneInitializerFinalizeDecoder(session.decoderCodec) != ICERR_OK)
        goto cleanup;

    if (JxrTranscodeEncoderInitializerInitialize(session.decoderCodec, outputStream,
        parameters, &encoderInitialization) != ICERR_OK)
        goto cleanup;
    session.encoderCodec = encoderInitialization.encoderCodec;
    session.encoderIoHeader = encoderInitialization.ioHeaderAllocation;
    if (JxrTranscodeRoiInitializerInitialize(session.decoderCodec, session.encoderCodec, parameters,
        &orientation, &roiInitialization) != ICERR_OK)
        goto cleanup;
    macroblockLeft = roiInitialization.macroblockLeft;
    macroblockRight = roiInitialization.macroblockRight;
    macroblockTop = roiInitialization.macroblockTop;
    macroblockBottom = roiInitialization.macroblockBottom;
    macroblockWidth = roiInitialization.macroblockWidth;
    macroblockHeight = roiInitialization.macroblockHeight;

    if (JxrTranscodeFrameBufferAllocatorAllocate(session.encoderCodec, parameters,
        orientationValue, coefficientUnit, &frameBuffers) != ICERR_OK)
        goto cleanup;
    session.primaryFrameBuffer = frameBuffers.primaryCoefficients;
    session.alphaFrameBuffer = frameBuffers.alphaCoefficients;
    session.primaryFrameMacroblocks = frameBuffers.primaryMacroblocks;
    session.alphaFrameMacroblocks = frameBuffers.alphaMacroblocks;
    if (JxrTranscodeEncoderOutputInitializerInitialize(session.decoderCodec, session.encoderCodec,
        parameters, orientationValue, &orientation,
        &encoderOutputInitialization) != ICERR_OK)
        goto cleanup;
    session.tileQuantizers = encoderOutputInitialization.tileQuantizers;
    session.usedFastTileExtraction = encoderOutputInitialization.usedFastTileExtraction;
    session.encoderOutputInitialized = TRUE;
    if (JxrTranscodeAlphaPlaneInitializerInitializeEncoder(session.decoderCodec,
        session.encoderCodec, parameters) != ICERR_OK)
        goto cleanup;

    {
        JxrTranscodeMacroblockProcessingPipeline pipeline = {0};

        if (!JxrTranscodePlanePairResolveLegacy(&pipeline.decoderPlanes,
            session.decoderCodec, parameters->uAlphaMode > 0) ||
            !JxrTranscodePlanePairResolveLegacy(&pipeline.encoderPlanes,
                session.encoderCodec, parameters->uAlphaMode > 0))
            goto cleanup;
        pipeline.parameters = parameters;
        pipeline.macroblockBuffer = session.macroblockBuffer;
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
        pipeline.tileQuantizers = session.tileQuantizers;
        pipeline.tileQuantizerCount = encoderOutputInitialization.tileQuantizerCount;
        pipeline.primaryFrameBuffer = session.primaryFrameBuffer;
        pipeline.alphaFrameBuffer = session.alphaFrameBuffer;
        pipeline.primaryFrameMacroblocks = session.primaryFrameMacroblocks;
        pipeline.alphaFrameMacroblocks = session.alphaFrameMacroblocks;
        pipeline.usedFastTileExtraction = session.usedFastTileExtraction;
        if (JxrTranscodeMacroblockProcessingPipelineExecute(&pipeline) != ICERR_OK)
            goto cleanup;
    }
    status = ICERR_OK;

cleanup:
    session.hasOrientation = orientationValue != O_NONE;
    session.hasAlphaFrame = parameters->uAlphaMode > 0;
    session.decoderHasAlpha = session.decoderCodec != NULL &&
        session.decoderCodec->m_param.bAlphaChannel;
    JxrTranscodeSessionRelease(&session);
    return status;
}
