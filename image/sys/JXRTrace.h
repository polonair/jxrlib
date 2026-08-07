#ifndef JXR_TRACE_H
#define JXR_TRACE_H

#include "strcodec.h"

typedef enum tagJXRTraceBuffer
{
    JXRTraceSamples = 0,
    JXRTraceCoefficients = 1,
    JXRTraceOutput = 2
} JXRTraceBuffer;

Void JXRTraceConfigure(const char* szDirectory);
Bool JXRTraceEnabled(Void);
Void JXRTraceDumpCodecState(const char* szMode, const CWMImageStrCodec* pSC);
Void JXRTraceDumpStage(const char* szMode, const char* szStage, const CWMImageStrCodec* pSC, Int iMBX, Int iMBY, JXRTraceBuffer eBuffer);
size_t JXRTraceBitPosition(const BitIOInfo* pIO, Bool bWrite);
Void JXRTraceDumpBitRange(const char* szMode, const char* szPacket, Int iMBX, Int iMBY,
    size_t cbitStart, size_t cbitEnd);
Void JXRTraceCopyFile(const char* szSourceFile, const char* szTraceFile);

#endif
