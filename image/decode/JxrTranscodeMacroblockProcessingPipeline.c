#include "JxrTranscodeMacroblockProcessingPipeline.h"
#include "JxrTranscodeMacroblockDecoder.h"
#include "JxrTranscodeTileContextResolver.h"
#include "JxrTranscodeTileQuantizerCapture.h"
#include "JxrTranscodeDirectMacroblockEncoder.h"
#include "JxrTranscodeOrientedMacroblockBuffer.h"
#include "JxrTranscodeOrientedMacroblockEncoder.h"
#include "JxrTranscodeTileExtractionExecutor.h"

EXTERN_C Int writeIndexTableNull(CWMImageStrCodec*);

static Int JxrTranscodeMacroblockProcessingPipelineCaptureQuantizers(
    JxrTranscodeMacroblockProcessingPipeline* pipeline)
{
    JxrTranscodeTileQuantizerCaptureRequest capture = {0};

    capture.states = pipeline->tileQuantizers;
    capture.stateCount = pipeline->tileQuantizerCount;
    capture.destinationTileRow = pipeline->encoderPlanes.primaryCodec->cTileRow;
    capture.destinationTileColumn = pipeline->encoderPlanes.primaryCodec->cTileColumn;
    capture.destinationTileColumnCount =
        pipeline->encoderPlanes.primaryCodec->WMISCP.cNumOfSliceMinus1V + 1;
    capture.storeByDestinationTile = pipeline->orientationValue != O_NONE;
    capture.primaryTile = pipeline->decoderPlanes.primaryCodec->pTile +
        pipeline->decoderPlanes.primaryCodec->cTileColumn;
    capture.primaryChannelCount = pipeline->encoderPlanes.primaryCodec->WMISCP.cChannel;
    capture.subband = pipeline->encoderPlanes.primaryCodec->WMISCP.sbSubband;
    capture.hasAlpha = pipeline->decoderPlanes.hasAlpha;
    if (capture.hasAlpha) {
        capture.alphaTile = pipeline->decoderPlanes.alphaCodec->pTile +
            pipeline->decoderPlanes.primaryCodec->cTileColumn;
        capture.alphaChannelIndex = pipeline->alphaChannelIndex;
    }
    return JxrTranscodeTileQuantizerCaptureCapture(&capture) ? ICERR_OK : ICERR_ERROR;
}

static Int JxrTranscodeMacroblockProcessingPipelineStoreOrEncode(
    JxrTranscodeMacroblockProcessingPipeline* pipeline, Int destinationColumn,
    Int destinationRow)
{
    if (pipeline->orientationValue == O_NONE)
        return JxrTranscodeDirectMacroblockEncoderEncode(&pipeline->decoderPlanes,
            &pipeline->encoderPlanes, pipeline->macroblockLeft, pipeline->macroblockTop,
            destinationColumn, destinationRow, pipeline->tileQuantizers);
    else {
        JxrTranscodeOrientedMacroblockBufferRequest bufferRequest = {0};
        size_t macroblockCount = pipeline->encoderPlanes.primaryCodec->cmbWidth *
            pipeline->encoderPlanes.primaryCodec->cmbHeight;

        bufferRequest.primaryMacroblock = &pipeline->decoderPlanes.primaryCodec->MBInfo;
        bufferRequest.primaryCoefficients = pipeline->macroblockBuffer;
        bufferRequest.primaryCoefficientCount = pipeline->coefficientUnit;
        bufferRequest.primaryFrameMacroblocks = pipeline->primaryFrameMacroblocks;
        bufferRequest.primaryFrameMacroblockCount = macroblockCount;
        bufferRequest.primaryFrameCoefficients = pipeline->primaryFrameBuffer;
        bufferRequest.primaryFrameCoefficientCount = macroblockCount *
            pipeline->coefficientUnit;
        bufferRequest.destinationRow = destinationRow;
        bufferRequest.destinationColumn = destinationColumn;
        bufferRequest.sourceMacroblockWidth = pipeline->macroblockWidth;
        bufferRequest.sourceMacroblockHeight = pipeline->macroblockHeight;
        bufferRequest.orientation = pipeline->orientation;
        bufferRequest.hasAlpha = pipeline->decoderPlanes.hasAlpha;
        if (bufferRequest.hasAlpha) {
            bufferRequest.alphaMacroblock = &pipeline->decoderPlanes.alphaCodec->MBInfo;
            bufferRequest.alphaCoefficients = pipeline->alphaMacroblockBuffer;
            bufferRequest.alphaCoefficientCount = 256;
            bufferRequest.alphaFrameMacroblocks = pipeline->alphaFrameMacroblocks;
            bufferRequest.alphaFrameMacroblockCount = macroblockCount;
            bufferRequest.alphaFrameCoefficients = pipeline->alphaFrameBuffer;
            bufferRequest.alphaFrameCoefficientCount = macroblockCount * 256;
        }
        return JxrTranscodeOrientedMacroblockBufferStore(&bufferRequest) ?
            ICERR_OK : ICERR_ERROR;
    }
}

