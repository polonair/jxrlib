/* Self-contained conformance runner for the managed-port reference profile. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "JxrManagedBitIO.h"
#include "strcodec.h"
#include "strTransform.h"
#include "JxrTransformMath.h"
#include "../image/encode/JxrForwardTransformMath.h"
#include "../image/encode/JxrForwardTransformStages.h"
#include "../image/encode/JxrForwardHardTileBoundaryState.h"
#include "../image/encode/JxrForwardTransformMacroblockGeometry.h"
#include "../image/encode/JxrForwardTransformBoundaryContext.h"
#include "../image/encode/JxrForwardTransformFullResolutionPlane.h"
#include "../image/encode/JxrForwardTransformChroma420Plane.h"
#include "../image/encode/JxrForwardTransformChroma422Plane.h"
#include "../image/encode/JxrForwardTransformCodecSetup.h"
#include "../image/encode/JxrForwardTransformPlanePlan.h"
#include "../image/encode/JxrForwardTransformPlaneContext.h"
#include "../image/encode/JxrEncoderMacroblockProcessor.h"
#include "../image/encode/JxrEncoderSubbandPipeline.h"
#include "../image/encode/JxrEncoderPacketHeaderWriter.h"
#include "../image/encode/JxrEncoderSliceFinalizer.h"
#include "../image/encode/JxrEncoderTileHeaderWriter.h"
#include "../image/encode/JxrEncoderImagePlaneHeaderWriter.h"
#include "../image/encode/JxrEncoderMainHeaderWriter.h"
#include "../image/encode/JxrEncoderIndexTableWriter.h"
#include "../image/encode/JxrEncoderPacketStreamAssembler.h"
#include "encode.h"
#include "decode.h"
#include "JxrEntropyState.h"
#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrHuffmanDecoder.h"
#include "JxrLpResidualDecoder.h"
#include "JxrLowpassCbpState.h"
#include "JxrMacroblockCbpState.h"
#include "JxrAdaptiveModelState.h"
#include "JxrHuffmanStateSet.h"
#include "JxrAdaptiveScanState.h"
#include "JxrHighpassCbpState.h"
#include "JxrMacroblockState.h"
#include "JxrCoefficientPlaneState.h"
#include "JxrBitInputBufferState.h"
#include "JxrBitCursorState.h"
#include "JxrBitReaderCore.h"
#include "JxrLegacyBitIoBridge.h"
#include "JxrPacketExecutor.h"
#include "JxrBitMath.h"
#include "JxrDecoderFormatState.h"
#include "JxrDecoderSubbandContext.h"
#include "JxrHpCoefficientBlockResolver.h"
#include "JxrMacroblockRegionState.h"
#include "JxrTranscodeTileQuantizerState.h"
#include "JxrTranscodeQuantizerWriter.h"
#include "JxrTranscodeTileHeaderWriter.h"
#include "JxrTranscodeOrientationState.h"
#include "JxrTranscodeCoefficientTransform.h"
#include "JxrTranscodeTileExtractionDecision.h"
#include "JxrTranscodeRoiGeometry.h"
#include "JxrDecoderTileQuantizerSyntaxReader.h"
#include "JxrDecoderDcQuantizerHeaderApplier.h"
#include "JxrDecoderLpQuantizerHeaderApplier.h"
#include "JxrDecoderHpQuantizerHeaderApplier.h"
#include "JxrDecoderTileHeaderReader.h"
#include "JxrDecoderCodingContextResetter.h"
#include "JxrDecoderPacketRowReader.h"
#include "JxrImagePlaneQuantizerHeaderReader.h"
#include "JxrImagePlaneDescriptorReader.h"
#include "JxrMainHeaderReader.h"
#include "JxrHeaderStateApplier.h"
#include "JxrHeaderValidation.h"
#include "JxrHeaderStreamReader.h"
#include "JxrStreamPositionScope.h"
#include "JxrHeaderMetadataFinalizer.h"
#include "JxrHeaderDecodePipeline.h"
#include "JxrDecoderInitializationPipeline.h"
#include "JxrPostProcessDecision.h"
#include "JxrPostProcessRowState.h"
#include "JxrPostProcessBlockNeighborhood.h"
#include "JxrPostProcessSmoothing.h"
#include "JxrPostProcessMacroblockAnalyzer.h"
#include "JxrPostProcessBlockDcCollector.h"
#include "JxrPostProcessMacroblockNeighborhood.h"
#include "JxrPostProcessBlockEdgeApplier.h"
#include "JxrSecondaryPlaneInitializer.h"
#include "JxrPredictionMath.h"
#include "JxrInverseTransformMath.h"
#include "JxrInverseTransformMacroblockGeometry.h"
#include "JxrHardTileBoundaryState.h"
#include "JxrInverseTransformBoundaryContext.h"
#include "JxrInversePostProcessParameters.h"
#include "JxrInverseHighPassParameters.h"
#include "JxrInverseTransformPlanePlan.h"
#include "JxrInverseTransformPlaneBuffers.h"
#include "JxrInverseTransformPlaneContext.h"
#include "JxrInverseTransformPlaneStage2.h"
#include "JxrInverseColorTransform.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
#include "JxrMonochromeExpansion.h"
#include "JxrDecoderRoiRowRange.h"
#include "JxrVariableLengthWordReader.h"
#include "JxrIndexTableReader.h"
#include "JxrDecoderStreamInitializer.h"
#include "JxrDecoderBitstreamSet.h"
#include "JxrDecoderPacketAttachment.h"
#include "JxrDecoderPacketHeaderReader.h"
#include "JxrPacketHeaderSyntaxReader.h"
#include "JxrEntropyReader.h"
#include "JxrLegacyBitReaderAdapter.h"
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

typedef int (*JxrTest)(void);
typedef struct { const char* name; JxrTest run; } JxrTestCase;

static int file_contains(const char* path, const char* text)
{
    FILE* f = fopen(path, "rb");
    char data[4096]; size_t n;
    if (!f) return 0;
    n = fread(data, 1, sizeof(data) - 1, f); fclose(f);
    data[n] = 0;
    return strstr(data, text) != NULL;
}

static int files_equal(const char* left, const char* right)
{
    FILE* a = fopen(left, "rb"), *b = fopen(right, "rb");
    int ca, cb;
    if (!a || !b) { if (a) fclose(a); if (b) fclose(b); return 0; }
    do { ca = fgetc(a); cb = fgetc(b); } while (ca == cb && ca != EOF);
    fclose(a); fclose(b); return ca == cb;
}

static int test_smoke(void) { return 1; }

static int test_bit_io_vectors(void)
{
    U8 data[8] = {0}; U32 value; JxrBitWriter writer; JxrBitReader reader;
    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, 5, 3) || !JxrBitWriterWrite(&writer, 17, 5) ||
        !JxrBitWriterWrite(&writer, 0xabcd, 16) || !JxrBitWriterWrite(&writer, 15, 4) ||
        !JxrBitWriterFlush(&writer) || JxrBitWriterBytes(&writer) != 4 ||
        data[0] != 0xb1 || data[1] != 0xab || data[2] != 0xcd || data[3] != 0xf0) return 0;
    JxrBitReaderInit(&reader, data, 4);
    return JxrBitReaderRead(&reader, 3, &value) && value == 5 &&
        JxrBitReaderRead(&reader, 5, &value) && value == 17 &&
        JxrBitReaderRead(&reader, 16, &value) && value == 0xabcd &&
        JxrBitReaderRead(&reader, 4, &value) && value == 15 &&
        !JxrBitReaderRead(&reader, 16, &value);
}

static int test_adaptive_state(void)
{
    CCodingContext context; Int mean[2] = { 0, 0 };
    memset(&context, 0, sizeof(context)); ResetCodingContext(&context); InitZigzagScan(&context);
    if (context.m_aModelDC.m_iFlcBits[0] != 8 || context.m_aModelLP.m_iFlcBits[0] != 4 ||
        context.m_aModelAC.m_iFlcBits[0] != 0 || context.m_iCBPCountZero != 1 ||
        context.m_aScanLowpass[0].uScan != 0 || context.m_aScanLowpass[1].uScan != 1 ||
        context.m_aScanHoriz[0].uScan != 0 || context.m_aScanVert[0].uScan != 0) return 0;
    UpdateModelMB(Y_ONLY, 1, mean, &context.m_aModelDC);
    UpdateModelMB(Y_ONLY, 1, mean, &context.m_aModelLP);
    return context.m_aModelDC.m_iFlcBits[0] == 7 && context.m_aModelLP.m_iFlcBits[0] == 3;
}

static int test_adaptive_model_state_vectors(void)
{
    CCodingContext expected;
    CCodingContext actual;
    Int expectedMeans[2] = { 3, 7 };
    Int actualMeans[2] = { 3, 7 };
    JxrAdaptiveModelState state;

    memset(&expected, 0, sizeof(expected));
    memset(&actual, 0, sizeof(actual));
    ResetCodingContext(&expected);
    ResetCodingContext(&actual);
    JxrAdaptiveModelStateInit(&state, &actual.m_aModelLP);
    if (JxrAdaptiveModelStateGetFlcBits(&state, 0) != expected.m_aModelLP.m_iFlcBits[0] ||
        JxrAdaptiveModelStateGetFlcBits(&state, 1) != expected.m_aModelLP.m_iFlcBits[1]) return 0;
    UpdateModelMB(YUV_444, 3, expectedMeans, &expected.m_aModelLP);
    JxrAdaptiveModelStateUpdateForMacroblock(&state, YUV_444, 3, actualMeans);
    return memcmp(&actual.m_aModelLP, &expected.m_aModelLP, sizeof(CAdaptiveModel)) == 0;
}

static int test_huffman_state_set_vectors(void)
{
    CAdaptiveHuffman huffman;
    CAdaptiveHuffman* states[8] = { 0 };
    Int delta[2] = { 3, 7 };
    JxrHuffmanStateSet stateSet;

    memset(&huffman, 0, sizeof(huffman));
    huffman.m_iDiscriminant = 5;
    huffman.m_pDelta = delta;
    states[3] = &huffman;
    JxrHuffmanStateSetInit(&stateSet, states);
    if (JxrHuffmanStateSetGet(&stateSet, 3) != &huffman) return 0;
    JxrHuffmanStateSetObserve(&stateSet, 3, 1);
    return huffman.m_iDiscriminant == 12;
}

static int test_highpass_cbp_state_vectors(void)
{
    CAdaptiveHuffman pattern;
    CAdaptiveHuffman count;
    CCBPModel model;
    JxrHighpassCbpState state;

    memset(&pattern, 0, sizeof(pattern));
    memset(&count, 0, sizeof(count));
    memset(&model, 0, sizeof(model));
    JxrHighpassCbpStateInit(&state, &pattern, &count, &model);
    return JxrHighpassCbpStateGetPatternHuffman(&state) == &pattern &&
        JxrHighpassCbpStateGetCountHuffman(&state) == &count &&
        JxrHighpassCbpStateGetPredictionModel(&state) == &model;
}

static int test_macroblock_state_vectors(void)
{
    CWMIMBInfo macroblock;
    JxrMacroblockState state;

    memset(&macroblock, 0xff, sizeof(macroblock));
    macroblock.iOrientation = 1;
    JxrMacroblockStateInit(&state, &macroblock);
    JxrMacroblockStateClearDc(&state, 2);
    if (JxrMacroblockStateGetDcCoefficients(&state, 0)[0] != 0 ||
        JxrMacroblockStateGetDcCoefficients(&state, 1)[15] != 0 ||
        JxrMacroblockStateGetDcCoefficients(&state, 2)[0] != -1) return 0;
    JxrMacroblockStateSetDcCoefficient(&state, 1, 5, 42);
    if (JxrMacroblockStateGetDcCoefficient(&state, 1, 5) != 42) return 0;
    JxrMacroblockStateResetQuantizerIndices(&state);
    JxrMacroblockStateSetLowpassQuantizerIndex(&state, 3);
    JxrMacroblockStateSetHighpassQuantizerIndex(&state, 7);
    if (JxrMacroblockStateGetLowpassQuantizerIndex(&state) != 3 ||
        JxrMacroblockStateGetHighpassQuantizerIndex(&state) != 7 ||
        JxrMacroblockStateGetOrientation(&state) != 1 || macroblock.iQIndexLP == 3)
        return 0;
    JxrMacroblockStateCommitToNative(&state);
    if (macroblock.iBlockDC[0][0] != 0 || macroblock.iBlockDC[1][5] != 42 ||
        macroblock.iQIndexLP != 3 || macroblock.iQIndexHP != 7 ||
        JxrMacroblockStateGetOrientation(&state) != 1) return 0;
    macroblock.iBlockDC[0][0] = 7;
    macroblock.iQIndexLP = 2;
    macroblock.iQIndexHP = 4;
    macroblock.iOrientation = 9;
    JxrMacroblockStateLoadFromNative(&state);
    return JxrMacroblockStateGetDcCoefficient(&state, 0, 0) == 7 &&
        JxrMacroblockStateGetLowpassQuantizerIndex(&state) == 2 &&
        JxrMacroblockStateGetHighpassQuantizerIndex(&state) == 4 &&
        JxrMacroblockStateGetOrientation(&state) == 9;
}

static int test_coefficient_plane_state_vectors(void)
{
    PixelI plane0[32], plane1[32];
    PixelI* planes[2] = { plane0, plane1 };
    JxrCoefficientPlaneState state;
    JxrCoefficientBuffer block;

    JxrCoefficientPlaneStateInit(&state, planes, YUV_444, 2);
    block = JxrCoefficientPlaneStateGetBlock(&state, 1, 4, 16);
    return JxrCoefficientPlaneStateGetPlane(&state, 0) == plane0 &&
        JxrCoefficientPlaneStateGetLength(&state, 0) == 256 &&
        JxrCoefficientPlaneStateGetLength(&state, 1) == 256 &&
        block.values == plane1 && block.offset == 4 && block.count == 16;
}

static int test_macroblock_region_state_vectors(void)
{
    JxrMacroblockRegionState state;

    memset(&state, 0, sizeof(state));
    state.macroblockX = 4;
    state.macroblockY = 2;
    state.tileLeftMacroblock = 4;
    state.tileTopMacroblock = 0;
    state.tileRightMacroblock = 8;
    state.tileBottomMacroblock = 4;
    state.roiLeftPixels = 70;
    state.roiTopPixels = 20;
    state.roiRightPixels = 100;
    state.roiBottomPixels = 50;
    if (!JxrMacroblockRegionStateIsTileStart(&state) ||
        !JxrMacroblockRegionStateIntersectsEntropyRoi(&state, OL_NONE) ||
        !JxrMacroblockRegionStateShouldTransform(&state, FALSE) ||
        !JxrMacroblockRegionStateShouldTransform(&state, TRUE)) return 0;

    state.roiLeftPixels = 128;
    if (JxrMacroblockRegionStateIntersectsEntropyRoi(&state, OL_NONE) ||
        !JxrMacroblockRegionStateIntersectsEntropyRoi(&state, OL_ONE)) return 0;

    state.macroblockX = 10;
    state.roiLeftPixels = 0;
    state.roiRightPixels = 100;
    return !JxrMacroblockRegionStateIsTileStart(&state) &&
        !JxrMacroblockRegionStateShouldTransform(&state, FALSE);
}

static int test_transcode_tile_quantizer_state_vectors(void)
{
    CWMITile primaryTile;
    CWMITile alphaTile;
    CWMIQuantizer primaryDc[MAX_CHANNELS][1] = {{{0}}};
    CWMIQuantizer primaryLp[MAX_CHANNELS][2] = {{{0}}};
    CWMIQuantizer primaryHp[MAX_CHANNELS][2] = {{{0}}};
    CWMIQuantizer alphaDc[1] = {{0}};
    CWMIQuantizer alphaLp[2] = {{0}};
    CWMIQuantizer alphaHp[2] = {{0}};
    JxrTranscodeTileQuantizerState state;
    size_t channel;

    memset(&primaryTile, 0, sizeof(primaryTile));
    memset(&alphaTile, 0, sizeof(alphaTile));
    for (channel = 0; channel < MAX_CHANNELS; ++channel) {
        primaryTile.pQuantizerDC[channel] = primaryDc[channel];
        primaryTile.pQuantizerLP[channel] = primaryLp[channel];
        primaryTile.pQuantizerHP[channel] = primaryHp[channel];
        primaryDc[channel][0].iIndex = (U8)(10 + channel);
        primaryLp[channel][0].iIndex = (U8)(20 + channel);
        primaryLp[channel][1].iIndex = (U8)(30 + channel);
        primaryHp[channel][0].iIndex = (U8)(40 + channel);
        primaryHp[channel][1].iIndex = (U8)(50 + channel);
    }
    primaryTile.cChModeDC = 2;
    primaryTile.cNumQPLP = 2;
    primaryTile.cNumQPHP = 2;
    primaryTile.cChModeLP[0] = 1;
    primaryTile.cChModeLP[1] = 2;
    primaryTile.cChModeHP[0] = 2;
    primaryTile.cChModeHP[1] = 1;
    primaryTile.bUseDC = FALSE;
    primaryTile.bUseLP = FALSE;

    alphaTile.pQuantizerDC[0] = alphaDc;
    alphaTile.pQuantizerLP[0] = alphaLp;
    alphaTile.pQuantizerHP[0] = alphaHp;
    alphaDc[0].iIndex = 60;
    alphaLp[0].iIndex = 61;
    alphaLp[1].iIndex = 62;
    alphaHp[0].iIndex = 63;
    alphaHp[1].iIndex = 64;
    alphaTile.cNumQPLP = 2;
    alphaTile.cNumQPHP = 2;
    alphaTile.bUseDC = TRUE;
    alphaTile.bUseLP = TRUE;

    JxrTranscodeTileQuantizerStateInit(&state);
    JxrTranscodeTileQuantizerStateCapturePrimary(&state, &primaryTile, 3, SB_ALL);
    JxrTranscodeTileQuantizerStateCaptureAlpha(&state, &alphaTile, 3, SB_ALL);
    return state.dcMode == 2 && state.dcIndex[0] == 10 && state.dcIndex[2] == 12 &&
        state.dcIndex[3] == 60 && !state.useDcForLowpass &&
        state.lowpassQuantizerCount == 2 && state.lowpassMode[1] == 2 &&
        state.lowpassIndex[1][2] == 32 && state.lowpassIndex[1][3] == 0 &&
        !state.useLowpassForHighpass && state.highpassQuantizerCount == 2 &&
        state.highpassMode[0] == 2 && state.highpassIndex[0][1] == 41 &&
        state.highpassIndex[1][3] == 0 && state.useDcForLowpassAlpha &&
        !state.useLowpassForHighpassAlpha && state.lowpassQuantizerCountAlpha == 2 &&
        state.highpassQuantizerCountAlpha == 0;
}

static Bool write_transcode_test_bits(Void* context, U32 value, U32 count)
{
    return JxrBitWriterWrite((JxrBitWriter*)context, value, count);
}

static int test_transcode_quantizer_writer_vectors(void)
{
    U8 data[8] = {0};
    U8 indices[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS] = {{0}};
    U8 modes[JXR_TRANSCODE_MAX_QUANTIZERS] = {0};
    JxrBitWriter writer;
    JxrTranscodeBitSink sink;

    indices[0][0] = 0x12;
    indices[0][1] = 0x34;
    indices[0][2] = 0x56;
    indices[1][0] = 0x78;
    indices[1][1] = 0x9a;
    indices[1][2] = 0xbc;
    modes[0] = 2;
    modes[1] = 1;
    JxrBitWriterInit(&writer, data, sizeof(data));
    JxrTranscodeBitSinkInit(&sink, &writer, write_transcode_test_bits);
    if (!JxrTranscodeQuantizerWriterWriteQuantizer(&sink, indices[0], 2, 3) ||
        !JxrBitWriterFlush(&writer) || JxrBitWriterBytes(&writer) != 4 ||
        data[0] != 0x84 || data[1] != 0x8d || data[2] != 0x15 || data[3] != 0x80) return 0;

    memset(data, 0, sizeof(data));
    JxrBitWriterInit(&writer, data, sizeof(data));
    JxrTranscodeBitSinkInit(&sink, &writer, write_transcode_test_bits);
    if (!JxrTranscodeQuantizerWriterWriteQuantizers(&sink, indices, modes, 2, 3, FALSE) ||
        !JxrBitWriterFlush(&writer) || JxrBitWriterBytes(&writer) != 7 ||
        data[0] != 0x0c || data[1] != 0x24 || data[2] != 0x68 || data[3] != 0xac ||
        data[4] != 0xbc || data[5] != 0x4d || data[6] != 0x00) return 0;

    memset(data, 0, sizeof(data));
    indices[0][3] = 0xaa;
    indices[1][3] = 0xbb;
    JxrBitWriterInit(&writer, data, sizeof(data));
    JxrTranscodeBitSinkInit(&sink, &writer, write_transcode_test_bits);
    return JxrTranscodeQuantizerWriterWriteAlphaQuantizers(&sink, indices, 2, 3, FALSE) &&
        JxrBitWriterFlush(&writer) && JxrBitWriterBytes(&writer) == 3 &&
        data[0] == 0x0d && data[1] == 0x55 && data[2] == 0xd8;
}

static int test_transcode_tile_header_writer_vectors(void)
{
    U8 dcBytes[32] = {0}, lpBytes[32] = {0}, hpBytes[32] = {0}, flexBytes[32] = {0};
    JxrBitWriter dcWriter, lpWriter, hpWriter, flexWriter;
    JxrTranscodeBitSink dcSink, lpSink, hpSink, flexSink;
    JxrTranscodeTileQuantizerState quantizers;
    JxrTranscodeTileHeaderState state;
    JxrTranscodeTileHeaderResult result;

    JxrTranscodeTileQuantizerStateInit(&quantizers);
    quantizers.dcMode = 0;
    quantizers.dcIndex[0] = 0x11;
    quantizers.lowpassQuantizerCount = 1;
    quantizers.lowpassMode[0] = 0;
    quantizers.lowpassIndex[0][0] = 0x22;
    quantizers.highpassQuantizerCount = 1;
    quantizers.highpassMode[0] = 0;
    quantizers.highpassIndex[0][0] = 0x33;
    JxrBitWriterInit(&dcWriter, dcBytes, sizeof(dcBytes));
    JxrBitWriterInit(&lpWriter, lpBytes, sizeof(lpBytes));
    JxrBitWriterInit(&hpWriter, hpBytes, sizeof(hpBytes));
    JxrBitWriterInit(&flexWriter, flexBytes, sizeof(flexBytes));
    JxrTranscodeBitSinkInit(&dcSink, &dcWriter, write_transcode_test_bits);
    JxrTranscodeBitSinkInit(&lpSink, &lpWriter, write_transcode_test_bits);
    JxrTranscodeBitSinkInit(&hpSink, &hpWriter, write_transcode_test_bits);
    JxrTranscodeBitSinkInit(&flexSink, &flexWriter, write_transcode_test_bits);
    memset(&state, 0, sizeof(state));
    state.isSpatial = FALSE;
    state.subband = SB_ALL;
    state.quantizerMode = 7;
    state.trimFlexbits = TRUE;
    state.trimFlexbitsValue = 9;
    state.tileId = 2;
    state.channelCount = 1;
    state.quantizers = &quantizers;
    state.dcOutput = &dcSink;
    state.lowpassOutput = &lpSink;
    state.highpassOutput = &hpSink;
    state.flexbitsOutput = &flexSink;
    if (!JxrTranscodeTileHeaderWriterWrite(&state, &result) ||
        !JxrBitWriterFlush(&dcWriter) || !JxrBitWriterFlush(&lpWriter) ||
        !JxrBitWriterFlush(&hpWriter) || !JxrBitWriterFlush(&flexWriter) ||
        dcBytes[0] != 0 || dcBytes[1] != 0 || dcBytes[2] != 1 || dcBytes[3] != 0x11 ||
        lpBytes[0] != 0 || lpBytes[1] != 0 || lpBytes[2] != 1 || lpBytes[3] != 0x12 ||
        hpBytes[0] != 0 || hpBytes[1] != 0 || hpBytes[2] != 1 || hpBytes[3] != 0x13 ||
        flexBytes[0] != 0 || flexBytes[1] != 0 || flexBytes[2] != 1 || flexBytes[3] != 0x14 ||
        result.lowpassQuantizerBits != 0 || result.highpassQuantizerBits != 0) return 0;

    memset(dcBytes, 0, sizeof(dcBytes));
    JxrBitWriterInit(&dcWriter, dcBytes, sizeof(dcBytes));
    JxrTranscodeBitSinkInit(&dcSink, &dcWriter, write_transcode_test_bits);
    state.isSpatial = TRUE;
    state.subband = SB_DC_ONLY;
    state.quantizerMode = 1;
    state.trimFlexbits = TRUE;
    state.dcOutput = &dcSink;
    return JxrTranscodeTileHeaderWriterWrite(&state, &result) &&
        JxrBitWriterFlush(&dcWriter) && dcBytes[0] == 0 && dcBytes[1] == 0 &&
        dcBytes[2] == 1 && dcBytes[3] == 0x10 && dcBytes[4] == 0x91 &&
        JxrBitWriterBytes(&dcWriter) == 6;
}

static int test_transcode_orientation_state_vectors(void)
{
    static const Bool expectedVertical[O_MAX] = { FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE };
    static const Bool expectedHorizontal[O_MAX] = { FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE };
    static const Bool expectedTranspose[O_MAX] = { FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE };
    ORIENTATION orientation;

    for (orientation = O_NONE; orientation < O_MAX; ++orientation) {
        JxrTranscodeOrientationState state;
        JxrTranscodeOrientationStateInit(&state, orientation);
        if (state.flipVertical != expectedVertical[orientation] ||
            state.flipHorizontal != expectedHorizontal[orientation] ||
            state.transpose != expectedTranspose[orientation] ||
            JxrTranscodeOrientationStateMapRow(&state, 2, 7) !=
                (state.flipVertical ? 4 : 2) ||
            JxrTranscodeOrientationStateMapColumn(&state, 3, 8) !=
                (state.flipHorizontal ? 4 : 3) ||
            JxrTranscodeOrientationStateTileRowCoordinate(&state, 2, 3) !=
                (state.transpose ? 3 : 2) ||
            JxrTranscodeOrientationStateTileColumnCoordinate(&state, 2, 3) !=
                (state.transpose ? 2 : 3) ||
            JxrTranscodeOrientationStateFrameOffset(&state, 2, 3, 8, 7) !=
                (state.transpose ? 23U : 19U)) return 0;
    }
    return 1;
}

static int test_transcode_coefficient_transform_vectors(void)
{
    PixelI sourceValues[300];
    PixelI destinationValues[300];
    JxrTranscodeCoefficientBuffer source;
    JxrTranscodeCoefficientBuffer destination;
    JxrTranscodeOrientationState orientation;
    size_t index;

    for (index = 0; index < 300; ++index) sourceValues[index] = (PixelI)index;
    memset(destinationValues, 0, sizeof(destinationValues));
    JxrTranscodeCoefficientBufferInit(&source, sourceValues, 4, 300);
    JxrTranscodeCoefficientBufferInit(&destination, destinationValues, 8, 300);
    JxrTranscodeOrientationStateInit(&orientation, O_RCW);
    if (!JxrTranscodeCoefficientTransformDc444(&source, &destination, &orientation) ||
        destinationValues[8] != 4 || destinationValues[9] != 8 ||
        destinationValues[10] != 12 || destinationValues[11] != 16 ||
        destinationValues[12] != -5 || destinationValues[13] != -9 ||
        destinationValues[14] != -13 || destinationValues[15] != -17 ||
        sourceValues[5] != -5 || sourceValues[19] != -19) return 0;

    for (index = 0; index < 256; ++index) sourceValues[index] = (PixelI)(1000 + index);
    memset(destinationValues, 0, sizeof(destinationValues));
    JxrTranscodeCoefficientBufferInit(&source, sourceValues, 0, 256);
    JxrTranscodeCoefficientBufferInit(&destination, destinationValues, 0, 256);
    JxrTranscodeOrientationStateInit(&orientation, O_FLIPH);
    if (!JxrTranscodeCoefficientTransformAc444(&source, &destination, &orientation) ||
        destinationValues[12 * 16] != 1000 || destinationValues[0] != 1192 ||
        sourceValues[dctIndex[0][4]] != -(1000 + dctIndex[0][4])) return 0;

    JxrTranscodeCoefficientBufferInit(&source, sourceValues, 250, 256);
    return !JxrTranscodeCoefficientTransformAc444(&source, &destination, &orientation);
}

static int test_transcode_coefficient_transform_422_vectors(void)
{
    PixelI sourceValues[128];
    PixelI destinationValues[128];
    JxrTranscodeCoefficientBuffer source;
    JxrTranscodeCoefficientBuffer destination;
    JxrTranscodeOrientationState orientation;
    size_t index;

    for (index = 0; index < 128; ++index) sourceValues[index] = (PixelI)(100 + index);
    memset(destinationValues, 0, sizeof(destinationValues));
    JxrTranscodeCoefficientBufferInit(&source, sourceValues, 0, 128);
    JxrTranscodeCoefficientBufferInit(&destination, destinationValues, 0, 128);
    JxrTranscodeOrientationStateInit(&orientation, O_FLIPV);
    if (!JxrTranscodeCoefficientTransformDc422(&source, &destination, &orientation) ||
        destinationValues[0] != 100 || destinationValues[1] != -105 ||
        destinationValues[2] != 106 || destinationValues[5] != -101 ||
        destinationValues[7] != -103) return 0;

    for (index = 0; index < 128; ++index) sourceValues[index] = (PixelI)(1000 + index);
    memset(destinationValues, 0, sizeof(destinationValues));
    JxrTranscodeOrientationStateInit(&orientation, O_FLIPH);
    if (!JxrTranscodeCoefficientTransformAc422(&source, &destination, &orientation) ||
        destinationValues[4 * 16] != 1000 || destinationValues[0] != 1064 ||
        sourceValues[dctIndex[0][4]] != -(1000 + dctIndex[0][4])) return 0;

    JxrTranscodeOrientationStateInit(&orientation, O_RCW);
    return !JxrTranscodeCoefficientTransformDc422(&source, &destination, &orientation) &&
        !JxrTranscodeCoefficientTransformAc422(&source, &destination, &orientation);
}

static int test_transcode_coefficient_transform_420_vectors(void)
{
    PixelI sourceValues[64];
    PixelI destinationValues[64];
    JxrTranscodeCoefficientBuffer source;
    JxrTranscodeCoefficientBuffer destination;
    JxrTranscodeOrientationState orientation;
    size_t index;

    for (index = 0; index < 64; ++index) sourceValues[index] = (PixelI)(10 + index);
    memset(destinationValues, 0, sizeof(destinationValues));
    JxrTranscodeCoefficientBufferInit(&source, sourceValues, 0, 64);
    JxrTranscodeCoefficientBufferInit(&destination, destinationValues, 0, 64);
    JxrTranscodeOrientationStateInit(&orientation, O_RCW);
    if (!JxrTranscodeCoefficientTransformDc420(&source, &destination, &orientation) ||
        destinationValues[0] != 10 || destinationValues[1] != 12 ||
        destinationValues[2] != -11 || destinationValues[3] != -13) return 0;

    for (index = 0; index < 64; ++index) sourceValues[index] = (PixelI)(1000 + index);
    memset(destinationValues, 0, sizeof(destinationValues));
    if (!JxrTranscodeCoefficientTransformAc420(&source, &destination, &orientation) ||
        destinationValues[dctIndex[0][1]] != sourceValues[16 + dctIndex[0][4]] ||
        destinationValues[2 * 16 + dctIndex[0][1]] != sourceValues[dctIndex[0][4]]) return 0;

    JxrTranscodeCoefficientBufferInit(&source, sourceValues, 62, 64);
    return !JxrTranscodeCoefficientTransformDc420(&source, &destination, &orientation) &&
        !JxrTranscodeCoefficientTransformAc420(&source, &destination, &orientation);
}

static int test_transcode_tile_extraction_decision_vectors(void)
{
    U32 columns[2] = { 0, 2 };
    U32 rows[2] = { 0, 2 };
    JxrTranscodeTileExtractionDecision decision;

    memset(&decision, 0, sizeof(decision));
    decision.tileColumns = columns;
    decision.tileColumnCount = 2;
    decision.macroblockWidth = 4;
    decision.tileRows = rows;
    decision.tileRowCount = 2;
    decision.macroblockHeight = 4;
    decision.roiWidthPixels = 32;
    decision.roiHeightPixels = 32;
    decision.sourceOverlap = OL_NONE;
    decision.sourceLayout = SPATIAL;
    decision.targetLayout = SPATIAL;
    decision.sourceSubband = SB_ALL;
    decision.targetSubband = SB_ALL;
    if (!JxrTranscodeTileExtractionDecisionCanUseFastPath(&decision) ||
        !decision.ignoreOverlap ||
        JxrTranscodeTileExtractionDecisionIsBoundary(columns, 2, 4, 17) ||
        !JxrTranscodeTileExtractionDecisionIsBoundary(columns, 2, 4, 64)) return 0;

    decision.targetSubband = SB_DC_ONLY;
    if (JxrTranscodeTileExtractionDecisionCanUseFastPath(&decision)) return 0;
    decision.targetSubband = SB_ALL;
    decision.hasTransform = TRUE;
    if (JxrTranscodeTileExtractionDecisionCanUseFastPath(&decision)) return 0;
    decision.hasTransform = FALSE;
    decision.roiLeftPixels = 1;
    return !JxrTranscodeTileExtractionDecisionCanUseFastPath(&decision);
}

static int test_transcode_roi_geometry_vectors(void)
{
    JxrTranscodeRoiGeometryRequest request;
    JxrTranscodeRoiGeometryResult result;

    memset(&request, 0, sizeof(request));
    request.imageWidth = 100;
    request.imageHeight = 80;
    request.extraLeft = 3;
    request.extraTop = 5;
    request.extraRight = 1;
    request.extraBottom = 2;
    request.requestedLeft = 10;
    request.requestedTop = 10;
    request.requestedWidth = 20;
    request.requestedHeight = 20;
    request.overlap = OL_NONE;
    if (!JxrTranscodeRoiGeometryCalculate(&request, &result) ||
        result.expandedLeft != 13 || result.expandedTop != 15 ||
        result.macroblockLeft != 0 || result.macroblockTop != 0 ||
        result.macroblockRight != 3 || result.macroblockBottom != 3 ||
        result.extraLeft != 13 || result.extraTop != 15 ||
        result.extraRight != 15 || result.extraBottom != 13) return 0;

    request.requestedLeft = 0;
    request.requestedTop = 0;
    request.requestedWidth = 5;
    request.requestedHeight = 5;
    request.overlap = OL_TWO;
    if (!JxrTranscodeRoiGeometryCalculate(&request, &result) ||
        result.expandedLeft != 0 || result.expandedTop != 0 ||
        result.expandedWidth != 18 || result.expandedHeight != 20 ||
        result.macroblockRight != 2 || result.macroblockBottom != 2) return 0;

    request.requestedLeft = 99;
    request.requestedWidth = 2;
    return !JxrTranscodeRoiGeometryCalculate(&request, &result);
}

static Bool read_decoder_test_bits(Void* context, U32 count, U32* value)
{ return JxrBitReaderRead((JxrBitReader*)context, count, value); }

static int test_decoder_tile_quantizer_syntax_vectors(void)
{
    U8 data[16] = {0};
    JxrBitWriter writer;
    JxrBitReader reader;
    JxrDecoderBitSource source;
    JxrDecoderQuantizerSyntax dc;
    JxrDecoderQuantizerSetSyntax lp;
    JxrDecoderQuantizerSetSyntax hp;
    struct WMPStream* simpleStream = NULL;
    SimpleBitIO simple;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, 2, 2) || !JxrBitWriterWrite(&writer, 10, 8) ||
        !JxrBitWriterWrite(&writer, 20, 8) || !JxrBitWriterWrite(&writer, 30, 8) ||
        !JxrBitWriterWrite(&writer, 0, 1) || !JxrBitWriterWrite(&writer, 0, 4) ||
        !JxrBitWriterWrite(&writer, 1, 2) || !JxrBitWriterWrite(&writer, 40, 8) ||
        !JxrBitWriterWrite(&writer, 50, 8) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterFlush(&writer)) return 0;
    JxrBitReaderInit(&reader, data, JxrBitWriterBytes(&writer));
    JxrDecoderBitSourceInit(&source, &reader, read_decoder_test_bits);
    if (!JxrDecoderTileQuantizerSyntaxReadDc(&source, 3, &dc) || dc.channelMode != 2 ||
        dc.indices[0] != 10 || dc.indices[1] != 20 || dc.indices[2] != 30 ||
        !JxrDecoderTileQuantizerSyntaxReadLowpass(&source, 3, &lp) || lp.copyPrevious ||
        lp.count != 1 || lp.values[0].channelMode != 1 || lp.values[0].indices[0] != 40 ||
        lp.values[0].indices[1] != 50 ||
        !JxrDecoderTileQuantizerSyntaxReadHighpass(&source, 3, lp.count, &hp)) return 0;
    if (!hp.copyPrevious || hp.count != 1 ||
        JxrDecoderTileQuantizerSyntaxReadDc(&source, 0, &dc)) return 0;

    memset(&simple, 0, sizeof(simple));
    if (CreateWS_Memory(&simpleStream, data, JxrBitWriterBytes(&writer)) != WMP_errSuccess ||
        attach_SB(&simple, simpleStream) != WMP_errSuccess) {
        if (simpleStream != NULL) CloseWS_Memory(&simpleStream);
        return 0;
    }
    JxrDecoderBitSourceInitSimple(&source, &simple);
    if (!JxrDecoderTileQuantizerSyntaxReadDc(&source, 3, &dc) || dc.channelMode != 2 ||
        dc.indices[0] != 10 || dc.indices[1] != 20 || dc.indices[2] != 30) {
        flushToByte_SB(&simple);
        detach_SB(&simple);
        CloseWS_Memory(&simpleStream);
        return 0;
    }
    flushToByte_SB(&simple);
    return detach_SB(&simple) == WMP_errSuccess &&
        CloseWS_Memory(&simpleStream) == WMP_errSuccess;
}

static int read_image_plane_descriptor_vector(const U8* data, size_t count,
    BITDEPTH_BITS bitDepth, JxrImagePlaneDescriptor* descriptor)
{
    SimpleBitIO input;
    struct WMPStream* stream = NULL;
    Bool read;
    memset(&input, 0, sizeof(input));
    if (CreateWS_Memory(&stream, (Void*)data, count) != WMP_errSuccess ||
        attach_SB(&input, stream) != WMP_errSuccess) {
        if (stream != NULL) CloseWS_Memory(&stream);
        return 0;
    }
    read = JxrImagePlaneDescriptorReaderRead(&input, bitDepth, descriptor);
    flushToByte_SB(&input);
    return read && detach_SB(&input) == WMP_errSuccess &&
        CloseWS_Memory(&stream) == WMP_errSuccess;
}

static int read_main_header_vector(const U8* data, size_t count,
    JxrMainHeaderDescriptor* descriptor)
{
    SimpleBitIO input;
    struct WMPStream* stream = NULL;
    Bool read;
    memset(&input, 0, sizeof(input));
    if (CreateWS_Memory(&stream, (Void*)data, count) != WMP_errSuccess ||
        attach_SB(&input, stream) != WMP_errSuccess) {
        if (stream != NULL) CloseWS_Memory(&stream);
        return 0;
    }
    read = JxrMainHeaderReaderRead(&input, descriptor);
    flushToByte_SB(&input);
    return read && detach_SB(&input) == WMP_errSuccess &&
        CloseWS_Memory(&stream) == WMP_errSuccess;
}

static int test_main_header_reader_vectors(void)
{
    U8 data[32] = {0}; JxrBitWriter writer; JxrMainHeaderDescriptor header;
    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, CODEC_VERSION, 4) ||
        !JxrBitWriterWrite(&writer, CODEC_SUBVERSION_NEWSCALING_HARD_TILES, 4) ||
        !JxrBitWriterWrite(&writer, 1, 1) || !JxrBitWriterWrite(&writer, FREQUENCY, 1) ||
        !JxrBitWriterWrite(&writer, O_RCW, 3) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, OL_TWO, 2) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, BD_LONG, 1) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, 1, 1) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, 1, 1) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, 1, 1) || !JxrBitWriterWrite(&writer, CF_RGB, 4) ||
        !JxrBitWriterWrite(&writer, BD_32F, 4) || !JxrBitWriterWrite(&writer, 31, 16) ||
        !JxrBitWriterWrite(&writer, 15, 16) || !JxrBitWriterWrite(&writer, 1, LOG_MAX_TILES) ||
        !JxrBitWriterWrite(&writer, 1, LOG_MAX_TILES) || !JxrBitWriterWrite(&writer, 3, 8) ||
        !JxrBitWriterWrite(&writer, 5, 8) || !JxrBitWriterWrite(&writer, 0, 8) ||
        !JxrBitWriterWrite(&writer, 0, 8) || !JxrBitWriterWrite(&writer, 0, 8) ||
        !JxrBitWriterWrite(&writer, 0, 8) || !JxrBitWriterWrite(&writer, 0, 6) ||
        !JxrBitWriterWrite(&writer, 0, 6) || !JxrBitWriterWrite(&writer, 0, 6) ||
        !JxrBitWriterWrite(&writer, 0, 6) || !JxrBitWriterFlush(&writer) ||
        !read_main_header_vector(data, JxrBitWriterBytes(&writer), &header)) return 0;
    return header.codecVersion == CODEC_VERSION &&
        header.codecSubVersion == CODEC_SUBVERSION_NEWSCALING_HARD_TILES &&
        header.useHardTileBoundaries && header.bitstreamFormat == FREQUENCY &&
        header.orientation == O_RCW && header.hasIndexTable && header.overlap == OL_TWO &&
        header.codedBitDepth == BD_LONG && header.trimFlexbits && header.redBlueSwapped &&
        header.hasAlphaChannel && header.sourceColorFormat == CF_RGB &&
        header.sourceBitDepth == BD_32F && header.width == 32 && header.height == 16 &&
        header.verticalSliceCountMinusOne == 1 && header.horizontalSliceCountMinusOne == 1 &&
        header.tileX[1] == 3 && header.tileY[1] == 5;
}

static int test_header_state_applier_vectors(void)
{
    JxrMainHeaderDescriptor mainHeader;
    JxrImagePlaneDescriptor plane;
    JxrImagePlaneQuantizerHeader quantizers;
    CWMImageInfo imageInfo;
    CWMIStrCodecParam codecParameters;
    CCoreParameters coreParameters;

    memset(&mainHeader, 0, sizeof(mainHeader));
    memset(&plane, 0, sizeof(plane));
    memset(&quantizers, 0, sizeof(quantizers));
    memset(&imageInfo, 0, sizeof(imageInfo));
    memset(&codecParameters, 0, sizeof(codecParameters));
    memset(&coreParameters, 0, sizeof(coreParameters));

    mainHeader.codecVersion = CODEC_VERSION;
    mainHeader.codecSubVersion = CODEC_SUBVERSION_NEWSCALING_HARD_TILES;
    mainHeader.useHardTileBoundaries = TRUE;
    mainHeader.bitstreamFormat = SPATIAL;
    mainHeader.orientation = O_RCW;
    mainHeader.hasIndexTable = TRUE;
    mainHeader.overlap = OL_ONE;
    mainHeader.trimFlexbits = TRUE;
    mainHeader.redBlueSwapped = TRUE;
    mainHeader.hasAlphaChannel = TRUE;
    mainHeader.sourceColorFormat = CF_RGB;
    mainHeader.sourceBitDepth = BD_32F;
    mainHeader.blackWhite = TRUE;
    mainHeader.width = 334; mainHeader.height = 330;
    mainHeader.extraPixelsRight = 2; mainHeader.extraPixelsBottom = 6;
    mainHeader.verticalSliceCountMinusOne = 1; mainHeader.horizontalSliceCountMinusOne = 1;
    mainHeader.tileX[1] = 7; mainHeader.tileY[1] = 9;
    if (!JxrHeaderStateApplierApplyMain(&mainHeader, &imageInfo, &codecParameters,
        &coreParameters) || coreParameters.cVersion != CODEC_VERSION ||
        !coreParameters.bUseHardTileBoundaries || !codecParameters.bUseHardTileBoundaries ||
        codecParameters.bfBitstreamFormat != SPATIAL || imageInfo.oOrientation != O_RCW ||
        !coreParameters.bIndexTable || codecParameters.olOverlap != OL_ONE ||
        codecParameters.bdBitDepth != BD_LONG || !coreParameters.bTrimFlexbitsFlag ||
        !coreParameters.bRBSwapped || !coreParameters.bAlphaChannel ||
        imageInfo.cfColorFormat != CF_RGB || imageInfo.bdBitDepth != BD_32F ||
        !codecParameters.bBlackWhite || imageInfo.cWidth != 334 || imageInfo.cHeight != 330 ||
        coreParameters.cExtraPixelsRight != 2 || coreParameters.cExtraPixelsBottom != 6 ||
        codecParameters.uiTileX[1] != 7 || codecParameters.uiTileY[1] != 9) return 0;

    plane.colorFormat = YUV_420; plane.scaledArithmetic = TRUE; plane.subband = SB_ALL;
    plane.channelCount = 3; plane.hasChromaCenteringX = plane.hasChromaCenteringY = TRUE;
    plane.chromaCenteringX = 4; plane.chromaCenteringY = 2;
    plane.hasSampleConversion = TRUE; plane.mantissaOrShift = 13; plane.exponentBias = -126;
    if (!JxrHeaderStateApplierApplyImagePlane(&plane, &imageInfo, &codecParameters,
        &coreParameters) || coreParameters.cfColorFormat != YUV_420 ||
        !coreParameters.bScaledArith || codecParameters.sbSubband != SB_ALL ||
        coreParameters.cNumChannels != 3 || imageInfo.cChromaCenteringX != 4 ||
        imageInfo.cChromaCenteringY != 2 || codecParameters.nLenMantissaOrShift != 13 ||
        codecParameters.nExpBias != -126) return 0;

    quantizers.quantizerMode = 0x720; quantizers.hasDc = quantizers.hasLp = quantizers.hasHp = TRUE;
    quantizers.dcMode = 0; quantizers.lpMode = 1; quantizers.hpMode = 2;
    quantizers.dcIndices[0] = 7; quantizers.lpIndices[0] = 8; quantizers.lpIndices[1] = 9;
    quantizers.hpIndices[0] = 10; quantizers.hpIndices[1] = 11; quantizers.hpIndices[2] = 12;
    if (!JxrHeaderStateApplierApplyImagePlaneQuantizers(&quantizers, &coreParameters) ||
        coreParameters.uQPMode != 0x720 || coreParameters.uiQPIndexDC[0] != 7 ||
        coreParameters.uiQPIndexLP[1] != 9 || coreParameters.uiQPIndexHP[2] != 12) return 0;
    return !JxrHeaderStateApplierApplyMain(NULL, &imageInfo, &codecParameters, &coreParameters) &&
        !JxrHeaderStateApplierApplyImagePlane(&plane, NULL, &codecParameters, &coreParameters) &&
        !JxrHeaderStateApplierApplyImagePlaneQuantizers(NULL, &coreParameters);
}

static int test_header_validation_vectors(void)
{
    CCoreParameters coreParameters;
    CWMImageInfo imageInfo;
    CWMIStrCodecParam codecParameters;
    memset(&coreParameters, 0, sizeof(coreParameters));
    memset(&imageInfo, 0, sizeof(imageInfo));
    memset(&codecParameters, 0, sizeof(codecParameters));
    coreParameters.uQPMode = 0x600;
    if (JxrHeaderValidationValidateImagePlaneQuantizers(&coreParameters) != JXR_HEADER_VALID)
        return 0;
    coreParameters.uQPMode = 0;
    if (JxrHeaderValidationValidateImagePlaneQuantizers(&coreParameters) !=
        JXR_HEADER_INVALID_QUANTIZER_MODE ||
        JxrHeaderValidationValidateImagePlaneQuantizers(NULL) !=
        JXR_HEADER_INVALID_QUANTIZER_MODE) return 0;

    imageInfo.bdBitDepth = BD_5;
    codecParameters.cfColorFormat = YUV_420;
    if (JxrHeaderValidationValidateSourceFormat(&imageInfo, &codecParameters) !=
        JXR_HEADER_VALID) return 0;
    codecParameters.cfColorFormat = CMYK;
    if (JxrHeaderValidationValidateSourceFormat(&imageInfo, &codecParameters) !=
        JXR_HEADER_UNSUPPORTED_SOURCE_FORMAT) return 0;
    imageInfo.bdBitDepth = BD_8;
    if (JxrHeaderValidationValidateSourceFormat(&imageInfo, &codecParameters) !=
        JXR_HEADER_VALID || JxrHeaderValidationValidateSourceFormat(NULL, &codecParameters) !=
        JXR_HEADER_UNSUPPORTED_SOURCE_FORMAT) return 0;
    return 1;
}

static int test_header_stream_reader_vectors(void)
{
    U8 data[10] = { 'W', 'M', 'P', 'H', 'O', 'T', 'O', 0, 0xa5, 0x5a };
    U8 invalid[8] = { 'W', 'M', 'P', 'H', 'O', 'T', 'O', 1 };
    struct WMPStream* stream = NULL;
    JxrHeaderStreamReader reader;
    SimpleBitIO* input;
    U32 bytesRead;
    if (CreateWS_Memory(&stream, data, sizeof(data)) != WMP_errSuccess ||
        !JxrHeaderStreamReaderOpen(&reader, stream)) {
        if (stream != NULL) CloseWS_Memory(&stream);
        return 0;
    }
    input = JxrHeaderStreamReaderGetBitInput(&reader);
    if (input == NULL || getBit32_SB(input, 4) != 0xa ||
        !JxrHeaderStreamReaderAlignToByte(&reader) ||
        !JxrHeaderStreamReaderClose(&reader, &bytesRead) || bytesRead != 1 ||
        JxrHeaderStreamReaderGetBitInput(&reader) != NULL ||
        CloseWS_Memory(&stream) != WMP_errSuccess) return 0;
    if (CreateWS_Memory(&stream, invalid, sizeof(invalid)) != WMP_errSuccess) return 0;
    if (JxrHeaderStreamReaderOpen(&reader, stream)) {
        JxrHeaderStreamReaderClose(&reader, &bytesRead);
        CloseWS_Memory(&stream);
        return 0;
    }
    return CloseWS_Memory(&stream) == WMP_errSuccess &&
        !JxrHeaderStreamReaderOpen(NULL, NULL);
}

static int test_stream_position_scope_vectors(void)
{
    U8 data[8] = {0};
    struct WMPStream* stream = NULL;
    JxrStreamPositionScope scope;
    size_t position;
    if (CreateWS_Memory(&stream, data, sizeof(data)) != WMP_errSuccess ||
        stream->SetPos(stream, 3) != WMP_errSuccess ||
        !JxrStreamPositionScopeCapture(&scope, stream) ||
        stream->SetPos(stream, 7) != WMP_errSuccess ||
        !JxrStreamPositionScopeRestore(&scope) ||
        stream->GetPos(stream, &position) != WMP_errSuccess || position != 3 ||
        JxrStreamPositionScopeRestore(&scope) || CloseWS_Memory(&stream) != WMP_errSuccess)
        return 0;
    return !JxrStreamPositionScopeCapture(NULL, NULL);
}

static int test_header_metadata_finalizer_vectors(void)
{
    CCoreParameters coreParameters;
    CWMIStrCodecParam codecParameters;
    memset(&coreParameters, 0, sizeof(coreParameters));
    memset(&codecParameters, 0, sizeof(codecParameters));
    coreParameters.bAlphaChannel = TRUE;
    coreParameters.cNumChannels = 3;
    codecParameters.uAlphaMode = 3;
    if (!JxrHeaderMetadataFinalizerApply(10, &coreParameters, &codecParameters) ||
        codecParameters.cbStream != (size_t)((U32)0 - 10) ||
        codecParameters.uAlphaMode != 3 || codecParameters.cChannel != 3) return 0;
    coreParameters.bAlphaChannel = FALSE;
    coreParameters.cNumChannels = 1;
    codecParameters.uAlphaMode = 2;
    if (!JxrHeaderMetadataFinalizerApply(0, &coreParameters, &codecParameters) ||
        codecParameters.cbStream != 0 || codecParameters.uAlphaMode != 0 ||
        codecParameters.cChannel != 1) return 0;
    return !JxrHeaderMetadataFinalizerApply(1, NULL, &codecParameters) &&
        !JxrHeaderMetadataFinalizerApply(1, &coreParameters, NULL);
}

static int test_header_decode_pipeline_vectors(void)
{
    struct WMPStream* stream = NULL;
    CWMImageInfo imageInfo;
    CWMIStrCodecParam codecParameters;
    CCoreParameters coreParameters;
    U8 invalid[8] = { 'W', 'M', 'P', 'H', 'O', 'T', 'O', 1 };
    if (CreateWS_Memory(&stream, invalid, sizeof(invalid)) != WMP_errSuccess) return 0;
    memset(&imageInfo, 0, sizeof(imageInfo));
    memset(&codecParameters, 0, sizeof(codecParameters));
    memset(&coreParameters, 0, sizeof(coreParameters));
    codecParameters.pWStream = stream;
    return !JxrHeaderDecodePipelineRead(&imageInfo, &codecParameters, &coreParameters) &&
        CloseWS_Memory(&stream) == WMP_errSuccess &&
        !JxrHeaderDecodePipelineRead(NULL, NULL, NULL) &&
        !JxrHeaderDecodePipelineReadImagePlane(NULL, NULL, NULL, NULL);
}

typedef struct JxrDecoderInitializationPipelineTestContext {
    Int ioResult;
    Int decoderResult;
    U8 callOrder[3];
    U8 callCount;
} JxrDecoderInitializationPipelineTestContext;

static JxrDecoderInitializationPipelineTestContext* g_decoder_initialization_test;

static Int initialize_pipeline_test_io(CWMImageStrCodec* codec)
{
    UNREFERENCED_PARAMETER(codec);
    g_decoder_initialization_test->callOrder[g_decoder_initialization_test->callCount++] = 1;
    return g_decoder_initialization_test->ioResult;
}

static Int initialize_pipeline_test_decoder(CWMImageStrCodec* codec)
{
    UNREFERENCED_PARAMETER(codec);
    g_decoder_initialization_test->callOrder[g_decoder_initialization_test->callCount++] = 2;
    return g_decoder_initialization_test->decoderResult;
}

static int test_decoder_initialization_pipeline_vectors(void)
{
    CWMImageStrCodec primaryCodec, secondaryCodec;
    JxrDecoderInitializationPipeline pipeline;
    JxrDecoderInitializationPipelineTestContext context;
    memset(&primaryCodec, 0, sizeof(primaryCodec));
    memset(&secondaryCodec, 0, sizeof(secondaryCodec));
    memset(&context, 0, sizeof(context));
    g_decoder_initialization_test = &context;
    JxrDecoderInitializationPipelineInit(&pipeline, &primaryCodec, &secondaryCodec,
        initialize_pipeline_test_io, initialize_pipeline_test_decoder);
    if (JxrDecoderInitializationPipelineRun(&pipeline) != ICERR_OK || context.callCount != 3 ||
        context.callOrder[0] != 1 || context.callOrder[1] != 2 ||
        context.callOrder[2] != 2 || primaryCodec.m_pNextSC != &secondaryCodec) return 0;
    memset(&primaryCodec, 0, sizeof(primaryCodec));
    memset(&context, 0, sizeof(context));
    context.ioResult = ICERR_ERROR;
    JxrDecoderInitializationPipelineInit(&pipeline, &primaryCodec, NULL,
        initialize_pipeline_test_io, initialize_pipeline_test_decoder);
    if (JxrDecoderInitializationPipelineRun(&pipeline) != ICERR_ERROR || context.callCount != 1 ||
        primaryCodec.m_pNextSC != NULL) return 0;
    JxrDecoderInitializationPipelineInit(&pipeline, NULL, NULL,
        initialize_pipeline_test_io, initialize_pipeline_test_decoder);
    return JxrDecoderInitializationPipelineRun(&pipeline) == ICERR_ERROR;
}

typedef struct JxrSecondaryPlaneInitializerTestContext {
    Int headerResult;
    U8 initializeCalls;
    U8 headerCalls;
} JxrSecondaryPlaneInitializerTestContext;

static JxrSecondaryPlaneInitializerTestContext* g_secondary_plane_initializer_test;

static Void initialize_secondary_plane_test_codec(CWMImageStrCodec* codec,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec)
{
    UNREFERENCED_PARAMETER(parameters);
    UNREFERENCED_PARAMETER(templateCodec);
    g_secondary_plane_initializer_test->initializeCalls++;
    codec->cmbWidth = 2;
}

static Int read_secondary_plane_test_header(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters,
    SimpleBitIO* bitInput)
{
    UNREFERENCED_PARAMETER(imageInfo);
    UNREFERENCED_PARAMETER(codecParameters);
    UNREFERENCED_PARAMETER(coreParameters);
    UNREFERENCED_PARAMETER(bitInput);
    g_secondary_plane_initializer_test->headerCalls++;
    return g_secondary_plane_initializer_test->headerResult;
}

static int test_secondary_plane_initializer_vectors(void)
{
    U8 data[1] = { 0 };
    struct WMPStream* stream = NULL;
    CWMImageStrCodec primaryCodec, templateCodec, *secondaryCodec = NULL;
    CCoreParameters parameters;
    BitIOInfo headerBitIO;
    JxrSecondaryPlaneInitializer initializer;
    JxrSecondaryPlaneInitializerTestContext context;
    CWMDecoderParameters decoderParameters;
    if (CreateWS_Memory(&stream, data, sizeof(data)) != WMP_errSuccess) return 0;
    memset(&primaryCodec, 0, sizeof(primaryCodec));
    memset(&templateCodec, 0, sizeof(templateCodec));
    memset(&parameters, 0, sizeof(parameters));
    memset(&headerBitIO, 0, sizeof(headerBitIO));
    memset(&decoderParameters, 0, sizeof(decoderParameters));
    memset(&context, 0, sizeof(context));
    primaryCodec.WMISCP.pWStream = stream;
    primaryCodec.m_Dparam = &decoderParameters;
    primaryCodec.pIOHeader = &headerBitIO;
    g_secondary_plane_initializer_test = &context;
    JxrSecondaryPlaneInitializerInit(&initializer, &primaryCodec, &parameters,
        &templateCodec, 2, 2, initialize_secondary_plane_test_codec,
        read_secondary_plane_test_header);
    if (JxrSecondaryPlaneInitializerRun(&initializer, &secondaryCodec) != ICERR_OK ||
        secondaryCodec == NULL || context.initializeCalls != 1 || context.headerCalls != 1 ||
        secondaryCodec->m_Dparam != &decoderParameters || secondaryCodec->cbChannel != 2 ||
        secondaryCodec->m_param.cfColorFormat != Y_ONLY ||
        secondaryCodec->m_param.cNumChannels != 1 || !secondaryCodec->m_param.bAlphaChannel ||
        secondaryCodec->a0MBbuffer[0] == NULL || secondaryCodec->a1MBbuffer[0] == NULL ||
        secondaryCodec->pIOHeader != &headerBitIO || secondaryCodec->m_pNextSC != &primaryCodec ||
        !secondaryCodec->m_bSecondary) {
        free(secondaryCodec);
        CloseWS_Memory(&stream);
        return 0;
    }
    free(secondaryCodec);
    secondaryCodec = NULL;
    context.headerResult = ICERR_ERROR;
    JxrSecondaryPlaneInitializerInit(&initializer, &primaryCodec, &parameters,
        &templateCodec, 2, 2, initialize_secondary_plane_test_codec,
        read_secondary_plane_test_header);
    if (JxrSecondaryPlaneInitializerRun(&initializer, &secondaryCodec) != ICERR_ERROR ||
        secondaryCodec != NULL || context.initializeCalls != 2 || context.headerCalls != 2) {
        CloseWS_Memory(&stream);
        return 0;
    }
    JxrSecondaryPlaneInitializerInit(&initializer, NULL, &parameters, &templateCodec, 2, 2,
        initialize_secondary_plane_test_codec, read_secondary_plane_test_header);
    return JxrSecondaryPlaneInitializerRun(&initializer, &secondaryCodec) == ICERR_ERROR &&
        CloseWS_Memory(&stream) == WMP_errSuccess;
}

static int test_image_plane_descriptor_reader_vectors(void)
{
    U8 data[16] = {0}; JxrBitWriter writer; JxrImagePlaneDescriptor descriptor;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, Y_ONLY, 3) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, SB_ALL, 4) || !JxrBitWriterFlush(&writer) ||
        !read_image_plane_descriptor_vector(data, JxrBitWriterBytes(&writer), BD_8, &descriptor) ||
        descriptor.colorFormat != Y_ONLY || !descriptor.scaledArithmetic ||
        descriptor.subband != SB_ALL || descriptor.channelCount != 1 ||
        descriptor.hasChromaCenteringX || descriptor.hasSampleConversion) return 0;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, YUV_420, 3) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, SB_NO_FLEXBITS, 4) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, 5, 3) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, 3, 3) || !JxrBitWriterWrite(&writer, 9, 8) ||
        !JxrBitWriterFlush(&writer) ||
        !read_image_plane_descriptor_vector(data, JxrBitWriterBytes(&writer), BD_16, &descriptor) ||
        descriptor.colorFormat != YUV_420 || descriptor.channelCount != 3 ||
        !descriptor.hasChromaCenteringX || !descriptor.hasChromaCenteringY ||
        descriptor.chromaCenteringX != 5 || descriptor.chromaCenteringY != 3 ||
        !descriptor.hasSampleConversion || descriptor.mantissaOrShift != 9) return 0;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, YUV_422, 3) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, SB_NO_HIGHPASS, 4) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, 6, 3) || !JxrBitWriterWrite(&writer, 0, 4) ||
        !JxrBitWriterWrite(&writer, 10, 8) || !JxrBitWriterFlush(&writer) ||
        !read_image_plane_descriptor_vector(data, JxrBitWriterBytes(&writer), BD_32S, &descriptor) ||
        descriptor.colorFormat != YUV_422 || descriptor.subband != SB_NO_HIGHPASS ||
        !descriptor.hasChromaCenteringX || descriptor.hasChromaCenteringY ||
        descriptor.chromaCenteringX != 6 || descriptor.mantissaOrShift != 10) return 0;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, YUV_444, 3) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, SB_ALL, 4) || !JxrBitWriterWrite(&writer, 0, 4) ||
        !JxrBitWriterWrite(&writer, 0, 4) || !JxrBitWriterFlush(&writer) ||
        !read_image_plane_descriptor_vector(data, JxrBitWriterBytes(&writer), BD_8, &descriptor) ||
        descriptor.colorFormat != YUV_444 || descriptor.channelCount != 3) return 0;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, CMYK, 3) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, SB_ALL, 4) || !JxrBitWriterFlush(&writer) ||
        !read_image_plane_descriptor_vector(data, JxrBitWriterBytes(&writer), BD_8, &descriptor) ||
        descriptor.colorFormat != CMYK || descriptor.channelCount != 4) return 0;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, NCOMPONENT, 3) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, SB_ALL, 4) || !JxrBitWriterWrite(&writer, 4, 4) ||
        !JxrBitWriterWrite(&writer, 0, 4) || !JxrBitWriterWrite(&writer, 13, 8) ||
        !JxrBitWriterWrite(&writer, 0x82, 8) || !JxrBitWriterFlush(&writer) ||
        !read_image_plane_descriptor_vector(data, JxrBitWriterBytes(&writer), BD_32F, &descriptor) ||
        descriptor.colorFormat != NCOMPONENT || descriptor.channelCount != 5 ||
        !descriptor.hasSampleConversion || descriptor.mantissaOrShift != 13 ||
        descriptor.exponentBias != -126) return 0;
    return 1;
}

static int test_image_plane_quantizer_header_reader_vectors(void)
{
    U8 data[16] = {0}; JxrBitWriter writer; SimpleBitIO input;
    struct WMPStream* stream = NULL; JxrImagePlaneQuantizerHeader header;
    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, 1, 1) || !JxrBitWriterWrite(&writer, 0, 2) ||
        !JxrBitWriterWrite(&writer, 7, 8) || !JxrBitWriterWrite(&writer, 0, 1) ||
        !JxrBitWriterWrite(&writer, 1, 1) || !JxrBitWriterWrite(&writer, 1, 2) ||
        !JxrBitWriterWrite(&writer, 8, 8) || !JxrBitWriterWrite(&writer, 9, 8) ||
        !JxrBitWriterWrite(&writer, 0, 1) || !JxrBitWriterWrite(&writer, 1, 1) ||
        !JxrBitWriterWrite(&writer, 2, 2) || !JxrBitWriterWrite(&writer, 10, 8) ||
        !JxrBitWriterWrite(&writer, 11, 8) || !JxrBitWriterWrite(&writer, 12, 8) ||
        !JxrBitWriterFlush(&writer) ||
        CreateWS_Memory(&stream, data, JxrBitWriterBytes(&writer)) != WMP_errSuccess) return 0;
    memset(&input, 0, sizeof(input)); attach_SB(&input, stream);
    if (!JxrImagePlaneQuantizerHeaderReaderRead(&input, 3, SB_ALL, &header)) return 0;
    flushToByte_SB(&input); detach_SB(&input); CloseWS_Memory(&stream);
    return header.quantizerMode == 0x720 && header.hasDc && header.hasLp && header.hasHp &&
        header.dcMode == 0 && header.lpMode == 1 && header.hpMode == 2 &&
        header.dcIndices[0] == 7 && header.lpIndices[1] == 9 && header.hpIndices[2] == 12;
}

static int test_decoder_dc_quantizer_header_applier_vectors(void)
{
    CWMImageStrCodec codec;
    CWMITile tiles[2];
    JxrDecoderQuantizerSyntax syntax;
    Bool applied;

    memset(&codec, 0, sizeof(codec));
    memset(tiles, 0, sizeof(tiles));
    memset(&syntax, 0, sizeof(syntax));
    codec.pTile = tiles;
    codec.cTileRow = 0;
    codec.cTileColumn = 0;
    codec.WMISCP.cNumOfSliceMinus1V = 1;
    codec.m_param.cNumChannels = 3;
    syntax.channelMode = 2;
    syntax.indices[0] = 10;
    syntax.indices[1] = 20;
    syntax.indices[2] = 30;

    applied = JxrDecoderDcQuantizerHeaderApplierApply(&codec, &syntax);
    if (!applied || tiles[0].cChModeDC != 2 || tiles[0].pQuantizerDC[0] == NULL ||
        tiles[1].pQuantizerDC[0] == NULL || tiles[0].pQuantizerDC[0][0].iIndex != 10 ||
        tiles[0].pQuantizerDC[1][0].iIndex != 20 ||
        tiles[0].pQuantizerDC[2][0].iIndex != 30 ||
        tiles[0].pQuantizerDC[0][0].iQP == 0) {
        freeQuantizer(tiles[0].pQuantizerDC);
        freeQuantizer(tiles[1].pQuantizerDC);
        return 0;
    }
    freeQuantizer(tiles[0].pQuantizerDC);
    freeQuantizer(tiles[1].pQuantizerDC);
    codec.m_param.cNumChannels = 0;
    return !JxrDecoderDcQuantizerHeaderApplierApply(&codec, &syntax) &&
        !JxrDecoderDcQuantizerHeaderApplierApply(NULL, &syntax);
}

static int test_decoder_lp_quantizer_header_applier_vectors(void)
{
    CWMImageStrCodec codec;
    CWMITile tile;
    JxrDecoderQuantizerSyntax dcSyntax;
    JxrDecoderQuantizerSetSyntax lpSyntax;

    memset(&codec, 0, sizeof(codec));
    memset(&tile, 0, sizeof(tile));
    memset(&dcSyntax, 0, sizeof(dcSyntax));
    memset(&lpSyntax, 0, sizeof(lpSyntax));
    codec.pTile = &tile;
    codec.m_param.cNumChannels = 3;
    dcSyntax.channelMode = 2;
    dcSyntax.indices[0] = 11;
    dcSyntax.indices[1] = 21;
    dcSyntax.indices[2] = 31;
    if (!JxrDecoderDcQuantizerHeaderApplierApply(&codec, &dcSyntax)) return 0;

    lpSyntax.count = 2;
    lpSyntax.values[0].channelMode = 2;
    lpSyntax.values[0].indices[0] = 12;
    lpSyntax.values[0].indices[1] = 22;
    lpSyntax.values[0].indices[2] = 32;
    lpSyntax.values[1].channelMode = 1;
    lpSyntax.values[1].indices[0] = 42;
    lpSyntax.values[1].indices[1] = 52;
    if (!JxrDecoderLpQuantizerHeaderApplierApply(&codec, &lpSyntax) || tile.bUseDC ||
        tile.cNumQPLP != 2 || tile.cBitsLP != 1 || tile.cChModeLP[0] != 2 ||
        tile.cChModeLP[1] != 1 || tile.pQuantizerLP[0][0].iIndex != 12 ||
        tile.pQuantizerLP[2][0].iIndex != 32 || tile.pQuantizerLP[1][1].iIndex != 52 ||
        tile.pQuantizerLP[0][0].iQP == 0) {
        freeQuantizer(tile.pQuantizerLP);
        freeQuantizer(tile.pQuantizerDC);
        return 0;
    }
    freeQuantizer(tile.pQuantizerLP);
    memset(tile.pQuantizerLP, 0, sizeof(tile.pQuantizerLP));

    memset(&lpSyntax, 0, sizeof(lpSyntax));
    lpSyntax.copyPrevious = TRUE;
    lpSyntax.count = 1;
    if (!JxrDecoderLpQuantizerHeaderApplierApply(&codec, &lpSyntax) || !tile.bUseDC ||
        tile.cNumQPLP != 1 || tile.cBitsLP != 0 ||
        tile.pQuantizerLP[0][0].iIndex != 11 || tile.pQuantizerLP[1][0].iIndex != 21 ||
        tile.pQuantizerLP[2][0].iIndex != 31) {
        freeQuantizer(tile.pQuantizerLP);
        freeQuantizer(tile.pQuantizerDC);
        return 0;
    }
    freeQuantizer(tile.pQuantizerLP);
    freeQuantizer(tile.pQuantizerDC);
    lpSyntax.count = 0;
    return !JxrDecoderLpQuantizerHeaderApplierApply(&codec, &lpSyntax) &&
        !JxrDecoderLpQuantizerHeaderApplierApply(NULL, &lpSyntax);
}

static int test_decoder_hp_quantizer_header_applier_vectors(void)
{
    CWMImageStrCodec codec;
    CWMITile tile;
    JxrDecoderQuantizerSetSyntax lpSyntax;
    JxrDecoderQuantizerSetSyntax hpSyntax;

    memset(&codec, 0, sizeof(codec));
    memset(&tile, 0, sizeof(tile));
    memset(&lpSyntax, 0, sizeof(lpSyntax));
    memset(&hpSyntax, 0, sizeof(hpSyntax));
    codec.pTile = &tile;
    codec.m_param.cNumChannels = 3;
    lpSyntax.count = 2;
    lpSyntax.values[0].channelMode = 2;
    lpSyntax.values[0].indices[0] = 13;
    lpSyntax.values[0].indices[1] = 23;
    lpSyntax.values[0].indices[2] = 33;
    lpSyntax.values[1].channelMode = 2;
    lpSyntax.values[1].indices[0] = 43;
    lpSyntax.values[1].indices[1] = 53;
    lpSyntax.values[1].indices[2] = 63;
    if (!JxrDecoderLpQuantizerHeaderApplierApply(&codec, &lpSyntax)) return 0;

    hpSyntax.count = 2;
    hpSyntax.values[0].channelMode = 2;
    hpSyntax.values[0].indices[0] = 14;
    hpSyntax.values[0].indices[1] = 24;
    hpSyntax.values[0].indices[2] = 34;
    hpSyntax.values[1].channelMode = 1;
    hpSyntax.values[1].indices[0] = 44;
    hpSyntax.values[1].indices[1] = 54;
    if (!JxrDecoderHpQuantizerHeaderApplierApply(&codec, &hpSyntax) || tile.bUseLP ||
        tile.cNumQPHP != 2 || tile.cBitsHP != 1 || tile.cChModeHP[0] != 2 ||
        tile.cChModeHP[1] != 1 || tile.pQuantizerHP[0][0].iIndex != 14 ||
        tile.pQuantizerHP[2][0].iIndex != 34 || tile.pQuantizerHP[1][1].iIndex != 54 ||
        tile.pQuantizerHP[0][0].iQP == 0) {
        freeQuantizer(tile.pQuantizerHP);
        freeQuantizer(tile.pQuantizerLP);
        return 0;
    }
    freeQuantizer(tile.pQuantizerHP);
    memset(tile.pQuantizerHP, 0, sizeof(tile.pQuantizerHP));

    memset(&hpSyntax, 0, sizeof(hpSyntax));
    hpSyntax.copyPrevious = TRUE;
    hpSyntax.count = 2;
    if (!JxrDecoderHpQuantizerHeaderApplierApply(&codec, &hpSyntax) || !tile.bUseLP ||
        tile.cNumQPHP != 2 || tile.cBitsHP != 0 ||
        tile.pQuantizerHP[0][0].iIndex != 13 || tile.pQuantizerHP[1][0].iIndex != 23 ||
        tile.pQuantizerHP[2][0].iIndex != 33 || tile.pQuantizerHP[0][1].iIndex != 43 ||
        tile.pQuantizerHP[1][1].iIndex != 53 || tile.pQuantizerHP[2][1].iIndex != 63) {
        freeQuantizer(tile.pQuantizerHP);
        freeQuantizer(tile.pQuantizerLP);
        return 0;
    }
    freeQuantizer(tile.pQuantizerHP);
    freeQuantizer(tile.pQuantizerLP);
    hpSyntax.count = 0;
    return !JxrDecoderHpQuantizerHeaderApplierApply(&codec, &hpSyntax) &&
        !JxrDecoderHpQuantizerHeaderApplierApply(NULL, &hpSyntax);
}

typedef struct JxrTileHeaderReaderTestContext {
    CWMImageStrCodec* primary;
    U8 events[6];
    U8 eventCount;
} JxrTileHeaderReaderTestContext;

static Void record_tile_header_reader_event(JxrTileHeaderReaderTestContext* context,
    CWMImageStrCodec* codec, U8 subband)
{
    context->events[context->eventCount++] = (U8)(subband +
        (codec == context->primary ? 0 : 1));
}

static Void read_tile_header_reader_dc(Void* context, CWMImageStrCodec* codec,
    BitIOInfo* input)
{
    UNREFERENCED_PARAMETER(input);
    record_tile_header_reader_event((JxrTileHeaderReaderTestContext*)context, codec, 0);
}

static Void read_tile_header_reader_lp(Void* context, CWMImageStrCodec* codec,
    BitIOInfo* input)
{
    UNREFERENCED_PARAMETER(input);
    record_tile_header_reader_event((JxrTileHeaderReaderTestContext*)context, codec, 2);
}

static Void read_tile_header_reader_hp(Void* context, CWMImageStrCodec* codec,
    BitIOInfo* input)
{
    UNREFERENCED_PARAMETER(input);
    record_tile_header_reader_event((JxrTileHeaderReaderTestContext*)context, codec, 4);
}

static int test_decoder_tile_header_reader_vectors(void)
{
    CWMImageStrCodec primary;
    CWMImageStrCodec secondary;
    BitIOInfo dcInput;
    BitIOInfo lpInput;
    BitIOInfo hpInput;
    JxrDecoderTileHeaderReaderConfig config;
    JxrDecoderTileHeaderReaderOperations operations;
    JxrTileHeaderReaderTestContext context;

    memset(&primary, 0, sizeof(primary));
    memset(&secondary, 0, sizeof(secondary));
    memset(&dcInput, 0, sizeof(dcInput));
    memset(&lpInput, 0, sizeof(lpInput));
    memset(&hpInput, 0, sizeof(hpInput));
    memset(&config, 0, sizeof(config));
    memset(&operations, 0, sizeof(operations));
    memset(&context, 0, sizeof(context));
    context.primary = &primary;
    config.primaryCodec = &primary;
    config.secondaryCodec = &secondary;
    config.dcInput = &dcInput;
    config.lpInput = &lpInput;
    config.hpInput = &hpInput;
    config.subbandCount = 3;
    operations.context = &context;
    operations.readDc = read_tile_header_reader_dc;
    operations.readLp = read_tile_header_reader_lp;
    operations.readHp = read_tile_header_reader_hp;
    if (!JxrDecoderTileHeaderReaderRead(&config, &operations) || context.eventCount != 6 ||
        context.events[0] != 0 || context.events[1] != 1 || context.events[2] != 2 ||
        context.events[3] != 3 || context.events[4] != 4 || context.events[5] != 5) return 0;

    config.secondaryCodec = NULL;
    config.subbandCount = 1;
    context.eventCount = 0;
    if (!JxrDecoderTileHeaderReaderRead(&config, &operations) || context.eventCount != 1 ||
        context.events[0] != 0) return 0;
    config.primaryCodec = NULL;
    if (JxrDecoderTileHeaderReaderRead(&config, &operations)) return 0;
    config.primaryCodec = &primary;
    config.subbandCount = 2;
    operations.readLp = NULL;
    return !JxrDecoderTileHeaderReaderRead(&config, &operations) &&
        !JxrDecoderTileHeaderReaderRead(NULL, &operations);
}

typedef struct JxrCodingContextResetterTestContext {
    CCodingContext* contexts;
    U8 indices[3];
    U8 count;
} JxrCodingContextResetterTestContext;

static Void record_coding_context_reset(Void* context, CCodingContext* codingContext)
{
    JxrCodingContextResetterTestContext* testContext =
        (JxrCodingContextResetterTestContext*)context;
    testContext->indices[testContext->count++] = (U8)(codingContext - testContext->contexts);
}

static int test_decoder_coding_context_resetter_vectors(void)
{
    CCodingContext contexts[3];
    JxrDecoderCodingContextResetterConfig config;
    JxrDecoderCodingContextResetterOperations operations;
    JxrCodingContextResetterTestContext context;

    memset(contexts, 0, sizeof(contexts));
    memset(&config, 0, sizeof(config));
    memset(&operations, 0, sizeof(operations));
    memset(&context, 0, sizeof(context));
    context.contexts = contexts;
    config.contexts = contexts;
    config.contextCount = 3;
    operations.context = &context;
    operations.reset = record_coding_context_reset;
    if (!JxrDecoderCodingContextResetterResetContexts(&config, &operations) ||
        context.count != 1 || context.indices[0] != 0) return 0;

    config.resetAll = TRUE;
    context.count = 0;
    if (!JxrDecoderCodingContextResetterResetContexts(&config, &operations) ||
        context.count != 3 || context.indices[0] != 0 || context.indices[1] != 1 ||
        context.indices[2] != 2) return 0;
    config.contextCount = 0;
    return !JxrDecoderCodingContextResetterResetContexts(&config, &operations) &&
        !JxrDecoderCodingContextResetterResetContexts(NULL, &operations);
}

typedef struct JxrPacketRowReaderTestContext {
    U8 events[6];
    U8 count;
} JxrPacketRowReaderTestContext;

static Void record_packet_row_event(JxrPacketRowReaderTestContext* context, U8 value)
{ context->events[context->count++] = value; }

static Bool packet_row_test_detach(Void* context, BitIOInfo* reader)
{
    UNREFERENCED_PARAMETER(reader);
    record_packet_row_event((JxrPacketRowReaderTestContext*)context, 0);
    return TRUE;
}

static Bool packet_row_test_attach(Void* context, BitIOInfo* reader, struct WMPStream* stream)
{
    UNREFERENCED_PARAMETER(reader);
    UNREFERENCED_PARAMETER(stream);
    record_packet_row_event((JxrPacketRowReaderTestContext*)context, 1);
    return TRUE;
}

static Bool packet_row_test_seek(Void* context, struct WMPStream* stream, U64 offset)
{
    UNREFERENCED_PARAMETER(stream);
    UNREFERENCED_PARAMETER(offset);
    record_packet_row_event((JxrPacketRowReaderTestContext*)context, 2);
    return TRUE;
}

static Bool packet_row_test_read_header(Void* context, BitIOInfo* reader,
    U8 packetType, U8 packetId)
{
    UNREFERENCED_PARAMETER(reader);
    UNREFERENCED_PARAMETER(packetType);
    UNREFERENCED_PARAMETER(packetId);
    record_packet_row_event((JxrPacketRowReaderTestContext*)context, 3);
    return TRUE;
}

static Bool packet_row_test_store_trim(Void* context, U32 tileColumn, Int value)
{
    UNREFERENCED_PARAMETER(tileColumn);
    UNREFERENCED_PARAMETER(value);
    record_packet_row_event((JxrPacketRowReaderTestContext*)context, 4);
    return TRUE;
}

static Void packet_row_test_reset(Void* context, CCodingContext* codingContext)
{
    UNREFERENCED_PARAMETER(codingContext);
    record_packet_row_event((JxrPacketRowReaderTestContext*)context, 5);
}

static int test_decoder_packet_row_reader_vectors(void)
{
    CWMImageStrCodec codec;
    CCodingContext codingContext;
    BitIOInfo headerInput;
    JxrDecoderPacketRowReaderOperations operations;
    JxrPacketRowReaderTestContext context;

    memset(&codec, 0, sizeof(codec));
    memset(&codingContext, 0, sizeof(codingContext));
    memset(&headerInput, 0, sizeof(headerInput));
    memset(&operations, 0, sizeof(operations));
    memset(&context, 0, sizeof(context));
    headerInput.pWS = (struct WMPStream*)1;
    codec.pIOHeader = &headerInput;
    codec.WMISCP.pWStream = (struct WMPStream*)1;
    codec.m_pCodingContext = &codingContext;
    codec.WMISCP.bfBitstreamFormat = SPATIAL;
    codec.WMISCP.sbSubband = SB_ALL;
    operations.attachment.context = &context;
    operations.attachment.detach = packet_row_test_detach;
    operations.attachment.attach = packet_row_test_attach;
    operations.attachment.seek = packet_row_test_seek;
    operations.header.context = &context;
    operations.header.readHeader = packet_row_test_read_header;
    operations.header.storeTrim = packet_row_test_store_trim;
    operations.resetter.context = &context;
    operations.resetter.reset = packet_row_test_reset;
    if (!JxrDecoderPacketRowReaderRead(&codec, &operations) || context.count != 6 ||
        context.events[0] != 0 || context.events[1] != 2 || context.events[2] != 1 ||
        context.events[3] != 3 || context.events[4] != 4 || context.events[5] != 5) return 0;

    codec.cNumBitIO = 1;
    return !JxrDecoderPacketRowReaderRead(&codec, &operations) &&
        !JxrDecoderPacketRowReaderRead(NULL, &operations);
}

static int test_inverse_color_transform_vectors(void)
{
    PixelI red, green, blue;
    PixelI cyan, magenta, yellow, black;

    red = 0; green = 0; blue = 0;
    JxrInverseColorTransformApplyRgb(&red, &green, &blue);
    if (red != 0 || green != 0 || blue != 0) return 0;
    red = 10; green = 20; blue = 30;
    JxrInverseColorTransformApplyRgb(&red, &green, &blue);
    if (red != 10 || green != 15 || blue != 40) return 0;
    red = -5; green = 7; blue = -8;
    JxrInverseColorTransformApplyRgb(&red, &green, &blue);
    if (red != 9 || green != 10 || blue != 1) return 0;
    red = 1; green = -2; blue = 2;
    JxrInverseColorTransformApplyRgb(&red, &green, &blue);
    if (red != -2 || green != -2 || blue != 0) return 0;

    cyan = 10; magenta = 20; yellow = 30; black = 40;
    JxrInverseColorTransformApplyCmyk(&cyan, &magenta, &yellow, &black);
    if (cyan != 40 || magenta != 45 || yellow != 70 || black != 30) return 0;
    cyan = -5; magenta = 7; yellow = -8; black = 2;
    JxrInverseColorTransformApplyCmyk(&cyan, &magenta, &yellow, &black);
    return cyan == 7 && magenta == 8 && yellow == -1 && black == -2;
}

static int test_postprocess_demacroblock_decision_vectors(void)
{
    struct tagPostProcInfo first;
    struct tagPostProcInfo second;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    first.iMBDC = 100;
    second.iMBDC = 103;
    if (!JxrPostProcessShouldDemacroblock(&first, &second, 3)) return 0;
    if (!JxrPostProcessShouldDemacroblock(&second, &first, 3)) return 0;
    if (JxrPostProcessShouldDemacroblock(&first, &second, 2)) return 0;

    second.iMBDC = 100;
    first.ucMBTexture = 1;
    if (JxrPostProcessShouldDemacroblock(&first, &second, 0)) return 0;
    first.ucMBTexture = 0;
    second.ucMBTexture = 3;
    if (JxrPostProcessShouldDemacroblock(&first, &second, 0)) return 0;
    second.ucMBTexture = 0;
    return !JxrPostProcessShouldDemacroblock(&first, &second, -1);
}

static int test_postprocess_deblock_boundary_decision_vectors(void)
{
    if (!JxrPostProcessShouldDeblockBoundary(0, 100, 2, 103, 3)) return 0;
    if (!JxrPostProcessShouldDeblockBoundary(1, 103, 1, 100, 3)) return 0;
    if (JxrPostProcessShouldDeblockBoundary(1, 100, 2, 100, 0)) return 0;
    if (JxrPostProcessShouldDeblockBoundary(3, 100, 0, 100, 0)) return 0;
    if (JxrPostProcessShouldDeblockBoundary(0, 100, 0, 104, 3)) return 0;
    return !JxrPostProcessShouldDeblockBoundary(0, 100, 0, 100, -1);
}

static int test_postprocess_row_state_vectors(void)
{
    struct tagPostProcInfo* rows[MAX_CHANNELS][2];
    struct tagPostProcInfo* previousRow;
    struct tagPostProcInfo* currentRow;
    Bool isValid = TRUE;

    memset(rows, 0, sizeof(rows));
    if (JxrPostProcessRowStateInitialize(rows, 3, 2) != ICERR_OK) return 0;

    if (rows[0][0][-1].ucMBTexture != 3 ||
        rows[0][0][-1].ucBlockTexture[0][0] != 3 ||
        rows[1][1][3].ucMBTexture != 3) isValid = FALSE;

    previousRow = rows[0][0];
    currentRow = rows[0][1];
    rows[0][1][0].iMBDC = 71;
    JxrPostProcessRowStateAdvance(rows, 2, 3, FALSE, FALSE);
    if (rows[0][0] != currentRow || rows[0][1] != previousRow || rows[0][0][0].iMBDC != 71) isValid = FALSE;

    rows[0][0][0].ucMBTexture = 0;
    rows[0][1][0].ucMBTexture = 0;
    JxrPostProcessRowStateAdvance(rows, 2, 3, TRUE, TRUE);
    if (rows[0][0][0].ucMBTexture != 3 || rows[0][1][0].ucMBTexture != 3) isValid = FALSE;
    JxrPostProcessRowStateRelease(rows, 2);
    return isValid;
}

static int test_postprocess_block_neighborhood_vectors(void)
{
    struct tagPostProcInfo macroblockA;
    struct tagPostProcInfo macroblockB;
    struct tagPostProcInfo macroblockC;
    struct tagPostProcInfo macroblockD;
    JxrPostProcessBlockNeighborhood neighborhood;
    size_t row;
    size_t column;

    memset(&macroblockA, 0, sizeof(macroblockA));
    memset(&macroblockB, 0, sizeof(macroblockB));
    memset(&macroblockC, 0, sizeof(macroblockC));
    memset(&macroblockD, 0, sizeof(macroblockD));
    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            macroblockA.iBlockDC[row][column] = (Int)(row * 10 + column);
            macroblockA.ucBlockTexture[row][column] = 0;
        }
        macroblockB.iBlockDC[row][0] = (Int)(100 + row);
        macroblockC.iBlockDC[0][row] = (Int)(200 + row);
        macroblockB.ucBlockTexture[row][0] = 1;
        macroblockC.ucBlockTexture[0][row] = 2;
    }
    macroblockD.iBlockDC[0][0] = 300;
    macroblockD.ucBlockTexture[0][0] = 3;

    JxrPostProcessBlockNeighborhoodLoad(&neighborhood, &macroblockA, &macroblockB, &macroblockC, &macroblockD);
    if (neighborhood.dc[3][2] != 32 || neighborhood.texture[3][2] != 0 ||
        neighborhood.dc[1][4] != 101 || neighborhood.texture[1][4] != 1 ||
        neighborhood.dc[4][2] != 202 || neighborhood.texture[4][2] != 2 ||
        neighborhood.dc[4][4] != 300 || neighborhood.texture[4][4] != 3) return 0;

    neighborhood.dc[1][1] = 20;
    neighborhood.dc[2][1] = 23;
    if (!JxrPostProcessBlockNeighborhoodShouldSmoothHorizontal(&neighborhood, 1, 1, 3)) return 0;
    neighborhood.texture[2][1] = 3;
    if (JxrPostProcessBlockNeighborhoodShouldSmoothHorizontal(&neighborhood, 1, 1, 3)) return 0;
    neighborhood.texture[2][1] = 0;
    neighborhood.dc[1][2] = 17;
    if (!JxrPostProcessBlockNeighborhoodShouldSmoothVertical(&neighborhood, 1, 1, 3)) return 0;
    neighborhood.dc[1][2] = 24;
    return !JxrPostProcessBlockNeighborhoodShouldSmoothVertical(&neighborhood, 1, 1, 3);
}

static int test_postprocess_smoothing_vectors(void)
{
    PixelI leftOuter;
    PixelI leftInner;
    PixelI rightInner;
    PixelI rightOuter;
    PixelI leftFar;
    PixelI rightFar;

    leftOuter = 0; leftInner = 10; rightInner = 30; rightOuter = 40;
    JxrPostProcessSmoothingApplyMacroblockEdge(&leftOuter, &leftInner, &rightInner, &rightOuter);
    if (leftOuter != 0 || leftInner != 15 || rightInner != 25 || rightOuter != 40) return 0;

    leftOuter = 40; leftInner = 30; rightInner = 10; rightOuter = 0;
    JxrPostProcessSmoothingApplyMacroblockEdge(&leftOuter, &leftInner, &rightInner, &rightOuter);
    if (leftOuter != 40 || leftInner != 25 || rightInner != 15 || rightOuter != 0) return 0;

    leftFar = 0; leftOuter = 10; leftInner = 20; rightInner = 40; rightOuter = 50; rightFar = 60;
    JxrPostProcessSmoothingApplyBlockEdge(&leftFar, &leftOuter, &leftInner, &rightInner, &rightOuter, &rightFar);
    if (leftFar != 0 || leftOuter != 11 || leftInner != 25 || rightInner != 35 || rightOuter != 48 || rightFar != 60) return 0;

    leftFar = 60; leftOuter = 50; leftInner = 40; rightInner = 20; rightOuter = 10; rightFar = 0;
    JxrPostProcessSmoothingApplyBlockEdge(&leftFar, &leftOuter, &leftInner, &rightInner, &rightOuter, &rightFar);
    return leftFar == 60 && leftOuter == 48 && leftInner == 35 &&
        rightInner == 25 && rightOuter == 11 && rightFar == 0;
}

static int test_postprocess_macroblock_analyzer_vectors(void)
{
    PixelI coefficients[256];
    struct tagPostProcInfo result;

    memset(coefficients, 0, sizeof(coefficients));
    memset(&result, 0xff, sizeof(result));
    coefficients[0] = 42;
    JxrPostProcessMacroblockAnalyzerAnalyze(coefficients, &result);
    if (result.iMBDC != 42 || result.ucMBTexture != 0 ||
        result.ucBlockTexture[0][0] != 0 || result.ucBlockTexture[3][3] != 0) return 0;

    coefficients[16] = 9;
    JxrPostProcessMacroblockAnalyzerAnalyze(coefficients, &result);
    if (result.iMBDC != 42 || result.ucMBTexture != 3 || result.ucBlockTexture[1][0] != 0) return 0;

    coefficients[16] = 0;
    coefficients[225] = -7;
    JxrPostProcessMacroblockAnalyzerAnalyze(coefficients, &result);
    return result.iMBDC == 42 && result.ucMBTexture == 0 &&
        result.ucBlockTexture[2][3] == 3 && result.ucBlockTexture[2][2] == 0;
}

static int test_postprocess_block_dc_collector_vectors(void)
{
    PixelI previousBuffer[512];
    PixelI currentBuffer[512];
    PixelI* previousSamples = previousBuffer + 256;
    PixelI* currentSamples = currentBuffer + 256;
    struct tagPostProcInfo macroblockA;
    struct tagPostProcInfo macroblockB;
    struct tagPostProcInfo macroblockC;
    struct tagPostProcInfo macroblockD;
    Int index;

    for (index = 0; index < 512; ++index) {
        previousBuffer[index] = 1000 + index;
        currentBuffer[index] = 2000 + index;
    }
    memset(&macroblockA, 0, sizeof(macroblockA));
    memset(&macroblockB, 0, sizeof(macroblockB));
    memset(&macroblockC, 0, sizeof(macroblockC));
    memset(&macroblockD, 0, sizeof(macroblockD));

    JxrPostProcessBlockDcCollectorCollect(previousSamples, currentSamples,
        &macroblockA, &macroblockB, &macroblockC, &macroblockD);
    return macroblockD.iBlockDC[0][0] == 2256 && macroblockD.iBlockDC[0][1] == 2320 &&
        macroblockD.iBlockDC[1][0] == 2272 && macroblockD.iBlockDC[1][1] == 2336 &&
        macroblockB.iBlockDC[2][0] == 1288 && macroblockB.iBlockDC[2][1] == 1352 &&
        macroblockB.iBlockDC[3][0] == 1304 && macroblockB.iBlockDC[3][1] == 1368 &&
        macroblockC.iBlockDC[0][2] == 2128 && macroblockC.iBlockDC[0][3] == 2192 &&
        macroblockC.iBlockDC[1][2] == 2144 && macroblockC.iBlockDC[1][3] == 2208 &&
        macroblockA.iBlockDC[2][2] == 1160 && macroblockA.iBlockDC[2][3] == 1224 &&
        macroblockA.iBlockDC[3][2] == 1176 && macroblockA.iBlockDC[3][3] == 1240;
}

static int test_postprocess_macroblock_neighborhood_vectors(void)
{
    struct tagPostProcInfo* rows[MAX_CHANNELS][2];
    JxrPostProcessMacroblockNeighborhood neighborhood;
    Bool isValid = TRUE;

    memset(rows, 0, sizeof(rows));
    if (JxrPostProcessRowStateInitialize(rows, 2, 1) != ICERR_OK) return 0;
    rows[0][0][-1].iMBDC = 10;
    rows[0][0][0].iMBDC = 11;
    rows[0][0][1].iMBDC = 12;
    rows[0][1][-1].iMBDC = 20;
    rows[0][1][0].iMBDC = 21;
    rows[0][1][1].iMBDC = 22;

    JxrPostProcessMacroblockNeighborhoodLoad(&neighborhood, rows, 0, 1);
    if (neighborhood.topLeft->iMBDC != 11 || neighborhood.topRight->iMBDC != 12 ||
        neighborhood.bottomLeft->iMBDC != 21 || neighborhood.bottomRight->iMBDC != 22) isValid = FALSE;
    JxrPostProcessMacroblockNeighborhoodLoad(&neighborhood, rows, 0, 0);
    if (neighborhood.topLeft->iMBDC != 10 || neighborhood.topRight->iMBDC != 11 ||
        neighborhood.bottomLeft->iMBDC != 20 || neighborhood.bottomRight->iMBDC != 21) isValid = FALSE;

    JxrPostProcessRowStateRelease(rows, 1);
    return isValid;
}

static int test_postprocess_block_edge_applier_vectors(void)
{
    PixelI samples[1024];
    PixelI* previousRow = samples + 256;
    PixelI* currentRow = samples + 512;
    Int index;

    for (index = 0; index < 1024; ++index) samples[index] = index;
    JxrPostProcessBlockEdgeApplierApplyHorizontal(previousRow, currentRow, 0, 0);
    if (samples[8] != 11 || samples[10] != 8 || samples[16] != 13 || samples[18] != 18) return 0;

    for (index = 0; index < 1024; ++index) samples[index] = index;
    JxrPostProcessBlockEdgeApplierApplyHorizontal(previousRow, currentRow, 3, 0);
    if (samples[56] != 131 || samples[58] != 74 || samples[256] != 181 || samples[258] != 240) return 0;

    for (index = 0; index < 1024; ++index) samples[index] = index;
    JxrPostProcessBlockEdgeApplierApplyVertical(previousRow, 0, 0);
    return samples[4] == 26 && samples[5] == 8 && samples[64] == 42 && samples[65] == 59;
}

static int test_sample_clipping_vectors(void)
{
    return JxrSampleClippingClamp(-2, 0, 31) == 0 &&
        JxrSampleClippingClamp(15, 0, 31) == 15 &&
        JxrSampleClippingClamp(32, 0, 31) == 31 &&
        JxrSampleClippingClamp(64, 0, 63) == 63 &&
        JxrSampleClippingClamp(1024, 0, 1023) == 1023 &&
        JxrSampleClippingToByte(-1) == 0 &&
        JxrSampleClippingToByte(128) == 128 &&
        JxrSampleClippingToByte(256) == 255 &&
        JxrSampleClippingToUInt16(-1) == 0 &&
        JxrSampleClippingToUInt16(32768) == 32768 &&
        JxrSampleClippingToUInt16(65536) == 65535 &&
        JxrSampleClippingToInt16(-32769) == -32768 &&
        JxrSampleClippingToInt16(-1) == -1 &&
        JxrSampleClippingToInt16(32768) == 32767;
}

static int test_float_sample_conversion_vectors(void)
{
    JxrRgbeSample sample;
    sample = JxrFloatSampleConversionToRgbe(0, 0, 0);
    if (sample.red != 0 || sample.green != 0 || sample.blue != 0 || sample.exponent != 0) return 0;
    sample = JxrFloatSampleConversionToRgbe(1, 1, 1);
    if (sample.red != 1 || sample.green != 1 || sample.blue != 1 || sample.exponent != 1) return 0;
    sample = JxrFloatSampleConversionToRgbe(384, 128, 1);
    if (sample.red != 128 || sample.green != 32 || sample.blue != 0 || sample.exponent != 3) return 0;
    return JxrFloatSampleConversionToHalf(0) == 0 &&
        JxrFloatSampleConversionToHalf(5) == 5 &&
        JxrFloatSampleConversionToHalf(-5) == 0x8005 &&
        JxrFloatSampleConversionSingleBits(JxrFloatSampleConversionToSingle(0, 0, 7)) == 0x00000000 &&
        JxrFloatSampleConversionSingleBits(JxrFloatSampleConversionToSingle(128, 1, 7)) == 0x3f800000 &&
        JxrFloatSampleConversionSingleBits(JxrFloatSampleConversionToSingle(-64, 1, 7)) == 0xbf000000 &&
        JxrFloatSampleConversionSingleBits(JxrFloatSampleConversionToSingle(1, 1, 7)) == 0x3c000000;
}

static int test_monochrome_expansion_vectors(void)
{
    U8 bytes[20] = { 10, 80, 90, 40, 20, 81, 91, 41, 0xee, 0xef,
                     30, 82, 92, 42, 40, 83, 93, 43, 0xfc, 0xfd };
    U16 words16[10] = { 100, 800, 900, 400, 0xeeee,
                         200, 801, 901, 401, 0xffff };
    U32 words32[10] = { 1000, 8000, 9000, 4000, 0xeeeeeeee,
                         2000, 8001, 9001, 4001, 0xffffffff };

    JxrMonochromeExpansionReplicateByte(bytes, 10, 2, 2, 4);
    JxrMonochromeExpansionReplicateUInt16(words16, 10, 1, 2, 4);
    JxrMonochromeExpansionReplicateUInt32(words32, 20, 1, 2, 4);
    return bytes[0] == 10 && bytes[1] == 10 && bytes[2] == 10 && bytes[3] == 40 &&
        bytes[4] == 20 && bytes[5] == 20 && bytes[6] == 20 && bytes[7] == 41 &&
        bytes[8] == 0xee && bytes[9] == 0xef && bytes[10] == 30 && bytes[11] == 30 &&
        bytes[12] == 30 && bytes[13] == 42 && bytes[14] == 40 && bytes[15] == 40 &&
        bytes[16] == 40 && bytes[17] == 43 && bytes[18] == 0xfc && bytes[19] == 0xfd &&
        words16[0] == 100 && words16[1] == 100 && words16[2] == 100 && words16[3] == 400 &&
        words16[4] == 0xeeee && words16[5] == 200 && words16[6] == 200 &&
        words16[7] == 200 && words16[8] == 401 && words16[9] == 0xffff &&
        words32[0] == 1000 && words32[1] == 1000 && words32[2] == 1000 && words32[3] == 4000 &&
        words32[4] == 0xeeeeeeee && words32[5] == 2000 && words32[6] == 2000 &&
        words32[7] == 2000 && words32[8] == 4001 && words32[9] == 0xffffffff;
}

static int test_monochrome_expansion_offset_vectors(void)
{
    size_t xOffsets[3] = { 0, 4, 10 };
    size_t yOffsets[2] = { 0, 16 };
    U8 bytes[32] = { 0 };
    U16 words16[32] = { 0 };
    U32 words32[32] = { 0 };

    bytes[20] = 10; bytes[21] = 80; bytes[22] = 90; bytes[23] = 40;
    bytes[26] = 20; bytes[27] = 81; bytes[28] = 91; bytes[29] = 41;
    words16[20] = 100; words16[21] = 800; words16[22] = 900; words16[23] = 400;
    words16[26] = 200; words16[27] = 801; words16[28] = 901; words16[29] = 401;
    words32[20] = 1000; words32[21] = 8000; words32[22] = 9000; words32[23] = 4000;
    words32[26] = 2000; words32[27] = 8001; words32[28] = 9001; words32[29] = 4001;
    JxrMonochromeExpansionReplicateByteAtOffsets(bytes, xOffsets, yOffsets, 1, 2, 1, 3);
    JxrMonochromeExpansionReplicateUInt16AtOffsets(words16, xOffsets, yOffsets, 1, 2, 1, 3);
    JxrMonochromeExpansionReplicateUInt32AtOffsets(words32, xOffsets, yOffsets, 1, 2, 1, 3);
    return bytes[20] == 10 && bytes[21] == 10 && bytes[22] == 10 && bytes[23] == 40 &&
        bytes[26] == 20 && bytes[27] == 20 && bytes[28] == 20 && bytes[29] == 41 &&
        words16[20] == 100 && words16[21] == 100 && words16[22] == 100 && words16[23] == 400 &&
        words16[26] == 200 && words16[27] == 200 && words16[28] == 200 && words16[29] == 401 &&
        words32[20] == 1000 && words32[21] == 1000 && words32[22] == 1000 && words32[23] == 4000 &&
        words32[26] == 2000 && words32[27] == 2000 && words32[28] == 2000 && words32[29] == 4001;
}

static int test_monochrome_expansion_thumbnail_vectors(void)
{
    size_t xOffsets[3] = { 0, 4, 10 };
    size_t yOffsets[2] = { 0, 16 };
    U8 bytes[32] = { 0 };
    U16 words16[32] = { 0 };
    U32 words32[32] = { 0 };

    bytes[20] = 10; bytes[21] = 80; bytes[22] = 90; bytes[23] = 40;
    bytes[26] = 70; bytes[27] = 81; bytes[28] = 20; bytes[29] = 41;
    words16[20] = 100; words16[21] = 800; words16[22] = 900; words16[23] = 400;
    words16[26] = 700; words16[27] = 801; words16[28] = 200; words16[29] = 401;
    words32[20] = 1000; words32[21] = 8000; words32[22] = 9000; words32[23] = 4000;
    words32[26] = 7000; words32[27] = 8001; words32[28] = 2000; words32[29] = 4001;
    JxrMonochromeExpansionReplicateByteAtScaledOffsets(bytes, xOffsets, yOffsets, 2, 4, 2, 6, 2, 1, 0, 2);
    JxrMonochromeExpansionReplicateUInt16AtScaledOffsets(words16, xOffsets, yOffsets, 2, 4, 2, 6, 2, 1, 2, 0);
    JxrMonochromeExpansionReplicateUInt32AtScaledOffsets(words32, xOffsets, yOffsets, 2, 4, 2, 6, 2, 1, 0, 2);
    return bytes[20] == 10 && bytes[21] == 10 && bytes[22] == 10 && bytes[23] == 40 &&
        bytes[26] == 70 && bytes[27] == 70 && bytes[28] == 70 && bytes[29] == 41 &&
        words16[20] == 900 && words16[21] == 900 && words16[22] == 900 && words16[23] == 400 &&
        words16[26] == 200 && words16[27] == 200 && words16[28] == 200 && words16[29] == 401 &&
        words32[20] == 1000 && words32[21] == 1000 && words32[22] == 1000 && words32[23] == 4000 &&
        words32[26] == 7000 && words32[27] == 7000 && words32[28] == 7000 && words32[29] == 4001;
}

static int test_decoder_roi_row_range_vectors(void)
{
    return JxrDecoderRoiRowRangeGetOutputHeight(1, 1) == 1 &&
        JxrDecoderRoiRowRangeGetOutputHeight(16, 1) == 16 &&
        JxrDecoderRoiRowRangeGetOutputHeight(17, 1) == 16 &&
        JxrDecoderRoiRowRangeGetOutputHeight(17, 2) == 1 &&
        JxrDecoderRoiRowRangeGetOutputHeight(31, 2) == 15 &&
        JxrDecoderRoiRowRangeGetOutputHeight(32, 2) == 16;
}

static int test_variable_length_word_vectors(void)
{
    U8 data[24] = { 0 };
    JxrBitWriter writer;
    JxrBitReader reader;
    JxrDecoderBitSource source;
    U64 value;
    U8 escape;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, 0xfd, 8) || !JxrBitWriterWrite(&writer, 0xfe, 8) ||
        !JxrBitWriterWrite(&writer, 0xff, 8) || !JxrBitWriterWrite(&writer, 0x12, 8) ||
        !JxrBitWriterWrite(&writer, 0x34, 8) || !JxrBitWriterWrite(&writer, 0xfb, 8) ||
        !JxrBitWriterWrite(&writer, 0xabcd, 16) || !JxrBitWriterWrite(&writer, 0xef01, 16) ||
        !JxrBitWriterWrite(&writer, 0xfc, 8) || !JxrBitWriterWrite(&writer, 0x1111, 16) ||
        !JxrBitWriterWrite(&writer, 0x2222, 16) || !JxrBitWriterWrite(&writer, 0x3333, 16) ||
        !JxrBitWriterWrite(&writer, 0x4444, 16) || !JxrBitWriterFlush(&writer)) return 0;
    JxrBitReaderInit(&reader, data, JxrBitWriterBytes(&writer));
    JxrDecoderBitSourceInit(&source, &reader, read_decoder_test_bits);
    if (!JxrVariableLengthWordReaderRead(&source, &value, &escape) || value != 0 || escape != 0xfd ||
        !JxrVariableLengthWordReaderRead(&source, &value, &escape) || value != 0 || escape != 0xfe ||
        !JxrVariableLengthWordReaderRead(&source, &value, &escape) || value != 0 || escape != 0xff ||
        !JxrVariableLengthWordReaderRead(&source, &value, &escape) || value != 0x1234 || escape != 0 ||
        !JxrVariableLengthWordReaderRead(&source, &value, &escape) || value != 0xabcdef01 || escape != 0 ||
        !JxrVariableLengthWordReaderRead(&source, &value, &escape)) return 0;
    return value == 0x1111222233334444 && escape == 0 &&
        !JxrVariableLengthWordReaderRead(&source, &value, &escape);
}

typedef struct JxrIndexTableReaderTestContext {
    JxrBitReader reader;
    U32 refillCount;
    Bool aligned;
    U64 position;
} JxrIndexTableReaderTestContext;

static Bool read_index_table_test_bits(Void* context, U32 count, U32* value)
{ return JxrBitReaderRead(&((JxrIndexTableReaderTestContext*)context)->reader, count, value); }
static Bool refill_index_table_test(Void* context)
{ ++((JxrIndexTableReaderTestContext*)context)->refillCount; return TRUE; }
static Bool align_index_table_test(Void* context)
{ ((JxrIndexTableReaderTestContext*)context)->aligned = TRUE; return TRUE; }
static U64 get_index_table_test_position(Void* context)
{ return ((JxrIndexTableReaderTestContext*)context)->position; }
static Bool store_index_table_test_entry(Void* context, U32 index, U64 value)
{ ((U64*)context)[index] = value; return TRUE; }

static int test_index_table_reader_vectors(void)
{
    U8 data[16] = { 0 };
    U8 noEntryData[2] = { 0x00, 0x20 };
    JxrBitWriter writer;
    JxrIndexTableReaderTestContext context;
    JxrIndexTableReader reader;
    U64 entries[2];
    U64 headerSize;

    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, 1, 16) || !JxrBitWriterWrite(&writer, 0x12, 8) ||
        !JxrBitWriterWrite(&writer, 0x34, 8) || !JxrBitWriterWrite(&writer, 0xfb, 8) ||
        !JxrBitWriterWrite(&writer, 0xabcd, 16) || !JxrBitWriterWrite(&writer, 0xef01, 16) ||
        !JxrBitWriterWrite(&writer, 0, 8) || !JxrBitWriterWrite(&writer, 0x20, 8) ||
        !JxrBitWriterFlush(&writer)) return 0;
    memset(&context, 0, sizeof(context));
    context.position = 7;
    JxrBitReaderInit(&context.reader, data, JxrBitWriterBytes(&writer));
    JxrIndexTableReaderInit(&reader, &context, read_index_table_test_bits,
        refill_index_table_test, align_index_table_test, get_index_table_test_position);
    if (!JxrIndexTableReaderRead(&reader, 2, store_index_table_test_entry, entries, &headerSize) || entries[0] != 0x1234 ||
        entries[1] != 0xabcdef01 || headerSize != 0x27 || context.refillCount != 3 ||
        !context.aligned) return 0;

    JxrBitReaderInit(&context.reader, noEntryData, sizeof(noEntryData));
    context.refillCount = 0; context.aligned = FALSE; context.position = 0;
    JxrIndexTableReaderInit(&reader, &context, read_index_table_test_bits,
        refill_index_table_test, align_index_table_test, get_index_table_test_position);
    if (!JxrIndexTableReaderRead(&reader, 0, NULL, NULL, &headerSize) || headerSize != 0x20 ||
        context.refillCount != 1 || !context.aligned) return 0;

    data[1] = 2;
    JxrBitReaderInit(&context.reader, data, JxrBitWriterBytes(&writer));
    context.refillCount = 0; context.aligned = FALSE; context.position = 0;
    if (JxrIndexTableReaderRead(&reader, 2, store_index_table_test_entry, entries, &headerSize)) return 0;
    return !JxrIndexTableReaderRead(&reader, 1, NULL, NULL, &headerSize);
}

typedef struct JxrDecoderStreamInitializerTestContext {
    Int allocateResult;
    Int attachResult;
    Int indexResult;
    U32 callOrder[3];
    U32 callCount;
} JxrDecoderStreamInitializerTestContext;

static Int initialize_test_allocate(Void* context)
{
    JxrDecoderStreamInitializerTestContext* test = (JxrDecoderStreamInitializerTestContext*)context;
    test->callOrder[test->callCount++] = 1;
    return test->allocateResult;
}

static Int initialize_test_attach(Void* context)
{
    JxrDecoderStreamInitializerTestContext* test = (JxrDecoderStreamInitializerTestContext*)context;
    test->callOrder[test->callCount++] = 2;
    return test->attachResult;
}

static Int initialize_test_index(Void* context)
{
    JxrDecoderStreamInitializerTestContext* test = (JxrDecoderStreamInitializerTestContext*)context;
    test->callOrder[test->callCount++] = 3;
    return test->indexResult;
}

static int test_decoder_stream_initializer_vectors(void)
{
    JxrDecoderStreamInitializer initializer;
    JxrDecoderStreamInitializerTestContext context;

    memset(&context, 0, sizeof(context));
    JxrDecoderStreamInitializerInit(&initializer, &context, initialize_test_allocate,
        initialize_test_attach, initialize_test_index);
    if (JxrDecoderStreamInitializerRun(&initializer) != ICERR_OK || context.callCount != 3 ||
        context.callOrder[0] != 1 || context.callOrder[1] != 2 || context.callOrder[2] != 3)
    {
        return 0;
    }

    memset(&context, 0, sizeof(context));
    context.attachResult = ICERR_ERROR;
    JxrDecoderStreamInitializerInit(&initializer, &context, initialize_test_allocate,
        initialize_test_attach, initialize_test_index);
    if (JxrDecoderStreamInitializerRun(&initializer) != ICERR_ERROR || context.callCount != 2 ||
        context.callOrder[0] != 1 || context.callOrder[1] != 2) return 0;

    memset(&context, 0, sizeof(context));
    context.indexResult = ICERR_ERROR;
    JxrDecoderStreamInitializerInit(&initializer, &context, initialize_test_allocate,
        initialize_test_attach, initialize_test_index);
    if (JxrDecoderStreamInitializerRun(&initializer) != ICERR_ERROR || context.callCount != 3 ||
        context.callOrder[2] != 3) return 0;

    JxrDecoderStreamInitializerInit(&initializer, &context, initialize_test_allocate,
        initialize_test_attach, NULL);
    return JxrDecoderStreamInitializerRun(&initializer) == ICERR_ERROR;
}

static int test_decoder_bitstream_set_vectors(void)
{
    JxrDecoderBitstreamSet bitstreams;
    JxrDecoderTileBitstreams tile;
    BitIOInfo header;
    BitIOInfo inputStorage[8];
    BitIOInfo* inputs[8];
    U32 index;

    memset(&header, 0, sizeof(header));
    memset(inputStorage, 0, sizeof(inputStorage));
    for (index = 0; index < 8; ++index) inputs[index] = &inputStorage[index];

    if (!JxrDecoderBitstreamSetInit(&bitstreams, FALSE, SPATIAL, 0, 0, SB_ALL) ||
        bitstreams.bitstreamCount != 0 || bitstreams.subbandCount != 4 || !bitstreams.usesHeaderStream ||
        !JxrDecoderBitstreamSetBindTile(&bitstreams, &header, NULL, 0, &tile) ||
        tile.dc != &header || tile.lp != &header || tile.hp != &header || tile.flexbits != &header)
    {
        return 0;
    }
    if (JxrDecoderBitstreamSetInit(&bitstreams, FALSE, FREQUENCY, 0, 0, SB_ALL) ||
        JxrDecoderBitstreamSetInit(&bitstreams, FALSE, SPATIAL, 1, 0, SB_ALL)) return 0;

    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, SPATIAL, 2, 3, SB_ALL) ||
        bitstreams.tileColumnCount != 3 || bitstreams.bitstreamsPerTile != 1 ||
        bitstreams.bitstreamCount != 3 || bitstreams.usesHeaderStream ||
        !JxrDecoderBitstreamSetBindTile(&bitstreams, &header, inputs, 2, &tile) ||
        tile.dc != inputs[2] || tile.lp != inputs[2] || tile.hp != inputs[2] ||
        tile.flexbits != inputs[2] || JxrDecoderBitstreamSetBindTile(&bitstreams, &header, inputs, 3, &tile))
    {
        return 0;
    }

    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, FREQUENCY, 1, 0, SB_NO_FLEXBITS) ||
        bitstreams.tileColumnCount != 2 || bitstreams.subbandCount != 3 || bitstreams.bitstreamsPerTile != 3 ||
        bitstreams.bitstreamCount != 6 || !JxrDecoderBitstreamSetBindTile(&bitstreams,
        &header, inputs, 1, &tile) || tile.dc != inputs[3] || tile.lp != inputs[4] ||
        tile.hp != inputs[5] || tile.flexbits != inputs[5])
    {
        return 0;
    }
    return !JxrDecoderBitstreamSetInit(&bitstreams, TRUE, SPATIAL, MAX_TILES, 0, SB_ALL);
}

typedef struct JxrPacketAttachmentTestContext {
    U32 detachCount;
    U32 attachCount;
    U32 seekCount;
    BitIOInfo* lastReader;
    struct WMPStream* lastStream;
    U64 lastOffset;
    Bool failAttach;
} JxrPacketAttachmentTestContext;

static Bool detach_packet_attachment_test(Void* context, BitIOInfo* reader)
{
    JxrPacketAttachmentTestContext* test = (JxrPacketAttachmentTestContext*)context;
    ++test->detachCount;
    reader->pWS = NULL;
    return TRUE;
}

static Bool attach_packet_attachment_test(Void* context, BitIOInfo* reader, struct WMPStream* stream)
{
    JxrPacketAttachmentTestContext* test = (JxrPacketAttachmentTestContext*)context;
    ++test->attachCount;
    test->lastReader = reader;
    test->lastStream = stream;
    if (test->failAttach) return FALSE;
    reader->pWS = stream;
    return TRUE;
}

static Bool seek_packet_attachment_test(Void* context, struct WMPStream* stream, U64 offset)
{
    JxrPacketAttachmentTestContext* test = (JxrPacketAttachmentTestContext*)context;
    ++test->seekCount;
    test->lastStream = stream;
    test->lastOffset = offset;
    return TRUE;
}

static int test_decoder_packet_attachment_vectors(void)
{
    JxrDecoderBitstreamSet bitstreams;
    JxrDecoderPacketAttachmentConfig config;
    JxrDecoderPacketAttachmentOperations operations;
    JxrPacketAttachmentTestContext context;
    BitIOInfo header, storage[6];
    BitIOInfo* readers[6];
    size_t offsets[4] = { 100, 200, 300, 400 };
    struct WMPStream* primary = (struct WMPStream*)(size_t)1;
    struct WMPStream* external[12];
    U32 index;

    memset(&context, 0, sizeof(context));
    memset(&header, 0, sizeof(header));
    memset(storage, 0, sizeof(storage));
    for (index = 0; index < 6; ++index) readers[index] = &storage[index];
    for (index = 0; index < 12; ++index) external[index] = (struct WMPStream*)(size_t)(index + 10);
    operations.context = &context;
    operations.detach = detach_packet_attachment_test;
    operations.attach = attach_packet_attachment_test;
    operations.seek = seek_packet_attachment_test;

    if (!JxrDecoderBitstreamSetInit(&bitstreams, FALSE, SPATIAL, 0, 0, SB_ALL)) return 0;
    JxrDecoderPacketAttachmentConfigInit(&config, &bitstreams, 0, 1, FALSE, &header, NULL,
        NULL, 12, primary, NULL, 0);
    if (!JxrDecoderPacketAttachmentAttachRow(&config, &operations) || context.detachCount != 1 ||
        context.seekCount != 1 || context.lastOffset != 12 || context.attachCount != 1 ||
        header.pWS != primary) return 0;

    memset(&context, 0, sizeof(context));
    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, SPATIAL, 1, 1, SB_ALL)) return 0;
    readers[0]->pWS = primary; readers[1]->pWS = primary;
    JxrDecoderPacketAttachmentConfigInit(&config, &bitstreams, 1, 2, FALSE, NULL, readers,
        offsets, 10, primary, NULL, 0);
    if (!JxrDecoderPacketAttachmentAttachRow(&config, &operations) || context.detachCount != 2 ||
        context.seekCount != 2 || context.lastOffset != 410 || context.attachCount != 2 ||
        readers[0]->pWS != primary || readers[1]->pWS != primary) return 0;

    memset(&context, 0, sizeof(context));
    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, FREQUENCY, 1, 1, SB_NO_FLEXBITS)) return 0;
    for (index = 0; index < 6; ++index) readers[index]->pWS = primary;
    JxrDecoderPacketAttachmentConfigInit(&config, &bitstreams, 1, 2, TRUE, NULL, readers,
        NULL, 0, NULL, external, 12);
    if (!JxrDecoderPacketAttachmentAttachRow(&config, &operations) || context.detachCount != 6 ||
        context.seekCount != 0 || context.attachCount != 6 || readers[0]->pWS != external[6] ||
        readers[5]->pWS != external[11]) return 0;

    context.failAttach = TRUE;
    return !JxrDecoderPacketAttachmentAttachRow(&config, &operations);
}

typedef struct JxrPacketHeaderReaderTestContext {
    U8 packetTypes[16];
    U8 packetIds[16];
    U32 packetCount;
    U32 trimValue;
    Int trims[4];
    U8 failedPacketType;
} JxrPacketHeaderReaderTestContext;

static Bool read_packet_header_test(Void* context, BitIOInfo* reader, U8 packetType, U8 packetId)
{
    JxrPacketHeaderReaderTestContext* test = (JxrPacketHeaderReaderTestContext*)context;
    UNREFERENCED_PARAMETER(reader);
    test->packetTypes[test->packetCount] = packetType;
    test->packetIds[test->packetCount++] = packetId;
    return packetType != test->failedPacketType;
}

static Bool read_packet_trim_test(Void* context, BitIOInfo* reader, U32* value)
{
    UNREFERENCED_PARAMETER(reader);
    *value = ((JxrPacketHeaderReaderTestContext*)context)->trimValue;
    return TRUE;
}

static Bool store_packet_trim_test(Void* context, U32 tileColumn, Int value)
{
    ((JxrPacketHeaderReaderTestContext*)context)->trims[tileColumn] = value;
    return TRUE;
}

static int test_decoder_packet_header_reader_vectors(void)
{
    JxrDecoderBitstreamSet bitstreams;
    JxrDecoderPacketHeaderReaderConfig config;
    JxrDecoderPacketHeaderReaderOperations operations;
    JxrPacketHeaderReaderTestContext context;
    BitIOInfo storage[4];
    BitIOInfo* readers[4];
    U32 index;

    memset(storage, 0, sizeof(storage));
    for (index = 0; index < 4; ++index) {
        readers[index] = &storage[index];
        readers[index]->pWS = (struct WMPStream*)(size_t)(index + 1);
    }
    operations.context = &context;
    operations.readHeader = read_packet_header_test;
    operations.readTrim = read_packet_trim_test;
    operations.storeTrim = store_packet_trim_test;

    memset(&context, 0, sizeof(context));
    context.trimValue = 9;
    context.failedPacketType = 0xff;
    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, SPATIAL, 1, 0, SB_ALL)) return 0;
    JxrDecoderPacketHeaderReaderConfigInit(&config, &bitstreams, 15, TRUE, NULL, readers);
    if (!JxrDecoderPacketHeaderReaderReadRow(&config, &operations) || context.packetCount != 2 ||
        context.packetTypes[0] != 0 || context.packetTypes[1] != 0 ||
        context.packetIds[0] != 30 || context.packetIds[1] != 31 ||
        context.trims[0] != 9 || context.trims[1] != 9) return 0;

    memset(&context, 0, sizeof(context));
    context.failedPacketType = 0xff;
    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, FREQUENCY, 0, 0, SB_DC_ONLY)) return 0;
    JxrDecoderPacketHeaderReaderConfigInit(&config, &bitstreams, 31, FALSE, NULL, readers);
    if (!JxrDecoderPacketHeaderReaderReadRow(&config, &operations) || context.packetCount != 1 ||
        context.packetTypes[0] != 1 || context.packetIds[0] != 31) return 0;

    memset(&context, 0, sizeof(context));
    context.trimValue = 6;
    context.failedPacketType = 4;
    if (!JxrDecoderBitstreamSetInit(&bitstreams, TRUE, FREQUENCY, 0, 0, SB_ALL)) return 0;
    JxrDecoderPacketHeaderReaderConfigInit(&config, &bitstreams, 0, TRUE, NULL, readers);
    if (!JxrDecoderPacketHeaderReaderReadRow(&config, &operations) || context.packetCount != 4 ||
        context.packetTypes[0] != 1 || context.packetTypes[1] != 2 ||
        context.packetTypes[2] != 3 || context.packetTypes[3] != 4 || context.trims[0] != 6) return 0;

    context.failedPacketType = 2;
    return !JxrDecoderPacketHeaderReaderReadRow(&config, &operations);
}

static int test_packet_header_syntax_reader_vectors(void)
{
    U8 data[4] = { 0x00, 0x00, 0x01, 0xad };
    JxrBitReader reader;
    JxrDecoderBitSource source;
    JxrPacketHeaderSyntax header;

    JxrBitReaderInit(&reader, data, sizeof(data));
    JxrDecoderBitSourceInit(&source, &reader, read_decoder_test_bits);
    if (!JxrPacketHeaderSyntaxReaderRead(&source, &header) ||
        !JxrPacketHeaderSyntaxIsValid(&header) ||
        JxrPacketHeaderSyntaxGetTileId(&header) != 21 ||
        JxrPacketHeaderSyntaxGetPacketType(&header) != 5) return 0;

    data[1] = 2;
    JxrBitReaderInit(&reader, data, sizeof(data));
    JxrDecoderBitSourceInit(&source, &reader, read_decoder_test_bits);
    if (!JxrPacketHeaderSyntaxReaderRead(&source, &header) ||
        JxrPacketHeaderSyntaxIsValid(&header)) return 0;

    JxrBitReaderInit(&reader, data, 3);
    JxrDecoderBitSourceInit(&source, &reader, read_decoder_test_bits);
    return !JxrPacketHeaderSyntaxReaderRead(&source, &header) &&
        !JxrPacketHeaderSyntaxReaderRead(NULL, &header) &&
        !JxrPacketHeaderSyntaxReaderRead(&source, NULL);
}

static int test_bit_input_buffer_state_vectors(void)
{
    JxrBitInputBufferState state;
    U8 ring[8192] = { 0 };

    JxrBitInputBufferStateInit(&state, ring, sizeof(ring), 0, 4095, 4096, 0);
    if (JxrBitInputBufferStateNeedsRefill(&state, 4096)) return 0;
    state.currentIndex = 4096;
    if (!JxrBitInputBufferStateNeedsRefill(&state, 4096)) return 0;
    JxrBitInputBufferStateAdvancePacketStart(&state, 4096);
    if (state.packetStartIndex != 4096 || state.streamOffset != 4096) return 0;
    JxrBitInputBufferStateInit(&state, ring, sizeof(ring), 4096, 4096, 8192, 0x12345678U);
    JxrBitInputBufferStateAdvancePacketStart(&state, 4096);
    return state.packetStartIndex == 0 && state.shadow == 0x12345678U &&
        JxrBitInputBufferStateNeedsRefill(&state, 4096);
}

typedef struct { U8* data; size_t length; size_t lastOffset; } JxrFakePacketSource;
static JxrPacketReadResult read_fake_packet(Void* context, size_t offset, U8* destination, size_t count)
{
    JxrFakePacketSource* source = (JxrFakePacketSource*)context;
    JxrPacketReadResult result;
    result.status = JxrPacketReadFailed;
    result.bytesRead = 0;
    result.nativeError = WMP_errFileIO;
    if (offset + count > source->length) return result;
    memcpy(destination, source->data + offset, count);
    source->lastOffset = offset;
    result.status = JxrPacketReadCompleted;
    result.bytesRead = count;
    result.nativeError = WMP_errSuccess;
    return result;
}

static JxrPacketReadResult read_partial_packet(Void* context, size_t offset, U8* destination,
    size_t count)
{
    JxrFakePacketSource* source = (JxrFakePacketSource*)context;
    JxrPacketReadResult result;
    size_t available;

    result.status = JxrPacketReadFailed;
    result.bytesRead = 0;
    result.nativeError = WMP_errFileIO;
    if (offset > source->length) return result;
    available = source->length - offset;
    if (available > count) available = count;
    memcpy(destination, source->data + offset, available);
    source->lastOffset = offset;
    result.status = available == count ? JxrPacketReadCompleted : JxrPacketReadShort;
    result.bytesRead = available;
    result.nativeError = available == count ? WMP_errSuccess : WMP_errFileIO;
    return result;
}

static int test_packet_source_vectors(void)
{
    U8 data[8192] = { 0 }, ring[8192] = { 0 };
    JxrFakePacketSource fake = { data, sizeof(data), 0 };
    JxrPacketSource source = { &fake, read_fake_packet };
    JxrBitInputBufferState state;
    JxrPacketReadResult result;
    data[4096] = 0x78; data[4097] = 0x56; data[4098] = 0x34; data[4099] = 0x12;
    JxrBitInputBufferStateInit(&state, ring, sizeof(ring), 0, 4096, 4096, 0);
    if (!JxrBitInputBufferStateReadPacket(&state, &source, 4096, &result)) return 0;
    return fake.lastOffset == 4096 && state.streamOffset == 8192 &&
        result.status == JxrPacketReadCompleted && result.bytesRead == 4096 &&
        state.shadow == 0x12345678U && state.packetStartIndex == 4096 && ring[0] == 0x78;
}

static int test_packet_executor_vectors(void)
{
    U8 data[8192] = { 0 }, ring[8192] = { 0 };
    JxrFakePacketSource fake = { data, sizeof(data), 0 };
    JxrPacketSource source = { &fake, read_fake_packet };
    JxrBitInputBufferState state;
    Bool didRefill;
    JxrPacketReadResult result;

    JxrBitInputBufferStateInit(&state, ring, sizeof(ring), 0, 0, 4096, 0x12345678U);
    if (!JxrPacketExecutorTryRefill(&state, &source, 4096, &didRefill, &result) || didRefill)
        return 0;
    data[4096] = 0x78; data[4097] = 0x56; data[4098] = 0x34; data[4099] = 0x12;
    state.currentIndex = 4096;
    if (!JxrPacketExecutorTryRefill(&state, &source, 4096, &didRefill, &result) || !didRefill)
        return 0;
    return fake.lastOffset == 4096 && ring[0] == 0x78 &&
        state.packetStartIndex == 4096 && state.streamOffset == 8192 &&
        state.shadow == 0x12345678U && result.status == JxrPacketReadCompleted;
}

static int test_packet_short_read_vectors(void)
{
    U8 data[4098] = { 0 }, ring[8192];
    JxrFakePacketSource fake = { data, sizeof(data), 0 };
    JxrPacketSource source = { &fake, read_partial_packet };
    JxrBitReaderCore core;
    Bool didRefill;
    U32 expectedShadow;

    memset(ring, 0x5a, sizeof(ring));
    data[4096] = 0x78;
    data[4097] = 0x56;
    JxrBitReaderCoreInit(&core, ring, sizeof(ring), 0, 4096, 4096, 0,
        0, 0);
    if (!JxrBitReaderCoreTryRefill(&core, &source, 4096, &didRefill)) return 0;
    memcpy(&expectedShadow, ring, sizeof(expectedShadow));
    return didRefill && !core.hasError && fake.lastOffset == 4096 &&
        core.lastPacketRead.status == JxrPacketReadShort &&
        core.lastPacketRead.bytesRead == 2 &&
        core.lastPacketRead.nativeError == WMP_errFileIO &&
        ring[0] == 0x78 && ring[1] == 0x56 && ring[2] == 0x5a &&
        core.input.shadow == expectedShadow && core.input.packetStartIndex == 4096 &&
        core.input.streamOffset == 8192;
}

static int test_bit_reader_core_vectors(void)
{
    U8 data[8192] = { 0 }, ring[8192] = { 0 };
    JxrFakePacketSource fake = { data, sizeof(data), 0 };
    JxrPacketSource source = { &fake, read_fake_packet };
    JxrBitReaderCore core;
    Bool didRefill;

    data[4096] = 0x78; data[4097] = 0x56; data[4098] = 0x34; data[4099] = 0x12;
    JxrBitReaderCoreInit(&core, ring, sizeof(ring), 0, 0, 4096, 0,
        0xa0000000U, 0);
    if (JxrBitReaderCorePeek(&core, 4) != 10) return 0;
    JxrBitReaderCoreConsume(&core, 4);
    if (core.cursor.usedBits != 4 || core.input.currentIndex != 0) return 0;
    if (!JxrBitReaderCoreTryRefill(&core, &source, 4096, &didRefill) || didRefill) return 0;
    core.input.currentIndex = 4096;
    if (!JxrBitReaderCoreTryRefill(&core, &source, 4096, &didRefill) || !didRefill) return 0;
    return !core.hasError && ring[0] == 0x78 && core.input.packetStartIndex == 4096 &&
        core.input.streamOffset == 8192 && core.input.shadow == 0x12345678U;
}

static int test_refill_error_propagation_vectors(void)
{
    union { U64 alignment; U8 bytes[PACKETLENGTH * 2 + sizeof(BitIOInfo)]; } storage;
    BitIOInfo* legacy = (BitIOInfo*)(storage.bytes + PACKETLENGTH * 2);
    JxrLegacyBitReaderAdapter adapter;
    JxrEntropyBitReader reader;
    JxrDecoderFormatState format;

    memset(&storage, 0, sizeof(storage));
    legacy->pbStart = storage.bytes;
    legacy->pbCurrent = storage.bytes + PACKETLENGTH;
    legacy->iMask = -8192;
    legacy->offRef = 4096;
    JxrLegacyBitReaderAdapterInit(&adapter, legacy);
    if (JxrLegacyBitReaderAdapterRefillLevel1(NULL, &adapter) ||
        !adapter.core.hasError || adapter.core.lastPacketRead.status != JxrPacketReadFailed)
        return 0;

    JxrEntropyBitReaderInit(&reader, legacy);
    JxrDecoderFormatStateInit(&format, NULL);
    if (JxrDecoderFormatStateRefillLevel1(&format, &reader) ||
        !JxrEntropyBitReaderHasError(&reader)) return 0;

    legacy->pbCurrent = storage.bytes;
    JxrEntropyBitReaderInit(&reader, legacy);
    return JxrDecoderFormatStateRefillLevel1(&format, &reader) &&
        !JxrEntropyBitReaderHasError(&reader) &&
        JxrDecoderFormatStateRefillLevel2(&format, &reader);
}

static int test_bit_cursor_state_vectors(void)
{
    U8 data[64] = { 0xb1, 0xab, 0xcd, 0xf0, 0x12, 0x34, 0x56, 0x78 };
    BitIOInfo legacy;
    JxrBitCursorState cursor;
    U32 legacyValue;

    memset(&legacy, 0, sizeof(legacy));
    legacy.pbCurrent = data;
    legacy.iMask = ~(UINTPTR_T)1;
    legacy.uiAccumulator = ((U32)data[0] << 24) | ((U32)data[1] << 16) |
        ((U32)data[2] << 8) | (U32)data[3];
    JxrBitCursorStateInit(&cursor, data, sizeof(data), 0, legacy.uiAccumulator, legacy.cBitsUsed);
    if (JxrBitCursorStatePeek(&cursor, 0) != 0 ||
        JxrBitCursorStateReadLong(&cursor, 0) != 0 ||
        !JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    if (JxrBitCursorStatePeek(&cursor, 3) != peekBit16(&legacy, 3) ||
        !JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    JxrBitCursorStateConsume(&cursor, 0);
    flushBit16(&legacy, 0);
    if (!JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    JxrBitCursorStateConsume(&cursor, 3);
    flushBit16(&legacy, 3);
    if (!JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    if (JxrBitCursorStatePeek(&cursor, 13) != peekBit16(&legacy, 13)) return 0;
    JxrBitCursorStateConsume(&cursor, 5);
    flushBit16(&legacy, 5);
    if (!JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    JxrBitCursorStateConsume(&cursor, 8);
    flushBit16(&legacy, 8);
    if (!JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    legacyValue = getBit32(&legacy, 17);
    if (JxrBitCursorStateReadLong(&cursor, 17) != legacyValue ||
        !JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor)) return 0;
    legacyValue = getBit32(&legacy, 32);
    return JxrBitCursorStateReadLong(&cursor, 32) == legacyValue &&
        JxrLegacyBitIoBridgeCursorMatches(&legacy, &cursor);
}

static int test_bit_cursor_ring_wrap_vectors(void)
{
    U8 ring[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    JxrBitCursorState cursor;

    JxrBitCursorStateInit(&cursor, ring, sizeof(ring), 6, 0, 0);
    JxrBitCursorStateConsume(&cursor, 16);
    if (cursor.currentIndex != 0 || cursor.usedBits != 0 ||
        cursor.accumulator != 0x01234567U) return 0;
    JxrBitCursorStateInit(&cursor, ring, sizeof(ring), 6, 0, 7);
    JxrBitCursorStateConsume(&cursor, 9);
    return cursor.currentIndex == 0 && cursor.usedBits == 0 &&
        cursor.accumulator == 0x01234567U;
}

static int test_explicit_entropy_context(void)
{
    CCodingContext native; JxrEntropyContext state;
    memset(&native, 0, sizeof(native)); JxrEntropyContextInit(&state, &native);
    JxrEntropyContextReset(&state);
    return state.native == &native && state.dcModel == &native.m_aModelDC &&
        state.lpModel == &native.m_aModelLP && state.acModel == &native.m_aModelAC &&
        state.lowpassScan[1].uScan == 1 && state.dcModel->m_iFlcBits[0] == 8;
}

static int test_adaptive_scan_vectors(void)
{
    CAdaptiveScan scan[4];
    memset(scan, 0, sizeof(scan)); scan[0].uScan=0; scan[1].uScan=1; scan[2].uScan=2; scan[3].uScan=3;
    JxrAdaptiveScanResetTotals(scan, 4);
    if (scan[0].uTotal != MAXTOTAL || scan[1].uTotal != 32 || scan[2].uTotal != 30 ||
        JxrAdaptiveScanGetCoefficientIndex(scan, 2) != 2) return 0;
    JxrAdaptiveScanObserveNonZero(scan, 2); JxrAdaptiveScanObserveNonZero(scan, 2);
    JxrAdaptiveScanObserveNonZero(scan, 2);
    return scan[1].uScan == 2 && scan[1].uTotal == 33 &&
        scan[2].uScan == 1 && scan[2].uTotal == 32;
}

static int test_adaptive_scan_state_vectors(void)
{
    CAdaptiveScan scan[4];
    JxrAdaptiveScanState state;

    memset(scan, 0, sizeof(scan));
    scan[0].uScan = 3;
    scan[1].uScan = 2;
    scan[2].uScan = 1;
    scan[3].uScan = 0;
    JxrAdaptiveScanStateInit(&state, scan);
    JxrAdaptiveScanStateResetTotals(&state, 4);
    if (JxrAdaptiveScanStateGetCoefficientIndex(&state, 2) != 1) return 0;
    JxrAdaptiveScanStateObserveNonZero(&state, 2);
    JxrAdaptiveScanStateObserveNonZero(&state, 2);
    JxrAdaptiveScanStateObserveNonZero(&state, 2);
    return JxrAdaptiveScanStateGetCoefficientIndex(&state, 1) == 1 &&
        JxrAdaptiveScanStateGetCoefficientIndex(&state, 2) == 2;
}

static int test_coefficient_buffer_vectors(void)
{
    PixelI values[5] = { 3, 5, 7, 11, 13 };
    JxrCoefficientBuffer buffer = JxrCoefficientBufferCreate(values, 1, 3);
    if (JxrCoefficientBufferGet(&buffer, 0) != 5 ||
        JxrCoefficientBufferGet(&buffer, 2) != 11) return 0;
    JxrCoefficientBufferSet(&buffer, 1, -2);
    JxrCoefficientBufferAdd(&buffer, 2, 4);
    if (values[0] != 3 || values[1] != 5 || values[2] != -2 || values[3] != 15 || values[4] != 13) return 0;
    JxrCoefficientBufferClear(&buffer);
    return values[0] == 3 && values[1] == 0 && values[2] == 0 && values[3] == 0 && values[4] == 13;
}

static int test_huffman_decoder_vectors(void)
{
    short root[32];
    short branch[JXR_HUFFMAN_BRANCH_OFFSET + 1];
    U8 rootData[2] = { 0, 0 };
    U8 branchData[2] = { 0x04, 0 };
    BitIOInfo input;
    JxrHuffmanTable table;
    int i;

    for (i = 0; i < 32; ++i) root[i] = (2 << JXR_HUFFMAN_ENCODED_LENGTH_BITS) | 5;
    table = JxrHuffmanTableCreate(root);
    memset(&input, 0, sizeof(input));
    input.uiAccumulator = 0;
    input.iMask = -2;
    input.pbCurrent = rootData;
    if (JxrHuffmanDecoderDecodeSymbol(&table, &input) != 2 || input.cBitsUsed != 5) return 0;

    for (i = 0; i <= JXR_HUFFMAN_BRANCH_OFFSET; ++i) branch[i] = 0;
    for (i = 0; i < 32; ++i) branch[i] = -8;
    branch[JXR_HUFFMAN_BRANCH_OFFSET - 8] = 4;
    branch[JXR_HUFFMAN_BRANCH_OFFSET - 7] = 6;
    table = JxrHuffmanTableCreate(branch);
    memset(&input, 0, sizeof(input));
    input.uiAccumulator = 0x04000000;
    input.iMask = -2;
    input.pbCurrent = branchData;
    return JxrHuffmanTableGetEntry(&table, JXR_HUFFMAN_BRANCH_OFFSET - 7) == 6 &&
        JxrHuffmanDecoderDecodeSymbol(&table, &input) == 6 && input.cBitsUsed == 6;
}

static int test_lp_residual_vectors(void)
{
    return JxrLpResidualDecoderCombineNonZero(1, 2, 2) == 6 &&
        JxrLpResidualDecoderCombineNonZero(-1, 2, 2) == -6 &&
        JxrLpResidualDecoderCombineSignedMagnitude(2, 1, 2) == 9 &&
        JxrLpResidualDecoderCombineSignedMagnitude(-2, 1, 2) == -9;
}

static int test_lowpass_cbp_state_vectors(void)
{
    Int zeroCount = 1;
    Int maxCount = 1;
    JxrLowpassCbpState state;

    JxrLowpassCbpStateInit(&state, &zeroCount, &maxCount);
    JxrLowpassCbpStateObserve(&state, 0, 3);
    if (JxrLowpassCbpStateGetZeroCount(&state) != -2 ||
        JxrLowpassCbpStateGetMaxCount(&state) != 2) return 0;
    JxrLowpassCbpStateObserve(&state, 3, 3);
    if (JxrLowpassCbpStateGetZeroCount(&state) != -1 ||
        JxrLowpassCbpStateGetMaxCount(&state) != -1) return 0;
    zeroCount = -8;
    maxCount = 7;
    JxrLowpassCbpStateObserve(&state, 0, 3);
    return JxrLowpassCbpStateGetZeroCount(&state) == -8 &&
        JxrLowpassCbpStateGetMaxCount(&state) == 7;
}

static int test_macroblock_cbp_state_vectors(void)
{
    Int cbp[16] = { 0 };
    Int differential[16] = { 0 };
    JxrMacroblockCbpState state;

    JxrMacroblockCbpStateInit(&state, cbp, differential);
    JxrMacroblockCbpStateSetCbp(&state, 0, 0x1234);
    JxrMacroblockCbpStateSetCbp(&state, 1, 0x3f);
    JxrMacroblockCbpStateSetCbp(&state, 2, 0x55);
    JxrMacroblockCbpStateSetCbp(&state, 15, 0x7a);
    JxrMacroblockCbpStateSetDifferential(&state, 0, 0x4321);
    JxrMacroblockCbpStateSetDifferential(&state, 1, 0x2a);
    JxrMacroblockCbpStateSetDifferential(&state, 2, 0x15);
    if (JxrMacroblockCbpStateGetCbp(&state, 0) != 0x1234 ||
        JxrMacroblockCbpStateGetCbp(&state, 1) != 0x3f ||
        JxrMacroblockCbpStateGetCbp(&state, 2) != 0x55 ||
        JxrMacroblockCbpStateGetCbp(&state, 15) != 0x7a ||
        JxrMacroblockCbpStateGetDifferential(&state, 0) != 0x4321 ||
        JxrMacroblockCbpStateGetDifferential(&state, 1) != 0x2a ||
        JxrMacroblockCbpStateGetDifferential(&state, 2) != 0x15 || cbp[0] != 0)
        return 0;
    JxrMacroblockCbpStateCommitToNative(&state);
    if (cbp[0] != 0x1234 || cbp[1] != 0x3f || cbp[2] != 0x55 || cbp[15] != 0x7a ||
        differential[0] != 0x4321 || differential[1] != 0x2a || differential[2] != 0x15)
        return 0;
    cbp[0] = 7;
    differential[0] = 9;
    JxrMacroblockCbpStateLoadFromNative(&state);
    return JxrMacroblockCbpStateGetCbp(&state, 0) == 7 &&
        JxrMacroblockCbpStateGetDifferential(&state, 0) == 9;
}

static int test_bit_math_vectors(void)
{
    const U32 value = 0x12345678U;

    return JxrBitMathRotateLeft32(value, 0) == 0x12345678U &&
        JxrBitMathRotateLeft32(value, 1) == 0x2468acf0U &&
        JxrBitMathRotateLeft32(value, 14) == 0x159e048dU &&
        JxrBitMathRotateLeft32(value, 16) == 0x56781234U &&
        JxrBitMathRotateLeft32(value, 31) == 0x091a2b3cU &&
        JxrBitMathLowMask32(0) == 0U &&
        JxrBitMathLowMask32(1) == 0x1U &&
        JxrBitMathLowMask32(14) == 0x3fffU &&
        JxrBitMathLowMask32(16) == 0xffffU &&
        JxrBitMathLowMask32(31) == 0x7fffffffU &&
        JxrBitMathLowMask32(32) == 0xffffffffU;
}

static int test_prediction_math_vectors(void)
{
    return JxrPredictionMathDequantize(0, 17) == 0 &&
        JxrPredictionMathDequantize(3, 5) == 15 &&
        JxrPredictionMathDequantize(-3, 5) == -15 &&
        JxrPredictionMathSaturateAdaptiveCount(-17) == -16 &&
        JxrPredictionMathSaturateAdaptiveCount(-16) == -16 &&
        JxrPredictionMathSaturateAdaptiveCount(-1) == -1 &&
        JxrPredictionMathSaturateAdaptiveCount(0) == 0 &&
        JxrPredictionMathSaturateAdaptiveCount(14) == 14 &&
        JxrPredictionMathSaturateAdaptiveCount(15) == 15 &&
        JxrPredictionMathSaturateAdaptiveCount(16) == 15;
}

static int test_inverse_transform_macroblock_geometry_vectors(void)
{
    JxrInverseTransformMacroblockGeometry geometry;

    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_420, 0, 0, 4, 7, 3, 8);
    if (geometry.overlap != OL_TWO || geometry.colorFormat != YUV_420 ||
        !geometry.isLeft || geometry.isRight || !geometry.isTop || geometry.isBottom ||
        !geometry.isTopOrBottom || !geometry.isLeftOrRight ||
        !geometry.isTopOrLeft || geometry.isBottomOrRight ||
        geometry.isLeftAdjacentColumn || geometry.isRightAdjacentColumn ||
        geometry.macroblockWidth != 4 || geometry.macroblockColumn != 0 ||
        geometry.channelCount != 1 || geometry.thumbnailScale != 8) return 0;

    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        OL_ONE, YUV_444, 3, 7, 4, 7, 4, 16);
    if (geometry.overlap != OL_ONE || geometry.colorFormat != YUV_444 ||
        geometry.isLeft || geometry.isRight || geometry.isTop || !geometry.isBottom ||
        !geometry.isTopOrBottom || geometry.isLeftOrRight ||
        geometry.isTopOrLeft || !geometry.isBottomOrRight ||
        geometry.isLeftAdjacentColumn || !geometry.isRightAdjacentColumn ||
        geometry.macroblockWidth != 4 || geometry.macroblockColumn != 3 ||
        geometry.channelCount != 4 || geometry.thumbnailScale != 16) return 0;

    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        OL_NONE, YUV_444, 4, 2, 4, 7, 3, 2);
    return geometry.isRight && !geometry.isBottom &&
        geometry.isLeftOrRight && geometry.isBottomOrRight &&
        !geometry.isRightAdjacentColumn && geometry.channelCount == 3;
}

static int test_hard_tile_boundary_state_vectors(void)
{
    static const U32 verticalColumns[] = { 0, 3, 6, 10 };
    static const U32 horizontalRows[] = { 0, 2, 5 };
    JxrHardTileBoundaryConfiguration configuration;
    JxrHardTileBoundaryState previous;
    JxrHardTileBoundaryState result;

    memset(&configuration, 0, sizeof(configuration));
    memset(&previous, 0, sizeof(previous));
    configuration.enabled = FALSE;
    previous.tileX = 3;
    previous.tileY = 2;
    previous.isVerticalBoundary = TRUE;
    previous.isHorizontalBoundary = TRUE;
    previous.isOneMacroblockLeftOfVerticalBoundary = TRUE;
    previous.isOneMacroblockRightOfVerticalBoundary = TRUE;
    JxrHardTileBoundaryStateCalculate(&result, &previous, &configuration, 4, 5);
    if (result.tileX != 3 || result.tileY != 2 || result.isVerticalBoundary ||
        result.isHorizontalBoundary || result.isOneMacroblockLeftOfVerticalBoundary ||
        result.isOneMacroblockRightOfVerticalBoundary ||
        result.previousMacroblockX != 4 || result.previousMacroblockY != 5) return 0;

    configuration.enabled = TRUE;
    configuration.verticalSliceCountMinusOne = 2;
    configuration.horizontalSliceCountMinusOne = 3;
    configuration.verticalSliceColumns = verticalColumns;
    configuration.horizontalSliceRows = horizontalRows;
    previous.tileX = 1;
    previous.tileY = 2;
    previous.previousMacroblockY = 4;
    previous.isVerticalBoundary = TRUE;
    previous.isHorizontalBoundary = TRUE;
    JxrHardTileBoundaryStateCalculate(&result, &previous, &configuration, 0, 0);
    if (result.tileX != 0 || result.tileY != 0 || result.isVerticalBoundary ||
        result.isHorizontalBoundary || result.isOneMacroblockLeftOfVerticalBoundary ||
        result.isOneMacroblockRightOfVerticalBoundary) return 0;

    previous = result;
    previous.previousMacroblockY = 1;
    JxrHardTileBoundaryStateCalculate(&result, &previous, &configuration, 3, 2);
    if (!result.isVerticalBoundary || !result.isHorizontalBoundary ||
        result.tileY != 1 || result.tileX != 1 ||
        result.isOneMacroblockLeftOfVerticalBoundary ||
        result.isOneMacroblockRightOfVerticalBoundary) return 0;

    previous = result;
    previous.previousMacroblockY = 2;
    JxrHardTileBoundaryStateCalculate(&result, &previous, &configuration, 4, 2);
    return !result.isVerticalBoundary && result.isHorizontalBoundary &&
        !result.isOneMacroblockLeftOfVerticalBoundary &&
        result.isOneMacroblockRightOfVerticalBoundary &&
        result.tileY == 1 && result.tileX == 1;
}

static int test_inverse_transform_boundary_context_vectors(void)
{
    JxrInverseTransformMacroblockGeometry geometry;
    JxrHardTileBoundaryState hardTileState;
    JxrInverseTransformBoundaryContext context;

    memset(&hardTileState, 0, sizeof(hardTileState));
    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        OL_NONE, YUV_444, 2, 3, 4, 7, 3, 1);
    JxrInverseTransformBoundaryContextInitialize(&context, &geometry, &hardTileState);
    if (context.isVerticalTileBoundary || context.isHorizontalTileBoundary ||
        context.hasTopBoundary || context.hasBottomBoundary ||
        context.hasLeftBoundary || context.hasRightBoundary ||
        context.hasTopOrBottomBoundary || context.hasLeftOrRightBoundary ||
        context.isLeftAdjacentToVerticalBoundary ||
        context.isRightAdjacentToVerticalBoundary) return 0;

    hardTileState.isVerticalBoundary = TRUE;
    hardTileState.isHorizontalBoundary = TRUE;
    hardTileState.isOneMacroblockRightOfVerticalBoundary = TRUE;
    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_444, 3, 2, 4, 7, 3, 1);
    JxrInverseTransformBoundaryContextInitialize(&context, &geometry, &hardTileState);
    return context.isVerticalTileBoundary && context.isHorizontalTileBoundary &&
        context.hasTopBoundary && context.hasBottomBoundary &&
        context.hasLeftBoundary && context.hasRightBoundary &&
        context.hasTopOrBottomBoundary && context.hasLeftOrRightBoundary &&
        context.isLeftAdjacentToVerticalBoundary &&
        context.isRightAdjacentToVerticalBoundary;
}

static int test_inverse_postprocess_parameters_vectors(void)
{
    CWMIQuantizer lowPassChannel0[2];
    CWMIQuantizer lowPassChannel1[2];
    CWMIQuantizer directCurrentChannel0[1];
    CWMIQuantizer directCurrentChannel1[1];
    CWMIQuantizer* lowPassQuantizers[2];
    CWMIQuantizer* directCurrentQuantizers[2];
    JxrInversePostProcessParameters parameters;

    memset(lowPassChannel0, 0, sizeof(lowPassChannel0));
    memset(lowPassChannel1, 0, sizeof(lowPassChannel1));
    memset(directCurrentChannel0, 0, sizeof(directCurrentChannel0));
    memset(directCurrentChannel1, 0, sizeof(directCurrentChannel1));
    lowPassChannel0[1].iQP = 5;
    lowPassChannel1[1].iQP = 7;
    directCurrentChannel0[0].iQP = 3;
    directCurrentChannel1[0].iQP = 11;
    lowPassQuantizers[0] = lowPassChannel0;
    lowPassQuantizers[1] = lowPassChannel1;
    directCurrentQuantizers[0] = directCurrentChannel0;
    directCurrentQuantizers[1] = directCurrentChannel1;

    JxrInversePostProcessParametersInitialize(&parameters,
        0, OL_NONE, 2, lowPassQuantizers, directCurrentQuantizers, 1);
    if (parameters.enabled || parameters.lowPassQuantizers[0] != 0 ||
        parameters.directCurrentQuantizers[1] != 0) return 0;

    JxrInversePostProcessParametersInitialize(&parameters,
        2, OL_NONE, 2, lowPassQuantizers, directCurrentQuantizers, 1);
    if (!parameters.enabled || parameters.lowPassQuantizers[0] != 40 ||
        parameters.lowPassQuantizers[1] != 56 ||
        parameters.directCurrentQuantizers[0] != 12 ||
        parameters.directCurrentQuantizers[1] != 44) return 0;

    JxrInversePostProcessParametersInitialize(&parameters,
        1, OL_ONE, 2, lowPassQuantizers, directCurrentQuantizers, 1);
    return parameters.enabled && parameters.lowPassQuantizers[0] == 10 &&
        parameters.lowPassQuantizers[1] == 14 &&
        parameters.directCurrentQuantizers[0] == 6 &&
        parameters.directCurrentQuantizers[1] == 22;
}

static int test_inverse_highpass_parameters_vectors(void)
{
    CWMIQuantizer channel0[2];
    CWMIQuantizer channel1[2];
    CWMIQuantizer* quantizers[2];
    JxrInverseHighPassParameters parameters;

    memset(channel0, 0, sizeof(channel0));
    memset(channel1, 0, sizeof(channel1));
    channel0[1].iQP = 19;
    channel1[1].iQP = 37;
    quantizers[0] = channel0;
    quantizers[1] = channel1;

    JxrInverseHighPassParametersInitialize(&parameters,
        SB_NO_HIGHPASS, 2, quantizers, 1);
    if (!parameters.isAbsent ||
        parameters.quantizers[0] != JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER ||
        parameters.quantizers[1] != JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER) return 0;

    JxrInverseHighPassParametersInitialize(&parameters,
        SB_DC_ONLY, 2, quantizers, 1);
    if (!parameters.isAbsent ||
        parameters.quantizers[0] != JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER) return 0;

    JxrInverseHighPassParametersInitialize(&parameters,
        SB_ALL, 2, quantizers, 1);
    return !parameters.isAbsent && parameters.quantizers[0] == 19 &&
        parameters.quantizers[1] == 37 &&
        parameters.quantizers[2] == JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER;
}

static int test_inverse_transform_plane_plan_vectors(void)
{
    JxrInverseTransformPlanePlan plan;

    JxrInverseTransformPlanePlanInitialize(&plan, YUV_444, 3, 1);
    if (!plan.transformsSamples || plan.fullResolutionChannelCount != 3 ||
        plan.chroma420ChannelCount != 0 || plan.chroma422ChannelCount != 0) return 0;

    JxrInverseTransformPlanePlanInitialize(&plan, YUV_420, 1, 4);
    if (!plan.transformsSamples || plan.fullResolutionChannelCount != 1 ||
        plan.chroma420ChannelCount != 2 || plan.chroma422ChannelCount != 0) return 0;

    JxrInverseTransformPlanePlanInitialize(&plan, YUV_422, 1, 16);
    return !plan.transformsSamples && plan.fullResolutionChannelCount == 1 &&
        plan.chroma420ChannelCount == 0 && plan.chroma422ChannelCount == 2;
}

static int test_inverse_transform_plane_buffers_vectors(void)
{
    PixelI first0[1], first1[1], first2[1];
    PixelI second0[1], second1[1], second2[1];
    PixelI* firstPlanes[] = { first0, first1, first2 };
    PixelI* secondPlanes[] = { second0, second1, second2 };
    JxrInverseTransformPlaneBuffers buffers;

    JxrInverseTransformPlaneBuffersResolveFullResolution(
        &buffers, firstPlanes, secondPlanes, 0);
    if (buffers.firstStage != first0 || buffers.secondStage != second0) return 0;

    JxrInverseTransformPlaneBuffersResolveFullResolution(
        &buffers, firstPlanes, secondPlanes, 2);
    if (buffers.firstStage != first2 || buffers.secondStage != second2) return 0;

    JxrInverseTransformPlaneBuffersResolveChroma(
        &buffers, firstPlanes, secondPlanes, 1);
    return buffers.firstStage == first2 && buffers.secondStage == second2;
}

static int test_inverse_transform_plane_context_vectors(void)
{
    PixelI first0[1], first1[1], first2[1];
    PixelI second0[1], second1[1], second2[1];
    PixelI* firstPlanes[] = { first0, first1, first2 };
    PixelI* secondPlanes[] = { second0, second1, second2 };
    JxrInversePostProcessParameters postProcessParameters;
    JxrInverseHighPassParameters highPassParameters;
    JxrInverseTransformPlaneContext context;

    memset(&postProcessParameters, 0, sizeof(postProcessParameters));
    memset(&highPassParameters, 0, sizeof(highPassParameters));
    postProcessParameters.lowPassQuantizers[1] = 12;
    postProcessParameters.directCurrentQuantizers[1] = 34;
    highPassParameters.quantizers[1] = 56;
    highPassParameters.isAbsent = FALSE;
    JxrInverseTransformPlaneContextInitialize(&context, firstPlanes, secondPlanes,
        TRUE, 1, &postProcessParameters, &highPassParameters);
    if (context.buffers.firstStage != first2 || context.buffers.secondStage != second2 ||
        context.channelIndex != 1 || context.lowPassQuantizer != 12 ||
        context.directCurrentQuantizer != 34 || context.highPassQuantizer != 56 ||
        context.isHighPassAbsent) return 0;

    JxrInverseTransformPlaneContextInitialize(&context, firstPlanes, secondPlanes,
        FALSE, 0, &postProcessParameters, NULL);
    return context.buffers.firstStage == first0 && context.buffers.secondStage == second0 &&
        context.highPassQuantizer == JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER &&
        context.isHighPassAbsent;
}

static int test_inverse_transform_plane_stage2_vectors(void)
{
    PixelI expected[256];
    PixelI actual[256];
    size_t index;

    for (index = 0; index < 256; ++index) {
        expected[index] = (PixelI)(index - 128);
        actual[index] = expected[index];
    }
    strIDCT4x4Stage2(expected);
    JxrInverseTransformMathNormalizeBlock(expected, TRUE, 256, 16);
    JxrInverseTransformPlaneStage2Apply(actual, TRUE, TRUE);
    if (memcmp(expected, actual, sizeof(expected)) != 0) return 0;

    for (index = 0; index < 256; ++index) {
        expected[index] = (PixelI)(index - 128);
        actual[index] = expected[index];
    }
    strIDCT4x4Stage2(expected);
    JxrInverseTransformPlaneStage2Apply(actual, FALSE, FALSE);
    return memcmp(expected, actual, sizeof(expected)) == 0;
}

static int test_inverse_transform_normalization_vectors(void)
{
    PixelI expected[256];
    PixelI actual[256];
    Int sampleIndex;

    for (sampleIndex = 0; sampleIndex < 256; ++sampleIndex) {
        expected[sampleIndex] = sampleIndex - 128;
        actual[sampleIndex] = expected[sampleIndex];
    }
    strNormalizeDec(actual, FALSE);
    if (memcmp(expected, actual, sizeof(expected)) != 0) return 0;

    strNormalizeDec(actual, TRUE);
    if (actual[0] != -256 || actual[16] != -224 || actual[128] != 0 ||
        actual[240] != 224 || actual[17] != -111) return 0;

    JxrInverseTransformMathNormalizeBlock(expected, TRUE, 256, 16);
    return memcmp(expected, actual, sizeof(expected)) == 0;
}

static int test_inverse_transform_math_vectors(void)
{
    PixelI first, second;
    PixelI topLeft, topRight, bottomLeft, bottomRight;

    first = 10; second = 5;
    JxrInverseTransformMathRotateHalf(&first, &second);
    if (first != 7 || second != 9) return 0;
    first = -10; second = 5;
    JxrInverseTransformMathRotateHalf(&first, &second);
    if (first != -13 || second != -1) return 0;
    first = 10; second = 5;
    JxrInverseTransformMathRotateThreeEighths(&first, &second);
    if (first != 8 || second != 8) return 0;
    first = -10; second = 5;
    JxrInverseTransformMathRotateThreeEighths(&first, &second);
    if (first != -12 || second != 1) return 0;
    {
        PixelI cornerSamples[3] = { 10, 20, 30 };

        JxrInverseTransformMathAddCornerPredictionAt(cornerSamples + 1, -1, 6);
        if (cornerSamples[0] != 16 || cornerSamples[1] != 20 || cornerSamples[2] != 30) return 0;
        JxrInverseTransformMathSubtractCornerPredictionAt(cornerSamples, 2, 9);
        if (cornerSamples[0] != 16 || cornerSamples[1] != 20 || cornerSamples[2] != 21) return 0;
    }
    {
        PixelI samples[256];
        Int sampleIndex;

        for (sampleIndex = 0; sampleIndex < 256; ++sampleIndex) {
            samples[sampleIndex] = sampleIndex;
        }
        JxrInverseTransformMathNormalizeBlock(samples, FALSE, 256, 16);
        if (samples[0] != 0 || samples[16] != 16 || samples[17] != 17 || samples[240] != 240) return 0;
        JxrInverseTransformMathNormalizeBlock(samples, TRUE, 256, 16);
        if (samples[0] != 0 || samples[16] != 32 || samples[17] != 17 || samples[240] != 480) return 0;
    }
    if (!(!JxrInverseTransformMathShouldCompensateDc(0, 20, FALSE) &&
        JxrInverseTransformMathShouldCompensateDc(0, 21, FALSE) &&
        JxrInverseTransformMathShouldCompensateDc(-20, 21, FALSE) &&
        !JxrInverseTransformMathShouldCompensateDc(21, 21, FALSE) &&
        !JxrInverseTransformMathShouldCompensateDc(-21, 21, FALSE) &&
        JxrInverseTransformMathShouldCompensateDc(500, 0, TRUE))) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyDcCompensation(&topLeft, &topRight, &bottomLeft, &bottomRight, 6);
    if (topLeft != 7 || topRight != 23 || bottomLeft != 33 || bottomRight != 37) return 0;
    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyDcCompensation(&topLeft, &topRight, &bottomLeft, &bottomRight, -6);
    if (topLeft != 13 || topRight != 17 || bottomLeft != 27 || bottomRight != 43) return 0;
    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyDcCompensation(&topLeft, &topRight, &bottomLeft, &bottomRight, 5);
    if (topLeft != 8 || topRight != 22 || bottomLeft != 32 || bottomRight != 38) return 0;

    topLeft = 20; topRight = 10; bottomLeft = 10; bottomRight = 20;
    if (JxrInverseTransformMathApplyConditionalDcCompensation(
        &topLeft, &topRight, &bottomLeft, &bottomRight, 6, 21, FALSE) != 6) return 0;
    if (topLeft != 17 || topRight != 13 || bottomLeft != 13 || bottomRight != 17) return 0;

    topLeft = -20; topRight = -10; bottomLeft = -10; bottomRight = -20;
    if (JxrInverseTransformMathApplyConditionalDcCompensation(
        &topLeft, &topRight, &bottomLeft, &bottomRight, -6, 21, FALSE) != -6) return 0;
    if (topLeft != -17 || topRight != -13 || bottomLeft != -13 || bottomRight != -17) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    if (JxrInverseTransformMathApplyConditionalDcCompensation(
        &topLeft, &topRight, &bottomLeft, &bottomRight, 50, 21, FALSE) != 50) return 0;
    if (topLeft != 10 || topRight != 20 || bottomLeft != 30 || bottomRight != 40) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyPost4(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != 19 || topRight != 33 || bottomLeft != 28 || bottomRight != 42) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyAlternatePost4(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != 25 || topRight != 37 || bottomLeft != 34 || bottomRight != 46) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyHadamardScale4(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != 38 || topRight != 35 || bottomLeft != 45 || bottomRight != -13) return 0;

    topLeft = 10; topRight = 40;
    JxrInverseTransformMathApplyHadamardScale2(&topLeft, &topRight);
    if (topLeft != 44 || topRight != -7) return 0;

    topLeft = 1000; topRight = 1000;
    JxrInverseTransformMathApplyAlternateHadamardScale2(&topLeft, &topRight);
    if (topLeft != 2000 || topRight != 389) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyOddOdd(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != -2 || topRight != -2 || bottomLeft != -12 || bottomRight != 52) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyOddOddPost(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != -2 || topRight != 1 || bottomLeft != 11 || bottomRight != 52) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyOdd(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != -6 || topRight != 38 || bottomLeft != -6 || bottomRight != -37) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyScaledDct2x2Down(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != 100 || topRight != -40 || bottomLeft != -20 || bottomRight != 0) return 0;

    topLeft = 10; topRight = 40;
    JxrInverseTransformMathApplyPost2(&topLeft, &topRight);
    if (topLeft != 20 || topRight != 44) return 0;

    topLeft = 10; topRight = 40;
    JxrInverseTransformMathApplyAlternatePost2(&topLeft, &topRight);
    if (topLeft != 33 || topRight != 51) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyPost2x2(&topLeft, &topRight, &bottomLeft, &bottomRight);
    if (topLeft != 26 || topRight != 37 || bottomLeft != 47 || bottomRight != 56) return 0;

    topLeft = 10; topRight = 20; bottomLeft = 30; bottomRight = 40;
    JxrInverseTransformMathApplyAlternatePost2x2(&topLeft, &topRight, &bottomLeft, &bottomRight);
    return topLeft == 26 && topRight == 37 && bottomLeft == 47 && bottomRight == 57;
}

static int test_transform_math_dct2x2_vectors(void)
{
    PixelI first, second, third, fourth;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrTransformMathApplyDct2x2Down(&first, &second, &third, &fourth);
    if (first != 50 || second != -20 || third != -10 || fourth != 0) return 0;

    first = 0; second = 0; third = 1; fourth = 0;
    JxrTransformMathApplyDct2x2Down(&first, &second, &third, &fourth);
    if (first != 1 || second != -1 || third != 0 || fourth != -1) return 0;

    first = 0; second = 0; third = 1; fourth = 0;
    JxrTransformMathApplyDct2x2Up(&first, &second, &third, &fourth);
    return first == 0 && second == 0 && third == 1 && fourth == 0;
}

static int test_forward_transform_math_vectors(void)
{
    PixelI first, second;
    PixelI third, fourth;
    PixelI samples[256];
    Int sampleIndex;

    first = 10; second = 5;
    JxrForwardTransformMathRotateHalf(&first, &second);
    if (first != 10 || second != 0) return 0;
    first = -10; second = 5;
    JxrForwardTransformMathRotateHalf(&first, &second);
    if (first != -5 || second != 10) return 0;
    first = 10; second = 5;
    JxrForwardTransformMathRotateThreeEighths(&first, &second);
    if (first != 10 || second != 1) return 0;
    first = -10; second = 5;
    JxrForwardTransformMathRotateThreeEighths(&first, &second);
    if (first != -7 || second != 9) return 0;

    for (sampleIndex = 0; sampleIndex < 256; ++sampleIndex) {
        samples[sampleIndex] = sampleIndex - 128;
    }
    JxrForwardTransformMathNormalizeBlock(samples, FALSE, 256, 16);
    if (samples[0] != -128 || samples[16] != -112 || samples[17] != -111) return 0;
    JxrForwardTransformMathNormalizeBlock(samples, TRUE, 256, 16);
    if (samples[0] != -64 || samples[16] != -56 || samples[240] != 56 ||
        samples[17] != -111) return 0;
    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyDct2x2Down(&first, &second, &third, &fourth);
    if (first != 25 || second != -10 || third != -5 || fourth != 0) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyDct2x2Down(&first, &second, &third, &fourth);
    if (first != -1 || second != -2 || third != -1 || fourth != -6) return 0;

    first = 10; second = 20;
    JxrForwardTransformMathApplyPre2(&first, &second);
    if (first != 1 || second != 17) return 0;
    first = -10; second = 5;
    JxrForwardTransformMathApplyPre2(&first, &second);
    if (first != -14 || second != 10) return 0;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyPre2x2(&first, &second, &third, &fourth);
    if (first != 0 || second != 9 || third != 20 || fourth != 30) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyPre2x2(&first, &second, &third, &fourth);
    if (first != -14 || second != 9 || third != 11 || fourth != -7) return 0;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyPre4(&first, &second, &third, &fourth);
    if (first != 0 || second != 1 || third != 34 || fourth != 35) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyPre4(&first, &second, &third, &fourth);
    if (first != -7 || second != 2 || third != 7 || fourth != -1) return 0;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyHst4(&first, &second, &third, &fourth);
    if (first != 41 || second != -20 || third != -10 || fourth != 25) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyHst4(&first, &second, &third, &fourth);
    if (first != -15 || second != -5 || third != -3 || fourth != 6) return 0;

    first = 10; fourth = 40;
    JxrForwardTransformMathApplyHst1(&first, &fourth);
    if (first != 37 || fourth != -41) return 0;
    first = -10; fourth = 5;
    JxrForwardTransformMathApplyHst1(&first, &fourth);
    if (first != 1 || fourth != -13) return 0;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyOddOdd(&first, &second, &third, &fourth);
    if (first != -2 || second != -2 || third != -12 || fourth != 52) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyOddOdd(&first, &second, &third, &fourth);
    if (first != -13 || second != -2 || third != -4 || fourth != 0) return 0;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyOddOddPre(&first, &second, &third, &fourth);
    if (first != 30 || second != 24 || third != 34 || fourth != 20) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyOddOddPre(&first, &second, &third, &fourth);
    if (first != -5 || second != 6 || third != 8 || fourth != -8) return 0;

    first = 10; second = 20; third = 30; fourth = 40;
    JxrForwardTransformMathApplyOdd(&first, &second, &third, &fourth);
    if (first != 35 || second != 10 || third != -4 || fourth != -39) return 0;
    first = -10; second = 5; third = 7; fourth = -3;
    JxrForwardTransformMathApplyOdd(&first, &second, &third, &fourth);
    return first == -2 && second == 7 && third == 10 && fourth == -4;
}

static int test_forward_transform_stage_vectors(void)
{
    static const PixelI stage1Expected[16] = {
        2, -4, -2, 0, 0, -14, 6, 0, 1, 2, -8, -1, 0, 0, 0, 0
    };
    PixelI stage1[16];
    PixelI stage2[256];
    Int sampleIndex;

    for (sampleIndex = 0; sampleIndex < 16; ++sampleIndex) {
        stage1[sampleIndex] = sampleIndex - 7;
    }
    JxrForwardTransformStagesApplyStage1Dct(stage1);
    if (memcmp(stage1, stage1Expected, sizeof(stage1)) != 0) return 0;

    for (sampleIndex = 0; sampleIndex < 256; ++sampleIndex) {
        stage2[sampleIndex] = sampleIndex - 128;
    }
    JxrForwardTransformStagesApplyStage2Dct(stage2);

    return stage2[0] == -32 && stage2[16] == 0 && stage2[32] == -268 &&
        stage2[48] == 0 && stage2[64] == 0 && stage2[80] == 0 &&
        stage2[128] == -67 && stage2[240] == 0;
}

static int test_forward_transform_prestage_vectors(void)
{
    PixelI firstStage[128];
    PixelI secondStage[128];
    Int sampleIndex;

    for (sampleIndex = 0; sampleIndex < 128; ++sampleIndex) {
        firstStage[sampleIndex] = sampleIndex - 64;
        secondStage[sampleIndex] = 96 - sampleIndex;
    }
    JxrForwardTransformStagesApplyPreStage1Split(firstStage + 16, secondStage + 16, 16);

    return firstStage[28] == -80 && firstStage[29] == -49 &&
        firstStage[72] == 25 && firstStage[73] == -3 &&
        secondStage[20] == 108 && secondStage[21] == 77 &&
        secondStage[64] == 2 && secondStage[65] == 30;
}

static int test_forward_transform_prestage2_vectors(void)
{
    PixelI firstStage[320];
    PixelI secondStage[320];
    Int sampleIndex;

    for (sampleIndex = 0; sampleIndex < 320; ++sampleIndex) {
        firstStage[sampleIndex] = sampleIndex - 160;
        secondStage[sampleIndex] = 192 - sampleIndex;
    }
    JxrForwardTransformStagesApplyPreStage2Split(firstStage + 144, secondStage + 144);

    return firstStage[48] == -70 && firstStage[64] == -171 &&
        firstStage[128] == -142 && firstStage[256] == 126 &&
        secondStage[16] == 226 && secondStage[32] == 104 &&
        secondStage[144] == -43 && secondStage[224] == -31;
}

static int test_forward_hard_tile_boundary_state_vectors(void)
{
    const U32 verticalColumns[2] = { 0, 2 };
    const U32 horizontalRows[2] = { 0, 3 };
    JxrForwardHardTileBoundaryConfiguration configuration;
    JxrForwardHardTileBoundaryState state;
    JxrForwardHardTileBoundaryState next;

    memset(&configuration, 0, sizeof(configuration));
    memset(&state, 0, sizeof(state));
    configuration.enabled = TRUE;
    configuration.verticalSliceCountMinusOne = 1;
    configuration.horizontalSliceCountMinusOne = 1;
    configuration.verticalSliceColumns = verticalColumns;
    configuration.horizontalSliceRows = horizontalRows;

    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 0, 0);
    if (next.tileX != 0 || next.tileY != 0 || next.isVerticalBoundary ||
        next.isHorizontalBoundary || next.previousMacroblockX != 0 || next.previousMacroblockY != 0) return 0;
    state = next;
    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 1, 0);
    if (!next.isOneMacroblockLeftOfVerticalBoundary || next.isVerticalBoundary || next.tileY != 0) return 0;
    state = next;
    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 2, 0);
    if (!next.isVerticalBoundary || next.tileY != 1 ||
        next.isOneMacroblockLeftOfVerticalBoundary || next.isOneMacroblockRightOfVerticalBoundary) return 0;
    state = next;
    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 3, 0);
    if (!next.isOneMacroblockRightOfVerticalBoundary || next.isVerticalBoundary || next.tileY != 1) return 0;
    state = next;
    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 0, 2);
    if (next.isHorizontalBoundary || next.tileX != 0) return 0;
    state = next;
    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 0, 3);
    if (!next.isHorizontalBoundary || next.tileX != 1) return 0;

    configuration.enabled = FALSE;
    state = next;
    JxrForwardHardTileBoundaryStateCalculate(&next, &state, &configuration, 4, 4);
    return !next.isVerticalBoundary && !next.isHorizontalBoundary &&
        !next.isOneMacroblockLeftOfVerticalBoundary && !next.isOneMacroblockRightOfVerticalBoundary &&
        next.previousMacroblockX == 4 && next.previousMacroblockY == 4;
}

static int test_forward_transform_macroblock_geometry_vectors(void)
{
    JxrForwardTransformMacroblockGeometry geometry;

    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_420, 0, 0, 4, 7, 3);
    if (geometry.overlap != OL_TWO || geometry.colorFormat != YUV_420 ||
        !geometry.isLeft || geometry.isRight || !geometry.isTop || geometry.isBottom ||
        !geometry.isTopOrBottom || !geometry.isLeftOrRight || !geometry.isTopOrLeft ||
        geometry.isLeftAdjacentColumn || geometry.isRightAdjacentColumn ||
        geometry.fullResolutionPlaneCount != 1) return 0;

    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_ONE, YUV_444, 3, 7, 4, 7, 4);
    if (geometry.overlap != OL_ONE || geometry.colorFormat != YUV_444 ||
        geometry.isLeft || geometry.isRight || geometry.isTop || !geometry.isBottom ||
        !geometry.isTopOrBottom || geometry.isLeftOrRight || geometry.isTopOrLeft ||
        geometry.isLeftAdjacentColumn || !geometry.isRightAdjacentColumn ||
        geometry.fullResolutionPlaneCount != 4) return 0;

    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_NONE, YUV_422, 4, 2, 4, 7, 3);
    return geometry.isRight && !geometry.isBottom && geometry.isLeftOrRight &&
        !geometry.isRightAdjacentColumn && geometry.fullResolutionPlaneCount == 1;
}

static int test_forward_transform_boundary_context_vectors(void)
{
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext context;

    memset(&hardTileState, 0, sizeof(hardTileState));
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_NONE, YUV_444, 2, 3, 4, 7, 3);
    JxrForwardTransformBoundaryContextInitialize(&context, &geometry, &hardTileState);
    if (context.isVerticalTileBoundary || context.isHorizontalTileBoundary ||
        context.hasTopBoundary || context.hasBottomBoundary ||
        context.hasLeftBoundary || context.hasRightBoundary ||
        context.hasTopOrBottomBoundary || context.hasLeftOrRightBoundary ||
        context.isLeftAdjacentToVerticalBoundary ||
        context.isRightAdjacentToVerticalBoundary) return 0;

    hardTileState.isVerticalBoundary = TRUE;
    hardTileState.isHorizontalBoundary = TRUE;
    hardTileState.isOneMacroblockRightOfVerticalBoundary = TRUE;
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_444, 3, 2, 4, 7, 3);
    JxrForwardTransformBoundaryContextInitialize(&context, &geometry, &hardTileState);
    return context.isVerticalTileBoundary && context.isHorizontalTileBoundary &&
        context.hasTopBoundary && context.hasBottomBoundary &&
        context.hasLeftBoundary && context.hasRightBoundary &&
        context.hasTopOrBottomBoundary && context.hasLeftOrRightBoundary &&
        context.isLeftAdjacentToVerticalBoundary &&
        context.isRightAdjacentToVerticalBoundary;
}

static int test_forward_transform_codec_setup_vectors(void)
{
    CWMImageStrCodec codec;
    JxrForwardTransformCodecSetup setup;

    memset(&codec, 0, sizeof(codec));
    codec.WMISCP.olOverlap = OL_TWO;
    codec.m_param.cfColorFormat = YUV_420;
    codec.m_param.cNumChannels = 3;
    codec.m_param.bScaledArith = TRUE;
    codec.cColumn = 0;
    codec.cRow = 0;
    codec.cmbWidth = 4;
    codec.cmbHeight = 7;
    codec.bVertTileBoundary = TRUE;
    codec.bHoriTileBoundary = TRUE;

    JxrForwardTransformCodecSetupInitialize(&setup, &codec);

    return setup.geometry.overlap == OL_TWO &&
        setup.geometry.colorFormat == YUV_420 &&
        setup.geometry.fullResolutionPlaneCount == 1 &&
        setup.geometry.isLeft && setup.geometry.isTop &&
        setup.boundaries.hasLeftBoundary && setup.boundaries.hasTopBoundary &&
        !setup.hardTileState.isVerticalBoundary &&
        !setup.hardTileState.isHorizontalBoundary &&
        setup.usesScaledArithmetic &&
        !codec.bVertTileBoundary && !codec.bHoriTileBoundary;
}

static int test_forward_transform_plane_plan_vectors(void)
{
    JxrForwardTransformPlanePlan plan;

    JxrForwardTransformPlanePlanInitialize(&plan, YUV_444, 3);
    if (plan.fullResolutionChannelCount != 3 ||
        plan.chroma420ChannelCount != 0 || plan.chroma422ChannelCount != 0) return 0;

    JxrForwardTransformPlanePlanInitialize(&plan, YUV_420, 1);
    if (plan.fullResolutionChannelCount != 1 ||
        plan.chroma420ChannelCount != 2 || plan.chroma422ChannelCount != 0) return 0;

    JxrForwardTransformPlanePlanInitialize(&plan, YUV_422, 1);
    return plan.fullResolutionChannelCount == 1 &&
        plan.chroma420ChannelCount == 0 && plan.chroma422ChannelCount == 2;
}

static int test_forward_transform_plane_context_vectors(void)
{
    PixelI first0, first1, first2;
    PixelI second0, second1, second2;
    PixelI* firstPlanes[3] = { &first0, &first1, &first2 };
    PixelI* secondPlanes[3] = { &second0, &second1, &second2 };
    PixelI predictionBefore[MAX_CHANNELS][2];
    PixelI predictionAfter[MAX_CHANNELS][2];
    JxrForwardTransformPlaneContext context;

    memset(predictionBefore, 0, sizeof(predictionBefore));
    memset(predictionAfter, 0, sizeof(predictionAfter));
    JxrForwardTransformPlaneContextInitializeFullResolution(&context,
        firstPlanes, secondPlanes, 2);
    if (context.firstStage != &first2 || context.secondStage != &second2 ||
        context.predictionBefore != NULL || context.predictionAfter != NULL ||
        context.channelIndex != 2 || context.isChroma) return 0;

    JxrForwardTransformPlaneContextInitializeChroma(&context,
        firstPlanes, secondPlanes, predictionBefore, predictionAfter, 1);
    return context.firstStage == &first2 && context.secondStage == &second2 &&
        context.predictionBefore == predictionBefore[1] &&
        context.predictionAfter == predictionAfter[1] &&
        context.channelIndex == 1 && context.isChroma;
}

static int test_encoder_macroblock_process_state_vectors(void)
{
    CWMImageStrCodec primaryCodec;
    CWMImageStrCodec secondaryCodec;
    JxrEncoderMacroblockProcessState state;

    memset(&primaryCodec, 0, sizeof(primaryCodec));
    JxrEncoderMacroblockProcessStateInitialize(&state, &primaryCodec);
    if (state.encodesPreviousMacroblock || state.processesSecondaryCodec ||
        state.previousMacroblockX != -1 || state.previousMacroblockY != -1) return 0;

    memset(&secondaryCodec, 0, sizeof(secondaryCodec));
    primaryCodec.cColumn = 5;
    primaryCodec.cRow = 3;
    primaryCodec.m_pNextSC = &secondaryCodec;
    JxrEncoderMacroblockProcessStateInitialize(&state, &primaryCodec);
    return state.encodesPreviousMacroblock && state.processesSecondaryCodec &&
        state.previousMacroblockX == 4 && state.previousMacroblockY == 2;
}

static int test_encoder_subband_plan_vectors(void)
{
    JxrEncoderSubbandPlan plan;

    JxrEncoderSubbandPlanInitialize(&plan, SB_ALL);
    if (!plan.encodesLowpass || !plan.encodesHighpass) return 0;

    JxrEncoderSubbandPlanInitialize(&plan, SB_NO_FLEXBITS);
    if (!plan.encodesLowpass || !plan.encodesHighpass) return 0;

    JxrEncoderSubbandPlanInitialize(&plan, SB_NO_HIGHPASS);
    if (!plan.encodesLowpass || plan.encodesHighpass) return 0;

    JxrEncoderSubbandPlanInitialize(&plan, SB_DC_ONLY);
    if (plan.encodesLowpass || plan.encodesHighpass) return 0;

    JxrEncoderSubbandPlanInitialize(&plan, SB_ISOLATED);
    return plan.encodesLowpass && plan.encodesHighpass;
}

static int test_encoder_packet_header_plan_vectors(void)
{
    JxrEncoderPacketHeaderPlan plan;

    JxrEncoderPacketHeaderPlanInitialize(&plan, SPATIAL, 1, TRUE,
        TRUE, TRUE, FALSE, FALSE);
    if (!plan.writesHeaders || !plan.usesSpatialLayout ||
        !plan.writesLowpassHeader || !plan.writesHighpassHeader ||
        plan.writesFlexbitsPacket || !plan.writesTrimFlexbits) return 0;

    JxrEncoderPacketHeaderPlanInitialize(&plan, FREQUENCY, 1, TRUE,
        TRUE, TRUE, FALSE, FALSE);
    if (!plan.writesHeaders || plan.usesSpatialLayout ||
        plan.writesLowpassHeader || plan.writesHighpassHeader ||
        plan.writesFlexbitsPacket || plan.writesTrimFlexbits) return 0;

    JxrEncoderPacketHeaderPlanInitialize(&plan, FREQUENCY, 3, FALSE,
        TRUE, TRUE, FALSE, FALSE);
    if (!plan.writesLowpassHeader || !plan.writesHighpassHeader ||
        plan.writesFlexbitsPacket || plan.writesTrimFlexbits) return 0;

    JxrEncoderPacketHeaderPlanInitialize(&plan, FREQUENCY, 4, TRUE,
        TRUE, TRUE, FALSE, FALSE);
    if (!plan.writesFlexbitsPacket || !plan.writesTrimFlexbits) return 0;

    JxrEncoderPacketHeaderPlanInitialize(&plan, SPATIAL, 4, TRUE,
        FALSE, TRUE, FALSE, FALSE);
    if (plan.writesHeaders) return 0;
    JxrEncoderPacketHeaderPlanInitialize(&plan, SPATIAL, 4, TRUE,
        TRUE, TRUE, TRUE, FALSE);
    if (plan.writesHeaders) return 0;
    JxrEncoderPacketHeaderPlanInitialize(&plan, SPATIAL, 4, TRUE,
        TRUE, TRUE, FALSE, TRUE);
    return !plan.writesHeaders;
}

static int test_encoder_slice_finalization_plan_vectors(void)
{
    CWMImageStrCodec codec;
    CWMImageStrCodec secondaryCodec;
    JxrEncoderSliceFinalizationPlan plan;

    memset(&codec, 0, sizeof(codec));
    codec.cmbWidth = 4;
    codec.cmbHeight = 5;
    codec.cTileRow = 0;
    codec.WMISCP.cNumOfSliceMinus1H = 1;
    codec.WMISCP.uiTileY[1] = 3;

    JxrEncoderSliceFinalizationPlanInitialize(&plan, &codec, 2, 2);
    if (plan.completesHorizontalSlice || plan.updatesPacketIndex ||
        plan.resetsCodingContexts) return 0;

    JxrEncoderSliceFinalizationPlanInitialize(&plan, &codec, 3, 4);
    if (!plan.completesHorizontalSlice || !plan.updatesPacketIndex ||
        plan.resetsCodingContexts) return 0;

    JxrEncoderSliceFinalizationPlanInitialize(&plan, &codec, 3, 2);
    if (!plan.completesHorizontalSlice || !plan.updatesPacketIndex ||
        !plan.resetsCodingContexts) return 0;

    memset(&secondaryCodec, 0, sizeof(secondaryCodec));
    codec.m_pNextSC = &secondaryCodec;
    JxrEncoderSliceFinalizationPlanInitialize(&plan, &codec, 3, 2);
    if (plan.updatesPacketIndex) return 0;

    codec.m_bSecondary = TRUE;
    JxrEncoderSliceFinalizationPlanInitialize(&plan, &codec, 3, 2);
    return plan.updatesPacketIndex && plan.resetsCodingContexts;
}

static int test_encoder_tile_header_plan_vectors(void)
{
    JxrEncoderTileHeaderPlan plan;

    JxrEncoderTileHeaderPlanInitialize(&plan, 0, SB_ALL);
    if (plan.writesDcQuantizer || plan.writesLpQuantizer ||
        plan.writesHpQuantizer) return 0;

    JxrEncoderTileHeaderPlanInitialize(&plan, 7, SB_ALL);
    if (!plan.writesDcQuantizer || !plan.writesLpQuantizer ||
        !plan.writesHpQuantizer) return 0;

    JxrEncoderTileHeaderPlanInitialize(&plan, 7, SB_NO_HIGHPASS);
    if (!plan.writesDcQuantizer || !plan.writesLpQuantizer ||
        plan.writesHpQuantizer) return 0;

    JxrEncoderTileHeaderPlanInitialize(&plan, 7, SB_DC_ONLY);
    if (!plan.writesDcQuantizer || plan.writesLpQuantizer ||
        plan.writesHpQuantizer) return 0;

    JxrEncoderTileHeaderPlanInitialize(&plan, 2, SB_ALL);
    return !plan.writesDcQuantizer && plan.writesLpQuantizer &&
        !plan.writesHpQuantizer;
}

static int test_encoder_image_plane_header_quantizer_plan_vectors(void)
{
    JxrEncoderImagePlaneHeaderQuantizerPlan plan;

    JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(&plan, 0, SB_ALL);
    if (!plan.writesDcFrameQuantizer || !plan.writesLowpassSyntax ||
        !plan.lowpassUsesDcQuantizer || plan.writesLpFrameQuantizer ||
        !plan.writesHighpassSyntax || !plan.highpassUsesLpQuantizer ||
        plan.writesHpFrameQuantizer) return 0;

    JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(&plan, 0x604, SB_ALL);
    if (!plan.writesDcFrameQuantizer || plan.lowpassUsesDcQuantizer ||
        !plan.writesLpFrameQuantizer || plan.highpassUsesLpQuantizer ||
        plan.writesHpFrameQuantizer) return 0;

    JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(&plan, 0x600, SB_ALL);
    if (!plan.writesDcFrameQuantizer || plan.lowpassUsesDcQuantizer ||
        !plan.writesLpFrameQuantizer || plan.highpassUsesLpQuantizer ||
        !plan.writesHpFrameQuantizer) return 0;

    JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(&plan, 0, SB_NO_HIGHPASS);
    if (!plan.writesLowpassSyntax || plan.writesHighpassSyntax) return 0;

    JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(&plan, 0, SB_DC_ONLY);
    return !plan.writesLowpassSyntax && !plan.writesHighpassSyntax;
}

static int test_encoder_main_header_plan_vectors(void)
{
    JxrEncoderMainHeaderPlan plan;

    JxrEncoderMainHeaderPlanInitialize(&plan, 16, 32, 0, 0,
        0, 0, 0, 0, BD_1, TRUE, TRUE);
    if (!plan.usesAbbreviatedFields || plan.writesTiling ||
        plan.writesWindowing || !plan.usesAlternateOneBitDepth ||
        !plan.usesHardTileSubversion || plan.tileSizeBitCount != 8) return 0;

    JxrEncoderMainHeaderPlanInitialize(&plan, 4096, 16, 2, 1,
        1, 0, 0, 2, BD_8, FALSE, FALSE);
    return !plan.usesAbbreviatedFields && plan.writesTiling &&
        plan.writesWindowing && !plan.usesAlternateOneBitDepth &&
        !plan.usesHardTileSubversion && plan.tileSizeBitCount == 16;
}

static int test_encoder_index_table_plan_vectors(void)
{
    JxrEncoderIndexTablePlan plan;

    JxrEncoderIndexTablePlanInitialize(&plan, 3, 2, SPATIAL, FALSE, 4);
    if (plan.packetGroupCount != 1 || plan.entryCount != 9) return 0;

    JxrEncoderIndexTablePlanInitialize(&plan, 12, 1, FREQUENCY, FALSE, 4);
    if (plan.packetGroupCount != 1 || plan.entryCount != 24) return 0;

    JxrEncoderIndexTablePlanInitialize(&plan, 12, 1, FREQUENCY, TRUE, 4);
    return plan.packetGroupCount == 4 && plan.entryCount == 24;
}

static int test_encoder_packet_stream_plan_vectors(void)
{
    JxrEncoderPacketStreamPlan plan;

    JxrEncoderPacketStreamPlanInitialize(&plan, SPATIAL, FALSE, 1, 2, 3);
    if (!plan.usesSpatialLayout || plan.usesProgressiveFrequencyLayout ||
        plan.packetGroupCount != 1 || plan.horizontalTileCount != 3 ||
        plan.verticalTileCount != 4) return 0;

    JxrEncoderPacketStreamPlanInitialize(&plan, FREQUENCY, FALSE, 4, 1, 2);
    if (plan.usesSpatialLayout || plan.usesProgressiveFrequencyLayout ||
        plan.packetGroupCount != 1 || plan.subbandPacketCount != 4) return 0;

    JxrEncoderPacketStreamPlanInitialize(&plan, FREQUENCY, TRUE, 3, 0, 0);
    return !plan.usesSpatialLayout && plan.usesProgressiveFrequencyLayout &&
        plan.packetGroupCount == 3 && plan.horizontalTileCount == 1 &&
        plan.verticalTileCount == 1;
}

static int test_forward_full_resolution_plane_vectors(void)
{
    PixelI samples[1408];
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext boundaries;
    Int index;

    memset(samples, 0, sizeof(samples));
    memset(&hardTileState, 0, sizeof(hardTileState));
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_444, 2, 2, 4, 4, 3);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);
    JxrForwardTransformFullResolutionPlaneApply(samples + 512, samples + 800, FALSE,
        &geometry, &boundaries, TRUE);
    for (index = 0; index < (Int)(sizeof(samples) / sizeof(samples[0])); ++index)
        if (samples[index] != 0) return 0;

    hardTileState.isVerticalBoundary = TRUE;
    hardTileState.isHorizontalBoundary = TRUE;
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_444, 0, 0, 4, 4, 3);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);
    JxrForwardTransformFullResolutionPlaneApply(samples + 512, samples + 800, TRUE,
        &geometry, &boundaries, FALSE);
    for (index = 0; index < (Int)(sizeof(samples) / sizeof(samples[0])); ++index)
        if (samples[index] != 0) return 0;

    return TRUE;
}

static int test_forward_chroma_420_plane_vectors(void)
{
    PixelI samples[1024];
    PixelI predictionBefore[2] = { 0, 0 };
    PixelI predictionAfter[2] = { 0, 0 };
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext boundaries;
    Int index;

    memset(samples, 0, sizeof(samples));
    memset(&hardTileState, 0, sizeof(hardTileState));
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_420, 2, 2, 4, 4, 3);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);
    JxrForwardTransformChroma420PlaneApply(samples + 384, samples + 640,
        predictionBefore, predictionAfter, &geometry, &boundaries, TRUE);
    for (index = 0; index < (Int)(sizeof(samples) / sizeof(samples[0])); ++index)
        if (samples[index] != 0) return 0;

    hardTileState.isVerticalBoundary = TRUE;
    hardTileState.isHorizontalBoundary = TRUE;
    hardTileState.isOneMacroblockLeftOfVerticalBoundary = TRUE;
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_420, 0, 0, 4, 4, 3);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);
    JxrForwardTransformChroma420PlaneApply(samples + 384, samples + 640,
        predictionBefore, predictionAfter, &geometry, &boundaries, FALSE);
    for (index = 0; index < (Int)(sizeof(samples) / sizeof(samples[0])); ++index)
        if (samples[index] != 0) return 0;

    return predictionBefore[0] == 0 && predictionBefore[1] == 0 &&
        predictionAfter[0] == 0 && predictionAfter[1] == 0;
}

static int test_forward_chroma_422_plane_vectors(void)
{
    PixelI samples[1024];
    PixelI predictionBefore[2] = { 0, 0 };
    PixelI predictionAfter[2] = { 0, 0 };
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext boundaries;
    Int index;

    memset(samples, 0, sizeof(samples));
    memset(&hardTileState, 0, sizeof(hardTileState));
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_422, 2, 2, 4, 4, 3);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);
    JxrForwardTransformChroma422PlaneApply(samples + 384, samples + 640,
        predictionBefore, predictionAfter, &geometry, &boundaries, TRUE);
    for (index = 0; index < (Int)(sizeof(samples) / sizeof(samples[0])); ++index)
        if (samples[index] != 0) return 0;

    hardTileState.isVerticalBoundary = TRUE;
    hardTileState.isHorizontalBoundary = TRUE;
    hardTileState.isOneMacroblockLeftOfVerticalBoundary = TRUE;
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        OL_TWO, YUV_422, 0, 0, 4, 4, 3);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);
    JxrForwardTransformChroma422PlaneApply(samples + 384, samples + 640,
        predictionBefore, predictionAfter, &geometry, &boundaries, FALSE);
    for (index = 0; index < (Int)(sizeof(samples) / sizeof(samples[0])); ++index)
        if (samples[index] != 0) return 0;

    return predictionBefore[0] == 0 && predictionBefore[1] == 0 &&
        predictionAfter[0] == 0 && predictionAfter[1] == 0;
}

static int test_inverse_transform_corner_prediction_vectors(void)
{
    PixelI value = 12;

    strTransformSubtractCornerPrediction(&value, 5);
    if (value != 7) return 0;
    strTransformAddCornerPrediction(&value, -3);
    return value == 4;
}

static int test_four_butterfly_vectors(void)
{
    PixelI actual[16];
    PixelI secondStageActual[256];
    const PixelI expected[16] = { -2, 0, 2, 4, -8, -8, -8, -8,
        -4, -4, -4, -4, 0, 0, 0, 0 };
    Int index;

    for (index = 0; index < 16; ++index) actual[index] = index - 7;
    JxrTransformMathApplyFourButterfly(actual, JxrTransformFirstStageFourButterflyOffsets);
    if (memcmp(actual, expected, sizeof(actual)) != 0) return 0;

    for (index = 0; index < 256; ++index) secondStageActual[index] = index;
    JxrTransformMathApplyFourButterfly(secondStageActual, JxrTransformSecondStageFourButterflyOffsets);
    return secondStageActual[0] == 240 && secondStageActual[192] == -48 &&
        secondStageActual[48] == -192 && secondStageActual[240] == 0 &&
        secondStageActual[64] == 240 && secondStageActual[128] == -48 &&
        secondStageActual[112] == -64 && secondStageActual[176] == 0 &&
        secondStageActual[16] == 240 && secondStageActual[208] == -16 &&
        secondStageActual[32] == -192 && secondStageActual[224] == 0 &&
        secondStageActual[80] == 240 && secondStageActual[144] == -16 &&
        secondStageActual[96] == -64 && secondStageActual[160] == 0 &&
        secondStageActual[1] == 1;
}

static int test_inverse_transform_dc_clip_vectors(void)
{
    return JxrInverseTransformMathClipDcWithAlternate(0, 7) == 0 &&
        JxrInverseTransformMathClipDcWithAlternate(7, 3) == 3 &&
        JxrInverseTransformMathClipDcWithAlternate(3, 7) == 3 &&
        JxrInverseTransformMathClipDcWithAlternate(7, -3) == 0 &&
        JxrInverseTransformMathClipDcWithAlternate(-7, -3) == -3 &&
        JxrInverseTransformMathClipDcWithAlternate(-3, -7) == -3 &&
        JxrInverseTransformMathClipDcWithAlternate(-7, 3) == 0;
}

static int test_entropy_reader_signed_residual_vectors(void)
{
    return JxrEntropyBitReaderDecodeSignedResidualValue(0) == 0 &&
        JxrEntropyBitReaderDecodeSignedResidualValue(1) == 0 &&
        JxrEntropyBitReaderDecodeSignedResidualValue(2) == 1 &&
        JxrEntropyBitReaderDecodeSignedResidualValue(3) == -1 &&
        JxrEntropyBitReaderDecodeSignedResidualValue(14) == 7 &&
        JxrEntropyBitReaderDecodeSignedResidualValue(15) == -7;
}

static int test_entropy_reader_state_vectors(void)
{
    U8 data[2] = { 0, 0 };
    BitIOInfo input;
    JxrEntropyBitReader reader;
    JxrEntropyBitReader sameStreamReader;

    memset(&input, 0, sizeof(input));
    input.uiAccumulator = 0xa0000000U;
    input.iMask = -2;
    input.pbCurrent = data;
    JxrEntropyBitReaderInit(&reader, &input);
    JxrEntropyBitReaderInit(&sameStreamReader, &input);

    if (JxrEntropyBitReaderPeek(&reader, 4) != 10 ||
        JxrEntropyBitReaderPosition(&reader) != 0 ||
        JxrEntropyBitReaderHasError(&reader)) return 0;
    if (JxrEntropyBitReaderRead(&reader, 4) != 10 ||
        JxrEntropyBitReaderPosition(&reader) != 4) return 0;
    JxrEntropyBitReaderConsume(&reader, 2);
    if (JxrEntropyBitReaderPosition(&reader) != 6 ||
        !JxrEntropyBitReaderSharesStream(&reader, &sameStreamReader)) return 0;
    return JxrEntropyBitReaderPeek(&reader, 17) == 0 &&
        JxrEntropyBitReaderHasError(&reader) &&
        JxrEntropyBitReaderPosition(&reader) == 6;
}

static int test_legacy_bit_reader_mirror_vectors(void)
{
    union { U64 alignment; U8 bytes[PACKETLENGTH * 2 + sizeof(BitIOInfo)]; } storage;
    BitIOInfo* input = (BitIOInfo*)(storage.bytes + PACKETLENGTH * 2);
    JxrLegacyBitReaderAdapter adapter;

    memset(&storage, 0, sizeof(storage));
    input->pbStart = storage.bytes;
    input->pbCurrent = storage.bytes + PACKETLENGTH;
    input->iMask = -8192;
    input->offRef = 4096;
    input->uiShadow = 0x12345678U;
    JxrLegacyBitReaderAdapterInit(&adapter, input);
    if (!JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(&adapter) ||
        !JxrLegacyBitReaderAdapterHasMatchingRefillDecision(&adapter) ||
        !JxrLegacyBitReaderAdapterNeedsRefill(&adapter)) return 0;
    input->pbStart = storage.bytes + PACKETLENGTH;
    input->pbCurrent = storage.bytes + PACKETLENGTH;
    input->offRef = 8192;
    input->uiShadow = 0xabcdef01U;
    JxrLegacyBitReaderAdapterSyncInputBufferState(&adapter);
    return JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(&adapter) &&
        JxrLegacyBitReaderAdapterHasMatchingRefillDecision(&adapter) &&
        !JxrLegacyBitReaderAdapterNeedsRefill(&adapter);
}

static int test_legacy_bit_io_bridge_vectors(void)
{
    union { U64 alignment; U8 bytes[PACKETLENGTH * 2 + sizeof(BitIOInfo)]; } storage;
    BitIOInfo* legacy = (BitIOInfo*)(storage.bytes + PACKETLENGTH * 2);
    JxrBitCursorState cursor;
    JxrBitInputBufferState input;

    memset(&storage, 0, sizeof(storage));
    legacy->pbStart = storage.bytes;
    legacy->pbCurrent = storage.bytes + 2;
    legacy->uiAccumulator = 0xabcdef00U;
    legacy->cBitsUsed = 7;
    legacy->offRef = 4096;
    legacy->uiShadow = 0x12345678U;
    JxrLegacyBitIoBridgeRead(legacy, &cursor, &input);
    if (cursor.currentIndex != 2 || input.packetStartIndex != 0 ||
        input.currentIndex != 2 || !JxrLegacyBitIoBridgeCursorMatches(legacy, &cursor) ||
        !JxrLegacyBitIoBridgeInputMatches(legacy, &input)) return 0;
    cursor.currentIndex = 4;
    cursor.accumulator = 0x10203040U;
    cursor.usedBits = 3;
    JxrLegacyBitIoBridgeApplyCursor(legacy, &cursor);
    input.packetStartIndex = PACKETLENGTH;
    input.streamOffset = 8192;
    input.shadow = 0x87654321U;
    JxrLegacyBitIoBridgeApplyInput(legacy, &input);
    return legacy->pbStart == storage.bytes + PACKETLENGTH &&
        legacy->pbCurrent == storage.bytes + 4 && legacy->uiAccumulator == 0x10203040U &&
        legacy->cBitsUsed == 3 && legacy->offRef == 8192 && legacy->uiShadow == 0x87654321U;
}

static int test_legacy_bit_reader_authoritative_state_vectors(void)
{
    union { U64 alignment; U8 bytes[PACKETLENGTH * 2 + sizeof(BitIOInfo)]; } storage;
    BitIOInfo* legacy = (BitIOInfo*)(storage.bytes + PACKETLENGTH * 2);
    JxrLegacyBitReaderAdapter adapter;

    memset(&storage, 0, sizeof(storage));
    legacy->pbStart = storage.bytes;
    legacy->pbCurrent = storage.bytes;
    legacy->iMask = -8192;
    legacy->uiAccumulator = 0xa0000000U;
    JxrLegacyBitReaderAdapterInit(&adapter, legacy);
    if (JxrLegacyBitReaderAdapterPeek16(&adapter, 4) != 10) return 0;
    legacy->uiAccumulator = 0;
    if (JxrLegacyBitReaderAdapterPeek16(&adapter, 4) != 0) return 0;
    legacy->uiAccumulator = 0xa0000000U;
    JxrLegacyBitReaderAdapterSyncInputBufferState(&adapter);
    JxrLegacyBitReaderAdapterConsume16(&adapter, 4);
    if (adapter.core.cursor.usedBits != 4 || adapter.core.input.currentIndex != 0 ||
        legacy->cBitsUsed != 4 || !JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(&adapter)) return 0;
    return JxrLegacyBitReaderAdapterPeek16(&adapter, 4) == 0;
}

static int test_hp_coefficient_block_resolver(void)
{
    CWMImageStrCodec codec;
    JxrDecoderSubbandContext state;
    PixelI plane0[256], plane1[256], plane2[256];
    JxrCoefficientBuffer block;
    JxrHpBlockAddress address;

    memset(&codec, 0, sizeof(codec));
    memset(&state, 0, sizeof(state));
    codec.p1MBbuffer[0] = plane0;
    codec.p1MBbuffer[1] = plane1;
    codec.p1MBbuffer[2] = plane2;
    codec.m_param.cNumChannels = 3;
    state.codec = &codec;

    codec.m_param.cfColorFormat = YUV_444;
    JxrDecoderFormatStateInit(&state.formatState, &codec);
    JxrCoefficientPlaneStateInit(&state.coefficientPlanes, codec.p1MBbuffer, YUV_444, 3);
    address = JxrHpCoefficientBlockResolverResolveAddress(YUV_444, 1, 0, 0, 2);
    if (address.planeIndex != 1 || address.coefficientOffset != 16) return 0;
    address = JxrHpCoefficientBlockResolverResolveAddress(YUV_444, 0, 0, 0, 15);
    if (address.planeIndex != 0 || address.coefficientOffset != 240) return 0;
    block = JxrHpCoefficientBlockResolverResolve(&state, 0, 0, 0, 15);
    if (block.values != plane0 || block.offset != 240 || block.count != 16) return 0;
    block = JxrHpCoefficientBlockResolverResolve(&state, 1, 0, 0, 2);
    if (block.values != plane1 || block.offset != 16 || block.count != 16)
        return 0;

    codec.m_param.cfColorFormat = YUV_420;
    JxrDecoderFormatStateInit(&state.formatState, &codec);
    JxrCoefficientPlaneStateInit(&state.coefficientPlanes, codec.p1MBbuffer, YUV_420, 3);
    if (JxrCoefficientPlaneStateGetLength(&state.coefficientPlanes, 0) != 256 ||
        JxrCoefficientPlaneStateGetLength(&state.coefficientPlanes, 1) != 64 ||
        JxrCoefficientPlaneStateGetLength(&state.coefficientPlanes, 2) != 64) return 0;
    address = JxrHpCoefficientBlockResolverResolveAddress(YUV_420, 0, 4, 1, 0);
    if (address.planeIndex != 1 || address.coefficientOffset != 32) return 0;
    block = JxrHpCoefficientBlockResolverResolve(&state, 0, 4, 1, 0);
    if (block.values != plane1 || block.offset != 32 || block.count != 16)
        return 0;

    codec.m_param.cfColorFormat = YUV_422;
    JxrDecoderFormatStateInit(&state.formatState, &codec);
    JxrCoefficientPlaneStateInit(&state.coefficientPlanes, codec.p1MBbuffer, YUV_422, 3);
    if (JxrCoefficientPlaneStateGetLength(&state.coefficientPlanes, 0) != 256 ||
        JxrCoefficientPlaneStateGetLength(&state.coefficientPlanes, 1) != 128 ||
        JxrCoefficientPlaneStateGetLength(&state.coefficientPlanes, 2) != 128) return 0;
    address = JxrHpCoefficientBlockResolverResolveAddress(YUV_422, 0, 5, 1, 0);
    if (address.planeIndex != 1 || address.coefficientOffset != 96) return 0;
    block = JxrHpCoefficientBlockResolverResolve(&state, 0, 5, 1, 0);
    return block.values == plane1 && block.offset == 96 && block.count == 16;
}

static int test_decoder_format_snapshot_vectors(void)
{
    CWMImageStrCodec codec;
    CWMDecoderParameters parameters;
    CWMITile tiles[2];
    CWMIQuantizer highpass0[2], highpass1[2], highpass2[2];
    JxrDecoderFormatState state;

    memset(&codec, 0, sizeof(codec));
    memset(&parameters, 0, sizeof(parameters));
    memset(tiles, 0, sizeof(tiles));
    memset(highpass0, 0, sizeof(highpass0));
    memset(highpass1, 0, sizeof(highpass1));
    memset(highpass2, 0, sizeof(highpass2));
    codec.m_param.cfColorFormat = YUV_444;
    codec.m_param.cNumChannels = 3;
    codec.m_param.bTranscode = TRUE;
    codec.WMISCP.bfBitstreamFormat = FREQUENCY;
    codec.WMISCP.sbSubband = SB_ALL;
    codec.pTile = tiles;
    codec.cTileColumn = 1;
    tiles[1].cBitsLP = 2;
    tiles[1].cBitsHP = 3;
    tiles[1].cNumQPLP = 2;
    tiles[1].cNumQPHP = 2;
    highpass0[1].iQP = 101;
    highpass1[1].iQP = 202;
    highpass2[1].iQP = 303;
    tiles[1].pQuantizerHP[0] = highpass0;
    tiles[1].pQuantizerHP[1] = highpass1;
    tiles[1].pQuantizerHP[2] = highpass2;
    codec.m_bResetRGITotals = TRUE;
    codec.m_bResetContext = TRUE;
    parameters.bSkipFlexbits = TRUE;
    parameters.cThumbnailScale = 16;
    codec.m_Dparam = &parameters;
    JxrDecoderFormatStateInit(&state, &codec);

    if (JxrDecoderFormatStateGetColorFormat(&state) != YUV_444 ||
        JxrDecoderFormatStateGetChannelCount(&state) != 3 ||
        JxrDecoderFormatStateIsDcOnly(&state) ||
        !JxrDecoderFormatStateHasHighpass(&state) ||
        JxrDecoderTileStateGetLowpassQuantizerBits(JxrDecoderFormatStateGetCurrentTile(&state)) != 2 ||
        JxrDecoderTileStateGetHighpassQuantizerBits(JxrDecoderFormatStateGetCurrentTile(&state)) != 3 ||
        JxrDecoderTileStateGetLowpassQuantizerCount(JxrDecoderFormatStateGetCurrentTile(&state)) != 2 ||
        JxrDecoderTileStateGetHighpassQuantizerCount(JxrDecoderFormatStateGetCurrentTile(&state)) != 2 ||
        JxrDecoderTileStateGetHighpassQuantizerParameter(JxrDecoderFormatStateGetCurrentTile(&state), 0, 1) != 101 ||
        JxrDecoderTileStateGetHighpassQuantizerParameter(JxrDecoderFormatStateGetCurrentTile(&state), 1, 1) != 202 ||
        JxrDecoderTileStateGetHighpassQuantizerParameter(JxrDecoderFormatStateGetCurrentTile(&state), 2, 1) != 303 ||
        !JxrDecoderFormatStateShouldResetScan(&state) ||
        !JxrDecoderFormatStateShouldResetContext(&state) ||
        !JxrDecoderFormatStateIsTranscode(&state) ||
        !JxrDecoderFormatStateHasFlexbits(&state) ||
        !JxrDecoderFormatStateShouldSkipFlexbits(&state) ||
        !JxrDecoderFormatStateShouldAdaptDcHuffman(&state)) return 0;

    codec.m_param.cfColorFormat = Y_ONLY;
    codec.m_param.cNumChannels = 1;
    codec.m_param.bTranscode = FALSE;
    codec.cTileColumn = 0;
    tiles[1].cBitsHP = 0;
    highpass0[1].iQP = 1;
    codec.m_bResetRGITotals = FALSE;
    codec.m_bResetContext = FALSE;
    parameters.bSkipFlexbits = FALSE;
    parameters.cThumbnailScale = 1;
    return JxrDecoderFormatStateGetColorFormat(&state) == YUV_444 &&
        JxrDecoderFormatStateGetChannelCount(&state) == 3 &&
        JxrDecoderTileStateGetHighpassQuantizerBits(JxrDecoderFormatStateGetCurrentTile(&state)) == 3 &&
        JxrDecoderTileStateGetHighpassQuantizerParameter(JxrDecoderFormatStateGetCurrentTile(&state), 0, 1) == 101 &&
        JxrDecoderFormatStateIsTranscode(&state) &&
        JxrDecoderFormatStateShouldResetScan(&state) &&
        JxrDecoderFormatStateShouldResetContext(&state) &&
        JxrDecoderFormatStateShouldSkipFlexbits(&state) &&
        JxrDecoderFormatStateShouldAdaptDcHuffman(&state);
}

static int test_decoder_subband_context(void)
{
    CWMImageStrCodec codec;
    CCodingContext entropy;
    BitIOInfo dcInput, lowpassInput, highpassInput, flexbitsInput;
    JxrDecoderSubbandContext state;
    memset(&codec, 0, sizeof(codec));
    memset(&entropy, 0, sizeof(entropy));
    entropy.m_pIODC = &dcInput;
    entropy.m_pIOLP = &lowpassInput;
    entropy.m_pIOAC = &highpassInput;
    entropy.m_pIOFL = &flexbitsInput;
    JxrDecoderSubbandContextInit(&state, &codec, &entropy);
    return state.codec == &codec &&
        !JxrEntropyBitReaderSharesStream(&state.dcReader, &state.lowpassReader) &&
        !JxrEntropyBitReaderSharesStream(&state.highpassReader, &state.flexbitsReader) &&
        JxrEntropyBitReaderPosition(&state.dcReader) == 0 &&
        JxrEntropyBitReaderPosition(&state.lowpassReader) == 0 &&
        !JxrEntropyBitReaderHasError(&state.highpassReader) &&
        !JxrEntropyBitReaderHasError(&state.flexbitsReader) &&
        JxrAdaptiveModelStateGetFlcBits(&state.dcModelState, 0) == entropy.m_aModelDC.m_iFlcBits[0] &&
        JxrAdaptiveModelStateGetFlcBits(&state.lowpassModelState, 0) == entropy.m_aModelLP.m_iFlcBits[0] &&
        JxrAdaptiveModelStateGetFlcBits(&state.highpassModelState, 0) == entropy.m_aModelAC.m_iFlcBits[0] &&
        JxrHuffmanStateSetGet(&state.huffmanStateSet, 0) == entropy.m_pAHexpt[0] &&
        JxrHighpassCbpStateGetPatternHuffman(&state.highpassCbpState) == entropy.m_pAdaptHuffCBPCY &&
        JxrHighpassCbpStateGetCountHuffman(&state.highpassCbpState) == entropy.m_pAdaptHuffCBPCY1 &&
        JxrHighpassCbpStateGetPredictionModel(&state.highpassCbpState) == &entropy.m_aCBPModel &&
        state.trimFlexBits == entropy.m_iTrimFlexBits &&
        JxrLowpassCbpStateGetZeroCount(&state.lowpassCbpState) == entropy.m_iCBPCountZero &&
        JxrLowpassCbpStateGetMaxCount(&state.lowpassCbpState) == entropy.m_iCBPCountMax &&
        JxrAdaptiveScanStateGetCoefficientIndex(&state.lowpassScanState, 1) == entropy.m_aScanLowpass[1].uScan &&
        JxrAdaptiveScanStateGetCoefficientIndex(&state.horizontalScanState, 1) == entropy.m_aScanHoriz[1].uScan &&
        JxrAdaptiveScanStateGetCoefficientIndex(&state.verticalScanState, 1) == entropy.m_aScanVert[1].uScan &&
        JxrMacroblockCbpStateGetCbp(&state.macroblockCbpState, 0) == codec.MBInfo.iCBP[0] &&
        JxrMacroblockCbpStateGetDifferential(&state.macroblockCbpState, 0) == codec.MBInfo.iDiffCBP[0];
}

static int test_decoder_subband_shared_reader_vectors(void)
{
    union { U64 alignment; U8 bytes[PACKETLENGTH * 2 + sizeof(BitIOInfo)]; } storage;
    BitIOInfo* input = (BitIOInfo*)(storage.bytes + PACKETLENGTH * 2);
    CWMImageStrCodec codec;
    CCodingContext entropy;
    JxrDecoderSubbandContext state;

    memset(&storage, 0, sizeof(storage));
    memset(&codec, 0, sizeof(codec));
    memset(&entropy, 0, sizeof(entropy));
    input->pbStart = storage.bytes;
    input->pbCurrent = storage.bytes;
    input->iMask = -8192;
    entropy.m_pIODC = input;
    entropy.m_pIOLP = input;
    entropy.m_pIOAC = input;
    entropy.m_pIOFL = input;
    JxrDecoderSubbandContextInit(&state, &codec, &entropy);
    return state.dcReader.sharedState == &state.dcSharedReaderState &&
        state.lowpassReader.sharedState == state.dcReader.sharedState &&
        state.highpassReader.sharedState == state.dcReader.sharedState &&
        state.flexbitsReader.sharedState == state.dcReader.sharedState &&
        JxrEntropyBitReaderSharesStream(&state.dcReader, &state.lowpassReader) &&
        JxrEntropyBitReaderSharesStream(&state.highpassReader, &state.flexbitsReader);
}

static int test_minimal_fixture(void)
{
    return files_equal("minimal-profile/minimal-gray-16x16.bmp",
                       "minimal-profile/minimal-gray-16x16-restored.bmp") &&
           files_equal("minimal-profile/minimal-gray-16x16.jxr",
                       "minimal-profile/trace/encoder-bitstream.jxr") &&
           files_equal("minimal-profile/trace/encoder-bitstream.jxr",
                       "minimal-profile/trace/decoder-bitstream.jxr");
}

static int test_minimal_round_trip(void)
{
    int result;
    result = system("cmd /c if not exist tests\\work mkdir tests\\work & "
        "jxrencoderdecoder\\Release\\JXREncApp\\x64\\JXREncApp.exe -i minimal-profile\\minimal-gray-16x16.bmp -o tests\\work\\minimal.jxr -c 2 -d 0 -q 1 -l 0 -f -X tests\\work\\trace & "
        "jxrencoderdecoder\\Release\\JXRDecApp\\x64\\JXRDecApp.exe -i tests\\work\\minimal.jxr -o tests\\work\\minimal.bmp -c 2 -a 0 -p 0 -X tests\\work\\trace");
    return result == 0 &&
        files_equal("minimal-profile/minimal-gray-16x16.bmp", "tests/work/minimal.bmp") &&
        files_equal("minimal-profile/minimal-gray-16x16.jxr", "tests/work/minimal.jxr") &&
        files_equal("tests/work/trace/encoder-bitstream.jxr", "tests/work/trace/decoder-bitstream.jxr") &&
        file_contains("tests/work/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_count\": 10") &&
        file_contains("tests/work/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_count\": 214") &&
        file_contains("tests/work/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_count\": 1958");
}

static int test_real_image_round_trip(void)
{
    int result;
    result = system("cmd /c if not exist tests\\work mkdir tests\\work & "
        "jxrencoderdecoder\\Release\\JXREncApp\\x64\\JXREncApp.exe -i real-image-profile\\test-sign-334x330.bmp -o tests\\work\\real-image.jxr -c 0 -d 3 -q 1 -l 0 -f -p & "
        "jxrencoderdecoder\\Release\\JXRDecApp\\x64\\JXRDecApp.exe -i tests\\work\\real-image.jxr -o tests\\work\\real-image.bmp -c 0 -a 0 -p 0");
    return result == 0 &&
        files_equal("real-image-profile/test-sign-334x330.bmp", "tests/work/real-image.bmp") &&
        files_equal("real-image-profile/test-sign-334x330-restored.bmp", "tests/work/real-image.bmp") &&
        files_equal("real-image-profile/test-sign-334x330.jxr", "tests/work/real-image.jxr");
}

static int test_default_image_round_trip(void)
{
    int result;
    result = system("cmd /c if not exist tests\\work mkdir tests\\work & "
        "jxrencoderdecoder\\Release\\JXREncApp\\x64\\JXREncApp.exe -i default-profile\\city-park-605x478.bmp -o tests\\work\\default-image.jxr -c 0 & "
        "jxrencoderdecoder\\Release\\JXRDecApp\\x64\\JXRDecApp.exe -i tests\\work\\default-image.jxr -o tests\\work\\default-image.bmp -c 0");
    return result == 0 &&
        files_equal("default-profile/city-park-605x478.bmp", "tests/work/default-image.bmp") &&
        files_equal("default-profile/city-park-605x478-restored.bmp", "tests/work/default-image.bmp") &&
        files_equal("default-profile/city-park-605x478.jxr", "tests/work/default-image.jxr");
}

static int test_bit_ranges(void)
{
    return file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_start\": 1320") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_end\": 1330") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_start\": 1330") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_end\": 1544") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_start\": 1544") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_end\": 3502");
}

static int test_entropy_trace(void)
{
    return file_contains("minimal-profile/trace/decoder-mb-000-000-after_ac_prediction.json", "\"cbp\": 65535") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_count\": 10") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_count\": 214") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_count\": 1958");
}

static int test_dc_conformance(void)
{
    return file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_start\": 1320") &&
        file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_end\": 1330") &&
        file_contains("minimal-profile/trace/decoder-mb-000-000-bitstream-dc.json", "\"bit_count\": 10");
}

static int test_lp_conformance(void)
{
    return file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_start\": 1330") &&
        file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_end\": 1544") &&
        file_contains("minimal-profile/trace/decoder-mb-000-000-bitstream-lp.json", "\"bit_count\": 214");
}

static int test_hp_conformance(void)
{
    return file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_start\": 1544") &&
        file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_end\": 3502") &&
        file_contains("minimal-profile/trace/decoder-mb-000-000-after_ac_prediction.json", "\"cbp\": 65535");
}

int main(int argc, char** argv)
{
    size_t i; int failed = 0;
    JxrTestCase tests[] = {
        { "smoke", test_smoke },
        { "bit_io_vectors", test_bit_io_vectors },
        { "adaptive_state", test_adaptive_state },
        { "adaptive_model_state_vectors", test_adaptive_model_state_vectors },
        { "huffman_state_set_vectors", test_huffman_state_set_vectors },
        { "highpass_cbp_state_vectors", test_highpass_cbp_state_vectors },
        { "macroblock_state_vectors", test_macroblock_state_vectors },
        { "coefficient_plane_state_vectors", test_coefficient_plane_state_vectors },
        { "macroblock_region_state_vectors", test_macroblock_region_state_vectors },
        { "transcode_tile_quantizer_state_vectors", test_transcode_tile_quantizer_state_vectors },
        { "transcode_quantizer_writer_vectors", test_transcode_quantizer_writer_vectors },
        { "transcode_tile_header_writer_vectors", test_transcode_tile_header_writer_vectors },
        { "transcode_orientation_state_vectors", test_transcode_orientation_state_vectors },
        { "transcode_coefficient_transform_vectors", test_transcode_coefficient_transform_vectors },
        { "transcode_coefficient_transform_422_vectors", test_transcode_coefficient_transform_422_vectors },
        { "transcode_coefficient_transform_420_vectors", test_transcode_coefficient_transform_420_vectors },
        { "transcode_tile_extraction_decision_vectors", test_transcode_tile_extraction_decision_vectors },
        { "transcode_roi_geometry_vectors", test_transcode_roi_geometry_vectors },
        { "decoder_tile_quantizer_syntax_vectors", test_decoder_tile_quantizer_syntax_vectors },
        { "main_header_reader_vectors", test_main_header_reader_vectors },
        { "header_state_applier_vectors", test_header_state_applier_vectors },
        { "header_validation_vectors", test_header_validation_vectors },
        { "header_stream_reader_vectors", test_header_stream_reader_vectors },
        { "stream_position_scope_vectors", test_stream_position_scope_vectors },
        { "header_metadata_finalizer_vectors", test_header_metadata_finalizer_vectors },
        { "header_decode_pipeline_vectors", test_header_decode_pipeline_vectors },
        { "decoder_initialization_pipeline_vectors", test_decoder_initialization_pipeline_vectors },
        { "secondary_plane_initializer_vectors", test_secondary_plane_initializer_vectors },
        { "image_plane_descriptor_reader_vectors", test_image_plane_descriptor_reader_vectors },
        { "image_plane_quantizer_header_reader_vectors", test_image_plane_quantizer_header_reader_vectors },
        { "decoder_dc_quantizer_header_applier_vectors", test_decoder_dc_quantizer_header_applier_vectors },
        { "decoder_lp_quantizer_header_applier_vectors", test_decoder_lp_quantizer_header_applier_vectors },
        { "decoder_hp_quantizer_header_applier_vectors", test_decoder_hp_quantizer_header_applier_vectors },
        { "decoder_tile_header_reader_vectors", test_decoder_tile_header_reader_vectors },
        { "decoder_coding_context_resetter_vectors", test_decoder_coding_context_resetter_vectors },
        { "decoder_packet_row_reader_vectors", test_decoder_packet_row_reader_vectors },
        { "inverse_color_transform_vectors", test_inverse_color_transform_vectors },
        { "postprocess_demacroblock_decision_vectors", test_postprocess_demacroblock_decision_vectors },
        { "postprocess_deblock_boundary_decision_vectors", test_postprocess_deblock_boundary_decision_vectors },
        { "postprocess_row_state_vectors", test_postprocess_row_state_vectors },
        { "postprocess_block_neighborhood_vectors", test_postprocess_block_neighborhood_vectors },
        { "postprocess_smoothing_vectors", test_postprocess_smoothing_vectors },
        { "postprocess_macroblock_analyzer_vectors", test_postprocess_macroblock_analyzer_vectors },
        { "postprocess_block_dc_collector_vectors", test_postprocess_block_dc_collector_vectors },
        { "postprocess_macroblock_neighborhood_vectors", test_postprocess_macroblock_neighborhood_vectors },
        { "postprocess_block_edge_applier_vectors", test_postprocess_block_edge_applier_vectors },
        { "sample_clipping_vectors", test_sample_clipping_vectors },
        { "float_sample_conversion_vectors", test_float_sample_conversion_vectors },
        { "monochrome_expansion_vectors", test_monochrome_expansion_vectors },
        { "monochrome_expansion_offset_vectors", test_monochrome_expansion_offset_vectors },
        { "monochrome_expansion_thumbnail_vectors", test_monochrome_expansion_thumbnail_vectors },
        { "decoder_roi_row_range_vectors", test_decoder_roi_row_range_vectors },
        { "variable_length_word_vectors", test_variable_length_word_vectors },
        { "index_table_reader_vectors", test_index_table_reader_vectors },
        { "decoder_stream_initializer_vectors", test_decoder_stream_initializer_vectors },
        { "decoder_bitstream_set_vectors", test_decoder_bitstream_set_vectors },
        { "decoder_packet_attachment_vectors", test_decoder_packet_attachment_vectors },
        { "decoder_packet_header_reader_vectors", test_decoder_packet_header_reader_vectors },
        { "packet_header_syntax_reader_vectors", test_packet_header_syntax_reader_vectors },
        { "bit_input_buffer_state_vectors", test_bit_input_buffer_state_vectors },
        { "packet_source_vectors", test_packet_source_vectors },
        { "packet_executor_vectors", test_packet_executor_vectors },
        { "packet_short_read_vectors", test_packet_short_read_vectors },
        { "bit_reader_core_vectors", test_bit_reader_core_vectors },
        { "refill_error_propagation_vectors", test_refill_error_propagation_vectors },
        { "bit_cursor_state_vectors", test_bit_cursor_state_vectors },
        { "bit_cursor_ring_wrap_vectors", test_bit_cursor_ring_wrap_vectors },
        { "explicit_entropy_context", test_explicit_entropy_context },
        { "adaptive_scan_vectors", test_adaptive_scan_vectors },
        { "adaptive_scan_state_vectors", test_adaptive_scan_state_vectors },
        { "coefficient_buffer_vectors", test_coefficient_buffer_vectors },
        { "huffman_decoder_vectors", test_huffman_decoder_vectors },
        { "lp_residual_vectors", test_lp_residual_vectors },
        { "lowpass_cbp_state_vectors", test_lowpass_cbp_state_vectors },
        { "macroblock_cbp_state_vectors", test_macroblock_cbp_state_vectors },
        { "bit_math_vectors", test_bit_math_vectors },
        { "prediction_math_vectors", test_prediction_math_vectors },
        { "inverse_transform_macroblock_geometry_vectors", test_inverse_transform_macroblock_geometry_vectors },
        { "hard_tile_boundary_state_vectors", test_hard_tile_boundary_state_vectors },
        { "inverse_transform_boundary_context_vectors", test_inverse_transform_boundary_context_vectors },
        { "inverse_postprocess_parameters_vectors", test_inverse_postprocess_parameters_vectors },
        { "inverse_highpass_parameters_vectors", test_inverse_highpass_parameters_vectors },
        { "inverse_transform_plane_plan_vectors", test_inverse_transform_plane_plan_vectors },
        { "inverse_transform_plane_buffers_vectors", test_inverse_transform_plane_buffers_vectors },
        { "inverse_transform_plane_context_vectors", test_inverse_transform_plane_context_vectors },
        { "inverse_transform_plane_stage2_vectors", test_inverse_transform_plane_stage2_vectors },
        { "inverse_transform_normalization_vectors", test_inverse_transform_normalization_vectors },
        { "inverse_transform_math_vectors", test_inverse_transform_math_vectors },
        { "transform_math_dct2x2_vectors", test_transform_math_dct2x2_vectors },
        { "forward_transform_math_vectors", test_forward_transform_math_vectors },
        { "forward_transform_stage_vectors", test_forward_transform_stage_vectors },
        { "forward_transform_prestage_vectors", test_forward_transform_prestage_vectors },
        { "forward_transform_prestage2_vectors", test_forward_transform_prestage2_vectors },
        { "forward_hard_tile_boundary_state_vectors", test_forward_hard_tile_boundary_state_vectors },
        { "forward_transform_macroblock_geometry_vectors", test_forward_transform_macroblock_geometry_vectors },
        { "forward_transform_boundary_context_vectors", test_forward_transform_boundary_context_vectors },
        { "forward_transform_codec_setup_vectors", test_forward_transform_codec_setup_vectors },
        { "forward_transform_plane_plan_vectors", test_forward_transform_plane_plan_vectors },
        { "forward_transform_plane_context_vectors", test_forward_transform_plane_context_vectors },
        { "encoder_macroblock_process_state_vectors", test_encoder_macroblock_process_state_vectors },
        { "encoder_subband_plan_vectors", test_encoder_subband_plan_vectors },
        { "encoder_packet_header_plan_vectors", test_encoder_packet_header_plan_vectors },
        { "encoder_slice_finalization_plan_vectors", test_encoder_slice_finalization_plan_vectors },
        { "encoder_tile_header_plan_vectors", test_encoder_tile_header_plan_vectors },
        { "encoder_image_plane_header_quantizer_plan_vectors", test_encoder_image_plane_header_quantizer_plan_vectors },
        { "encoder_main_header_plan_vectors", test_encoder_main_header_plan_vectors },
        { "encoder_index_table_plan_vectors", test_encoder_index_table_plan_vectors },
        { "encoder_packet_stream_plan_vectors", test_encoder_packet_stream_plan_vectors },
        { "forward_full_resolution_plane_vectors", test_forward_full_resolution_plane_vectors },
        { "forward_chroma_420_plane_vectors", test_forward_chroma_420_plane_vectors },
        { "forward_chroma_422_plane_vectors", test_forward_chroma_422_plane_vectors },
        { "inverse_transform_corner_prediction_vectors", test_inverse_transform_corner_prediction_vectors },
        { "four_butterfly_vectors", test_four_butterfly_vectors },
        { "inverse_transform_dc_clip_vectors", test_inverse_transform_dc_clip_vectors },
        { "entropy_reader_signed_residual_vectors", test_entropy_reader_signed_residual_vectors },
        { "entropy_reader_state_vectors", test_entropy_reader_state_vectors },
        { "legacy_bit_reader_mirror_vectors", test_legacy_bit_reader_mirror_vectors },
        { "legacy_bit_io_bridge_vectors", test_legacy_bit_io_bridge_vectors },
        { "legacy_bit_reader_authoritative_state_vectors", test_legacy_bit_reader_authoritative_state_vectors },
        { "hp_coefficient_block_resolver", test_hp_coefficient_block_resolver },
        { "decoder_format_snapshot_vectors", test_decoder_format_snapshot_vectors },
        { "decoder_subband_context", test_decoder_subband_context },
        { "decoder_subband_shared_reader_vectors", test_decoder_subband_shared_reader_vectors },
        { "minimal_fixture", test_minimal_fixture },
        { "minimal_round_trip", test_minimal_round_trip },
        { "real_image_round_trip", test_real_image_round_trip },
        { "default_image_round_trip", test_default_image_round_trip },
        { "bit_ranges", test_bit_ranges },
        { "entropy_trace", test_entropy_trace }
        ,{ "dc_conformance", test_dc_conformance }
        ,{ "lp_conformance", test_lp_conformance }
        ,{ "hp_conformance", test_hp_conformance }
    };
    const char* selected = argc == 2 ? argv[1] : NULL;
#ifdef _WIN32
    /* The solution starts this executable from jxrencoderdecoder/. */
    if (_access("minimal-profile", 0) != 0) _chdir("..");
#endif
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        if (selected && strcmp(selected, tests[i].name)) continue;
        if (tests[i].run()) printf("PASS %s\n", tests[i].name);
        else { printf("FAIL %s\n", tests[i].name); failed = 1; }
    }
    if (selected && !failed) {
        int found = 0; for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) if (!strcmp(selected, tests[i].name)) found = 1;
        if (!found) { printf("FAIL unknown test %s\n", selected); failed = 1; }
    }
    return failed;
}
