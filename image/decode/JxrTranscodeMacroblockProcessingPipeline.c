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
    capture.destinationTileRow = pipeline->encoderCodec->cTileRow;
    capture.destinationTileColumn = pipeline->encoderCodec->cTileColumn;
    capture.destinationTileColumnCount =
        pipeline->encoderCodec->WMISCP.cNumOfSliceMinus1V + 1;
    capture.storeByDestinationTile = pipeline->orientationValue != O_NONE;
    capture.primaryTile = pipeline->decoderCodec->pTile +
        pipeline->decoderCodec->cTileColumn;
    capture.primaryChannelCount = pipeline->encoderCodec->WMISCP.cChannel;
    capture.subband = pipeline->encoderCodec->WMISCP.sbSubband;
    capture.hasAlpha = pipeline->parameters->uAlphaMode > 0;
    if (capture.hasAlpha) {
        capture.alphaTile = pipeline->decoderCodec->m_pNextSC->pTile +
            pipeline->decoderCodec->cTileColumn;
        capture.alphaChannelIndex = pipeline->alphaChannelIndex;
    }
    return JxrTranscodeTileQuantizerCaptureCapture(&capture) ? ICERR_OK : ICERR_ERROR;
}

static Int JxrTranscodeMacroblockProcessingPipelineStoreOrEncode(
    JxrTranscodeMacroblockProcessingPipeline* pipeline, Int destinationColumn,
    Int destinationRow)
{
    if (pipeline->orientationValue == O_NONE)
        return JxrTranscodeDirectMacroblockEncoderEncode(pipeline->decoderCodec,
            pipeline->encoderCodec, pipeline->macroblockLeft, pipeline->macroblockTop,
            destinationColumn, destinationRow, pipeline->tileQuantizers,
            pipeline->parameters->uAlphaMode > 0);
    else {
        JxrTranscodeOrientedMacroblockBufferRequest bufferRequest = {0};
        size_t macroblockCount = pipeline->encoderCodec->cmbWidth *
            pipeline->encoderCodec->cmbHeight;

        bufferRequest.primaryMacroblock = &pipeline->decoderCodec->MBInfo;
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
        bufferRequest.hasAlpha = pipeline->parameters->uAlphaMode > 0;
        if (bufferRequest.hasAlpha) {
            bufferRequest.alphaMacroblock = &pipeline->decoderCodec->m_pNextSC->MBInfo;
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
    encoder.destinationCodec = pipeline->encoderCodec;
    encoder.sourceAlphaCodec = pipeline->decoderCodec;
    encoder.primaryMacroblocks = pipeline->primaryFrameMacroblocks;
    encoder.primaryCoefficients = pipeline->primaryFrameBuffer;
    encoder.coefficientUnit = pipeline->coefficientUnit;
    encoder.macroblockCount = pipeline->encoderCodec->cmbWidth *
        pipeline->encoderCodec->cmbHeight;
    encoder.destinationCoefficients = pipeline->macroblockBuffer;
    encoder.orientation = pipeline->orientation;
    encoder.tileQuantizers = pipeline->tileQuantizers;
    encoder.tileQuantizerCount = pipeline->tileQuantizerCount;
    encoder.tileColumnCount = pipeline->encoderCodec->WMISCP.cNumOfSliceMinus1V + 1;
    encoder.hasAlpha = pipeline->parameters->uAlphaMode > 0;
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
    if (pipeline == NULL || pipeline->decoderCodec == NULL ||
        pipeline->encoderCodec == NULL || pipeline->parameters == NULL ||
        pipeline->macroblockBuffer == NULL || pipeline->orientation == NULL)
        return ICERR_ERROR;
    if (pipeline->usedFastTileExtraction)
        return JxrTranscodeTileExtractionExecutorExecute(pipeline->decoderCodec,
            pipeline->encoderCodec, pipeline->macroblockLeft, pipeline->macroblockRight,
            pipeline->macroblockTop, pipeline->macroblockBottom);
    if (writeIndexTableNull(pipeline->encoderCodec) != ICERR_OK)
        return ICERR_ERROR;

    for (pipeline->decoderCodec->cRow = 0;
        pipeline->decoderCodec->cRow < pipeline->macroblockBottom;
        pipeline->decoderCodec->cRow++) {
        for (pipeline->decoderCodec->cColumn = 0;
            pipeline->decoderCodec->cColumn < pipeline->decoderCodec->cmbWidth;
            pipeline->decoderCodec->cColumn++) {
            Int destinationRow = (Int)pipeline->decoderCodec->cRow;
            Int destinationColumn = (Int)pipeline->decoderCodec->cColumn;
            JxrTranscodeTileContextRequest contextRequest = {0};
            JxrTranscodeTileContextResult context;

            memset(pipeline->macroblockBuffer, 0, sizeof(PixelI) *
                pipeline->coefficientUnit);
            if (pipeline->decoderCodec->m_param.bAlphaChannel) {
                memset(pipeline->decoderCodec->m_pNextSC->p1MBbuffer[0], 0,
                    sizeof(PixelI) * 256);
                pipeline->decoderCodec->m_pNextSC->cRow =
                    pipeline->decoderCodec->cRow;
                pipeline->decoderCodec->m_pNextSC->cColumn =
                    pipeline->decoderCodec->cColumn;
            }
            if (JxrTranscodeMacroblockDecoderDecode(pipeline->decoderCodec,
                destinationColumn, destinationRow) != ICERR_OK)
                return ICERR_ERROR;

            contextRequest.sourceRow = pipeline->decoderCodec->cRow;
            contextRequest.sourceColumn = pipeline->decoderCodec->cColumn;
            contextRequest.macroblockLeft = pipeline->macroblockLeft;
            contextRequest.macroblockRight = pipeline->macroblockRight;
            contextRequest.macroblockTop = pipeline->macroblockTop;
            contextRequest.macroblockBottom = pipeline->macroblockBottom;
            contextRequest.macroblockWidth = pipeline->macroblockWidth;
            contextRequest.macroblockHeight = pipeline->macroblockHeight;
            contextRequest.tileColumns = pipeline->encoderCodec->WMISCP.uiTileX;
            contextRequest.tileColumnCount =
                pipeline->encoderCodec->WMISCP.cNumOfSliceMinus1V + 1;
            contextRequest.tileRows = pipeline->encoderCodec->WMISCP.uiTileY;
            contextRequest.tileRowCount =
                pipeline->encoderCodec->WMISCP.cNumOfSliceMinus1H + 1;
            contextRequest.orientation = pipeline->orientation;
            if (!JxrTranscodeTileContextResolverResolve(&contextRequest, &context))
                return ICERR_ERROR;
            if (!context.isInsideRoi) continue;

            destinationRow = context.destinationRow;
            destinationColumn = context.destinationColumn;
            pipeline->encoderCodec->m_bCtxLeft = context.isTileColumnStart;
            pipeline->encoderCodec->m_bCtxTop = context.isTileRowStart;
            if (pipeline->encoderCodec->m_bCtxLeft)
                pipeline->encoderCodec->cTileColumn = context.tileColumn;
            if (pipeline->encoderCodec->m_bCtxTop)
                pipeline->encoderCodec->cTileRow = context.tileRow;
            if (pipeline->encoderCodec->m_bCtxLeft &&
                pipeline->encoderCodec->m_bCtxTop &&
                JxrTranscodeMacroblockProcessingPipelineCaptureQuantizers(pipeline) !=
                    ICERR_OK)
                return ICERR_ERROR;
            if (JxrTranscodeMacroblockProcessingPipelineStoreOrEncode(pipeline,
                destinationColumn, destinationRow) != ICERR_OK)
                return ICERR_ERROR;
        }
        advanceOneMBRow(pipeline->decoderCodec);
        if (pipeline->orientationValue == O_NONE)
            advanceOneMBRow(pipeline->encoderCodec);
    }
    return JxrTranscodeMacroblockProcessingPipelineEncodeOriented(pipeline);
}