static Int JxrTranscodeMacroblockProcessingPipelineEncodeOriented(
    JxrTranscodeMacroblockProcessingPipeline* pipeline)
{
    JxrTranscodeOrientedMacroblockEncoderRequest encoder = {0};

    if (pipeline->orientationValue == O_NONE) return ICERR_OK;
    encoder.destinationCodec = pipeline->encoderPlanes.primaryCodec;
    encoder.sourceAlphaCodec = pipeline->decoderPlanes.alphaCodec;
    encoder.destinationAlphaCodec = pipeline->encoderPlanes.alphaCodec;
    encoder.primaryMacroblocks = pipeline->primaryFrameMacroblocks;
    encoder.primaryCoefficients = pipeline->primaryFrameBuffer;
    encoder.coefficientUnit = pipeline->coefficientUnit;
    encoder.macroblockCount = pipeline->encoderPlanes.primaryCodec->cmbWidth *
        pipeline->encoderPlanes.primaryCodec->cmbHeight;
    encoder.destinationCoefficients = pipeline->macroblockBuffer;
    encoder.orientation = pipeline->orientation;
    encoder.tileQuantizers = pipeline->tileQuantizers;
    encoder.tileQuantizerCount = pipeline->tileQuantizerCount;
    encoder.tileColumnCount = pipeline->encoderPlanes.primaryCodec->WMISCP.cNumOfSliceMinus1V + 1;
    encoder.hasAlpha = pipeline->decoderPlanes.hasAlpha;
    if (encoder.hasAlpha) {
        encoder.alphaMacroblocks = pipeline->alphaFrameMacroblocks;
        encoder.alphaCoefficients = pipeline->alphaFrameBuffer;
        encoder.alphaDestinationCoefficients = pipeline->alphaMacroblockBuffer;
    }
    return JxrTranscodeOrientedMacroblockEncoderEncode(&encoder);
}

