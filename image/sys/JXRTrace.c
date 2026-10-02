#include "JXRTrace.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define JXR_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define JXR_MKDIR(path) mkdir(path, 0777)
#endif

static char g_szTraceDirectory[1024] = {0};
static size_t g_cRefillSnapshots = 0;

static Void JXRTraceMakePath(char* szPath, size_t cchPath, const char* szFile)
{
    snprintf(szPath, cchPath, "%s/%s", g_szTraceDirectory, szFile);
}

Void JXRTraceConfigure(const char* szDirectory)
{
    if (NULL == szDirectory || 0 == szDirectory[0])
    {
        g_szTraceDirectory[0] = 0;
        g_cRefillSnapshots = 0;
        return;
    }

    strncpy(g_szTraceDirectory, szDirectory, sizeof(g_szTraceDirectory) - 1);
    g_szTraceDirectory[sizeof(g_szTraceDirectory) - 1] = 0;
    g_cRefillSnapshots = 0;
    JXR_MKDIR(g_szTraceDirectory);
}

Bool JXRTraceEnabled(Void)
{
    return 0 != g_szTraceDirectory[0];
}

size_t JXRTraceBitPosition(const BitIOInfo* pIO, Bool bWrite)
{
    size_t cb;
    if (NULL == pIO) return 0;
    cb = bWrite ? pIO->offRef + (size_t)getSizeWrite((BitIOInfo*)pIO) : (size_t)getPosRead((BitIOInfo*)pIO);
    return cb * 8 + (pIO->cBitsUsed & 7);
}

Void JXRTraceDumpBitRange(const char* szMode, const char* szPacket, Int iMBX, Int iMBY,
    size_t cbitStart, size_t cbitEnd)
{
    char szFile[256], szPath[1200];
    FILE* pFile;
    size_t cbitCount = cbitEnd >= cbitStart ? cbitEnd - cbitStart : 0;

    if (!JXRTraceEnabled()) return;
    snprintf(szFile, sizeof(szFile), "%s-mb-%03d-%03d-bitstream-%s.json", szMode, (int)iMBX, (int)iMBY, szPacket);
    JXRTraceMakePath(szPath, sizeof(szPath), szFile);
    pFile = fopen(szPath, "wb");
    if (NULL == pFile) return;

    fprintf(pFile,
        "{\n  \"mode\": \"%s\",\n  \"packet\": \"%s\",\n"
        "  \"macroblock\": { \"x\": %d, \"y\": %d },\n"
        "  \"byte_start\": %lu,\n  \"bit_start\": %lu,\n"
        "  \"byte_end\": %lu,\n  \"bit_end\": %lu,\n"
        "  \"bit_count\": %lu,\n"
        "  \"binary_source\": \"%s-bitstream.jxr\"\n}\n",
        szMode, szPacket, (int)iMBX, (int)iMBY,
        (unsigned long)(cbitStart / 8), (unsigned long)cbitStart,
        (unsigned long)(cbitEnd / 8), (unsigned long)cbitEnd,
        (unsigned long)cbitCount, szMode);
    fclose(pFile);
}

Void JXRTraceCopyFile(const char* szSourceFile, const char* szTraceFile)
{
    char szPath[1200];
    FILE* pSource;
    FILE* pDestination;
    U8 buffer[8192];
    size_t cbRead;

    if (!JXRTraceEnabled() || NULL == szSourceFile || NULL == szTraceFile) return;
    pSource = fopen(szSourceFile, "rb");
    if (NULL == pSource) return;
    JXRTraceMakePath(szPath, sizeof(szPath), szTraceFile);
    pDestination = fopen(szPath, "wb");
    if (NULL == pDestination) { fclose(pSource); return; }
    while (0 != (cbRead = fread(buffer, 1, sizeof(buffer), pSource)))
        fwrite(buffer, 1, cbRead, pDestination);
    fclose(pDestination);
    fclose(pSource);
}

