/* Self-contained conformance runner for the managed-port reference profile. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "JxrManagedBitIO.h"
#include "strcodec.h"
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
    return hp.copyPrevious && hp.count == 1 && !JxrDecoderTileQuantizerSyntaxReadDc(&source, 0, &dc);
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
        { "decoder_dc_quantizer_header_applier_vectors", test_decoder_dc_quantizer_header_applier_vectors },
        { "decoder_lp_quantizer_header_applier_vectors", test_decoder_lp_quantizer_header_applier_vectors },
        { "decoder_hp_quantizer_header_applier_vectors", test_decoder_hp_quantizer_header_applier_vectors },
        { "decoder_tile_header_reader_vectors", test_decoder_tile_header_reader_vectors },
        { "decoder_coding_context_resetter_vectors", test_decoder_coding_context_resetter_vectors },
        { "inverse_color_transform_vectors", test_inverse_color_transform_vectors },
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