Int JxrTranscodeMacroblockProcessingPipelineExecute(
    JxrTranscodeMacroblockProcessingPipeline* pipeline)
{
    if (pipeline == NULL || pipeline->decoderPlanes.primaryCodec == NULL ||
        pipeline->encoderPlanes.primaryCodec == NULL || pipeline->parameters == NULL ||
        pipeline->macroblockBuffer == NULL || pipeline->orientation == NULL)
        return ICERR_ERROR;
    if (pipeline->usedFastTileExtraction)
        return JxrTranscodeTileExtractionExecutorExecute(pipeline->decoderPlanes.primaryCodec,
            pipeline->encoderPlanes.primaryCodec, pipeline->macroblockLeft, pipeline->macroblockRight,
            pipeline->macroblockTop, pipeline->macroblockBottom);
    if (writeIndexTableNull(pipeline->encoderPlanes.primaryCodec) != ICERR_OK)
        return ICERR_ERROR;

    for (pipeline->decoderPlanes.primaryCodec->cRow = 0;
        pipeline->decoderPlanes.primaryCodec->cRow < pipeline->macroblockBottom;
        pipeline->decoderPlanes.primaryCodec->cRow++) {
        for (pipeline->decoderPlanes.primaryCodec->cColumn = 0;
            pipeline->decoderPlanes.primaryCodec->cColumn < pipeline->decoderPlanes.primaryCodec->cmbWidth;
            pipeline->decoderPlanes.primaryCodec->cColumn++) {
            Int destinationRow = (Int)pipeline->decoderPlanes.primaryCodec->cRow;
            Int destinationColumn = (Int)pipeline->decoderPlanes.primaryCodec->cColumn;
            JxrTranscodeTileContextRequest contextRequest = {0};
            JxrTranscodeTileContextResult context;

            memset(pipeline->macroblockBuffer, 0, sizeof(PixelI) *
                pipeline->coefficientUnit);
            if (pipeline->decoderPlanes.hasAlpha) {
                memset(pipeline->decoderPlanes.alphaCodec->p1MBbuffer[0], 0,
                    sizeof(PixelI) * 256);
                pipeline->decoderPlanes.alphaCodec->cRow =
                    pipeline->decoderPlanes.primaryCodec->cRow;
                pipeline->decoderPlanes.alphaCodec->cColumn =
                    pipeline->decoderPlanes.primaryCodec->cColumn;
            }
            if (JxrTranscodeMacroblockDecoderDecode(&pipeline->decoderPlanes,
                destinationColumn, destinationRow) != ICERR_OK)
                return ICERR_ERROR;

            contextRequest.sourceRow = pipeline->decoderPlanes.primaryCodec->cRow;
            contextRequest.sourceColumn = pipeline->decoderPlanes.primaryCodec->cColumn;
            contextRequest.macroblockLeft = pipeline->macroblockLeft;
            contextRequest.macroblockRight = pipeline->macroblockRight;
            contextRequest.macroblockTop = pipeline->macroblockTop;
            contextRequest.macroblockBottom = pipeline->macroblockBottom;
            contextRequest.macroblockWidth = pipeline->macroblockWidth;
            contextRequest.macroblockHeight = pipeline->macroblockHeight;
            contextRequest.tileColumns = pipeline->encoderPlanes.primaryCodec->WMISCP.uiTileX;
            contextRequest.tileColumnCount =
                pipeline->encoderPlanes.primaryCodec->WMISCP.cNumOfSliceMinus1V + 1;
            contextRequest.tileRows = pipeline->encoderPlanes.primaryCodec->WMISCP.uiTileY;
            contextRequest.tileRowCount =
                pipeline->encoderPlanes.primaryCodec->WMISCP.cNumOfSliceMinus1H + 1;
            contextRequest.orientation = pipeline->orientation;
            if (!JxrTranscodeTileContextResolverResolve(&contextRequest, &context))
                return ICERR_ERROR;
            if (!context.isInsideRoi) continue;

            destinationRow = context.destinationRow;
            destinationColumn = context.destinationColumn;
            pipeline->encoderPlanes.primaryCodec->m_bCtxLeft = context.isTileColumnStart;
            pipeline->encoderPlanes.primaryCodec->m_bCtxTop = context.isTileRowStart;
            if (pipeline->encoderPlanes.primaryCodec->m_bCtxLeft)
                pipeline->encoderPlanes.primaryCodec->cTileColumn = context.tileColumn;
            if (pipeline->encoderPlanes.primaryCodec->m_bCtxTop)
                pipeline->encoderPlanes.primaryCodec->cTileRow = context.tileRow;
            if (pipeline->encoderPlanes.primaryCodec->m_bCtxLeft &&
                pipeline->encoderPlanes.primaryCodec->m_bCtxTop &&
                JxrTranscodeMacroblockProcessingPipelineCaptureQuantizers(pipeline) !=
                    ICERR_OK)
                return ICERR_ERROR;
            if (JxrTranscodeMacroblockProcessingPipelineStoreOrEncode(pipeline,
                destinationColumn, destinationRow) != ICERR_OK)
                return ICERR_ERROR;
        }
        advanceOneMBRow(pipeline->decoderPlanes.primaryCodec);
        if (pipeline->orientationValue == O_NONE)
            advanceOneMBRow(pipeline->encoderPlanes.primaryCodec);
    }
    return JxrTranscodeMacroblockProcessingPipelineEncodeOriented(pipeline);
}