Void JXRTraceDumpRefillSnapshot(const BitIOInfo* before, const BitIOInfo* after,
    Bool adapterNeedsRefill, Bool legacyNeedsRefill, Bool executorDidRefill,
    Bool executorMatchesLegacy, UINTPTR_T executorStartAddress,
    UINTPTR_T executorCurrentAddress, size_t executorOffset, U32 executorShadow,
    ERR executorReadResult)
{
    char szPath[1200];
    FILE* pFile;
    Bool didRefill;
    const U8* packet;
    if (!JXRTraceEnabled()) return;
    didRefill = before->offRef != after->offRef;
    packet = before->pbStart;
    JXRTraceMakePath(szPath, sizeof(szPath), "decoder-refill-snapshots.jsonl");
    pFile = fopen(szPath, "ab");
    if (NULL == pFile) return;
    fprintf(pFile,
        "{\"sequence\":%lu,\"adapter_needs_refill\":%s,\"legacy_needs_refill\":%s,\"executor_did_refill\":%s,\"executor_read_result\":%d,\"did_refill\":%s,\"executor_matches_legacy\":%s,\"packet_first4\":[%u,%u,%u,%u],\"before\":{\"start\":\"%p\",\"current\":\"%p\",\"offset\":%lu,\"bits_used\":%u,\"shadow\":%u},"
        "\"after\":{\"start\":\"%p\",\"current\":\"%p\",\"offset\":%lu,\"bits_used\":%u,\"shadow\":%u},"
        "\"executor_after\":{\"start\":\"%p\",\"current\":\"%p\",\"offset\":%lu,\"shadow\":%u}}\n",
        (unsigned long)g_cRefillSnapshots++, adapterNeedsRefill ? "true" : "false",
        legacyNeedsRefill ? "true" : "false", executorDidRefill ? "true" : "false",
        (int)executorReadResult, didRefill ? "true" : "false", executorMatchesLegacy ? "true" : "false",
        didRefill ? (unsigned)packet[0] : 0, didRefill ? (unsigned)packet[1] : 0,
        didRefill ? (unsigned)packet[2] : 0, didRefill ? (unsigned)packet[3] : 0,
        before->pbStart, before->pbCurrent, (unsigned long)before->offRef, (unsigned)before->cBitsUsed, (unsigned)before->uiShadow,
        after->pbStart, after->pbCurrent, (unsigned long)after->offRef, (unsigned)after->cBitsUsed, (unsigned)after->uiShadow,
        (Void*)executorStartAddress, (Void*)executorCurrentAddress, (unsigned long)executorOffset, (unsigned)executorShadow);
    fclose(pFile);
}

static Void JXRTraceWriteValues(FILE* pFile, const PixelI* pValues)
{
    size_t i;
    fprintf(pFile, "[");
    for (i = 0; i < 256; ++i)
    {
        if (i != 0) fprintf(pFile, ",");
        fprintf(pFile, "%d", (int)pValues[i]);
    }
    fprintf(pFile, "]");
}

Void JXRTraceDumpValues(const char* szMode, const char* szStage, Int iMBX,
    Int iMBY, const char* szChannel, const PixelI* pValues)
{
    char szFile[256], szPath[1200];
    PixelI orderedValues[256];
    const PixelI* valuesToWrite = pValues;
    FILE* pFile;
    size_t row, column;

    if (!JXRTraceEnabled() || NULL == pValues || NULL == szChannel) return;
    if (0 == strcmp(szStage, "reconstructed_samples"))
    {
        /* Decoder storage is in idxCC order; emit natural row-major pixels. */
        for (row = 0; row < 16; ++row)
            for (column = 0; column < 16; ++column)
                orderedValues[row * 16 + column] =
                    pValues[idxCC[row][column]];
        valuesToWrite = orderedValues;
    }
    snprintf(szFile, sizeof(szFile), "%s-mb-%03d-%03d-%s-%s.json",
        szMode, (int)iMBX, (int)iMBY, szStage, szChannel);
    JXRTraceMakePath(szPath, sizeof(szPath), szFile);
    pFile = fopen(szPath, "wb");
    if (NULL == pFile) return;

    fprintf(pFile,
        "{\n  \"mode\": \"%s\",\n  \"stage\": \"%s\",\n"
        "  \"macroblock\": { \"x\": %d, \"y\": %d },\n"
        "  \"channel\": \"%s\",\n  \"storage\": \"%s\",\n"
        "  \"values\": ",
        szMode, szStage, (int)iMBX, (int)iMBY, szChannel,
        valuesToWrite == orderedValues ? "row-major-256" : "internal-256");
    JXRTraceWriteValues(pFile, valuesToWrite);
    fprintf(pFile, "\n}\n");
    fclose(pFile);
}

