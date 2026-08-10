/* Self-contained conformance runner for the managed-port reference profile. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "JxrManagedBitIO.h"
#include "strcodec.h"
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
#include "JxrLegacyBitIoBridge.h"
#include "JxrPacketExecutor.h"
#include "JxrBitMath.h"
#include "JxrDecoderSubbandContext.h"
#include "JxrHpCoefficientBlockResolver.h"
#include "JxrEntropyReader.h"
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
    JxrMacroblockStateInit(&state, &macroblock);
    JxrMacroblockStateClearDc(&state, 2);
    if (JxrMacroblockStateGetDcCoefficients(&state, 0)[0] != 0 ||
        JxrMacroblockStateGetDcCoefficients(&state, 1)[15] != 0 ||
        JxrMacroblockStateGetDcCoefficients(&state, 2)[0] != -1) return 0;
    JxrMacroblockStateSetDcCoefficient(&state, 1, 5, 42);
    if (JxrMacroblockStateGetDcCoefficient(&state, 1, 5) != 42) return 0;
    JxrMacroblockStateResetQuantizerIndices(&state);
    macroblock.iOrientation = 1;
    JxrMacroblockStateSetLowpassQuantizerIndex(&state, 3);
    JxrMacroblockStateSetHighpassQuantizerIndex(&state, 7);
    return JxrMacroblockStateGetLowpassQuantizerIndex(&state) == 3 &&
        JxrMacroblockStateGetHighpassQuantizerIndex(&state) == 7 &&
        JxrMacroblockStateGetOrientation(&state) == 1;
}

static int test_coefficient_plane_state_vectors(void)
{
    PixelI plane0[32], plane1[32];
    PixelI* planes[2] = { plane0, plane1 };
    JxrCoefficientPlaneState state;
    JxrCoefficientBuffer block;

    JxrCoefficientPlaneStateInit(&state, planes);
    block = JxrCoefficientPlaneStateGetBlock(&state, 1, 4, 16);
    return JxrCoefficientPlaneStateGetPlane(&state, 0) == plane0 &&
        block.values == plane1 && block.offset == 4 && block.count == 16;
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
static Bool read_fake_packet(Void* context, size_t offset, U8* destination, size_t count)
{
    JxrFakePacketSource* source = (JxrFakePacketSource*)context;
    if (offset + count > source->length) return FALSE;
    memcpy(destination, source->data + offset, count); source->lastOffset = offset; return TRUE;
}

static int test_packet_source_vectors(void)
{
    U8 data[8192] = { 0 }, ring[8192] = { 0 };
    JxrFakePacketSource fake = { data, sizeof(data), 0 };
    JxrPacketSource source = { &fake, read_fake_packet };
    JxrBitInputBufferState state;
    data[4096] = 0x78; data[4097] = 0x56; data[4098] = 0x34; data[4099] = 0x12;
    JxrBitInputBufferStateInit(&state, ring, sizeof(ring), 0, 4096, 4096, 0);
    if (!JxrBitInputBufferStateReadPacket(&state, &source, 4096)) return 0;
    return fake.lastOffset == 4096 && state.streamOffset == 8192 &&
        state.shadow == 0x12345678U && state.packetStartIndex == 4096 && ring[0] == 0x78;
}

static int test_packet_executor_vectors(void)
{
    U8 data[8192] = { 0 }, ring[8192] = { 0 };
    JxrFakePacketSource fake = { data, sizeof(data), 0 };
    JxrPacketSource source = { &fake, read_fake_packet };
    JxrBitInputBufferState state;
    Bool didRefill;

    JxrBitInputBufferStateInit(&state, ring, sizeof(ring), 0, 0, 4096, 0x12345678U);
    if (!JxrPacketExecutorTryRefill(&state, &source, 4096, &didRefill) || didRefill)
        return 0;
    data[4096] = 0x78; data[4097] = 0x56; data[4098] = 0x34; data[4099] = 0x12;
    state.currentIndex = 4096;
    if (!JxrPacketExecutorTryRefill(&state, &source, 4096, &didRefill) || !didRefill)
        return 0;
    return fake.lastOffset == 4096 && ring[0] == 0x78 &&
        state.packetStartIndex == 4096 && state.streamOffset == 8192 &&
        state.shadow == 0x12345678U;
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
    return JxrMacroblockCbpStateGetCbp(&state, 0) == 0x1234 &&
        JxrMacroblockCbpStateGetCbp(&state, 1) == 0x3f &&
        JxrMacroblockCbpStateGetCbp(&state, 2) == 0x55 &&
        JxrMacroblockCbpStateGetCbp(&state, 15) == 0x7a &&
        JxrMacroblockCbpStateGetDifferential(&state, 0) == 0x4321 &&
        JxrMacroblockCbpStateGetDifferential(&state, 1) == 0x2a &&
        JxrMacroblockCbpStateGetDifferential(&state, 2) == 0x15;
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
    if (adapter.bitCursor.usedBits != 4 || adapter.inputBufferState.currentIndex != 0 ||
        legacy->cBitsUsed != 4 || !JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(&adapter)) return 0;
    return JxrLegacyBitReaderAdapterPeek16(&adapter, 4) == 0;
}

static int test_hp_coefficient_block_resolver(void)
{
    CWMImageStrCodec codec;
    JxrDecoderSubbandContext state;
    PixelI plane0[256], plane1[256], plane2[256];
    JxrCoefficientBuffer block;

    memset(&codec, 0, sizeof(codec));
    memset(&state, 0, sizeof(state));
    codec.p1MBbuffer[0] = plane0;
    codec.p1MBbuffer[1] = plane1;
    codec.p1MBbuffer[2] = plane2;
    state.codec = &codec;
    JxrCoefficientPlaneStateInit(&state.coefficientPlanes, codec.p1MBbuffer);

    codec.m_param.cfColorFormat = YUV_444;
    block = JxrHpCoefficientBlockResolverResolve(&state, 1, 0, 0, 2);
    if (block.values != plane1 || block.offset != 16 || block.count != 16)
        return 0;

    codec.m_param.cfColorFormat = YUV_420;
    block = JxrHpCoefficientBlockResolverResolve(&state, 0, 4, 1, 0);
    if (block.values != plane1 || block.offset != 32 || block.count != 16)
        return 0;

    codec.m_param.cfColorFormat = YUV_422;
    block = JxrHpCoefficientBlockResolverResolve(&state, 0, 5, 1, 0);
    return block.values == plane1 && block.offset == 96 && block.count == 16;
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
        { "bit_input_buffer_state_vectors", test_bit_input_buffer_state_vectors },
        { "packet_source_vectors", test_packet_source_vectors },
        { "packet_executor_vectors", test_packet_executor_vectors },
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
        { "decoder_subband_context", test_decoder_subband_context },
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