Void JXRTraceDumpCodecState(const char* szMode, const CWMImageStrCodec* pSC)
{
    char szFile[128], szPath[1200];
    FILE* pFile;

    if (!JXRTraceEnabled() || NULL == pSC) return;
    snprintf(szFile, sizeof(szFile), "%s-global.json", szMode);
    JXRTraceMakePath(szPath, sizeof(szPath), szFile);
    pFile = fopen(szPath, "wb");
    if (NULL == pFile) return;

    fprintf(pFile,
        "{\n  \"mode\": \"%s\",\n  \"width\": %u,\n  \"height\": %u,\n"
        "  \"macroblocks\": { \"width\": %u, \"height\": %u },\n"
        "  \"color_format\": %d,\n  \"bit_depth\": %d,\n"
        "  \"overlap\": %d,\n  \"bitstream_format\": %d,\n"
        "  \"channels\": %u,\n  \"tiles\": { \"columns\": %u, \"rows\": %u },\n"
        "  \"subbands\": %d,\n  \"alpha_mode\": %u\n}\n",
        szMode, (unsigned)pSC->WMII.cWidth, (unsigned)pSC->WMII.cHeight,
        (unsigned)pSC->cmbWidth, (unsigned)pSC->cmbHeight,
        (int)pSC->WMISCP.cfColorFormat, (int)pSC->WMII.bdBitDepth,
        (int)pSC->WMISCP.olOverlap, (int)pSC->WMISCP.bfBitstreamFormat,
        (unsigned)pSC->m_param.cNumChannels,
        (unsigned)(pSC->WMISCP.cNumOfSliceMinus1V + 1),
        (unsigned)(pSC->WMISCP.cNumOfSliceMinus1H + 1),
        (int)pSC->WMISCP.sbSubband, (unsigned)pSC->WMISCP.uAlphaMode);
    fclose(pFile);
}

Void JXRTraceDumpStage(const char* szMode, const char* szStage, const CWMImageStrCodec* pSC, Int iMBX, Int iMBY, JXRTraceBuffer eBuffer)
{
    char szFile[256], szPath[1200];
    const PixelI* pValues;
    FILE* pFile;

    if (!JXRTraceEnabled() || NULL == pSC) return;
    if (JXRTraceSamples == eBuffer)
        pValues = pSC->p1MBbuffer[0];
    else if (JXRTraceOutput == eBuffer)
        pValues = pSC->a0MBbuffer[0];
    else if (0 == strcmp(szMode, "decoder"))
        pValues = pSC->p1MBbuffer[0];
    else
        pValues = pSC->pPlane[0];
    if (NULL == pValues)
        pValues = pSC->p1MBbuffer[0];
    if (NULL == pValues) return;

    snprintf(szFile, sizeof(szFile), "%s-mb-%03d-%03d-%s.json", szMode, (int)iMBX, (int)iMBY, szStage);
    JXRTraceMakePath(szPath, sizeof(szPath), szFile);
    pFile = fopen(szPath, "wb");
    if (NULL == pFile) return;

    fprintf(pFile,
        "{\n  \"mode\": \"%s\",\n  \"stage\": \"%s\",\n"
        "  \"macroblock\": { \"x\": %d, \"y\": %d },\n"
        "  \"channel\": \"Y\",\n  \"storage\": \"internal-256\",\n"
        "  \"macroblock_info\": { \"cbp\": %d, \"diff_cbp\": %d, \"qp_lp\": %u, \"qp_hp\": %u },\n"
        "  \"values\": ",
        szMode, szStage, (int)iMBX, (int)iMBY,
        (int)pSC->MBInfo.iCBP[0], (int)pSC->MBInfo.iDiffCBP[0],
        (unsigned)pSC->MBInfo.iQIndexLP, (unsigned)pSC->MBInfo.iQIndexHP);
    JXRTraceWriteValues(pFile, pValues);
    fprintf(pFile, "\n}\n");
    fclose(pFile);
}
