#ifndef JXR_TRANSCODE_QUANTIZER_WRITER_H
#define JXR_TRANSCODE_QUANTIZER_WRITER_H

#include "JxrTranscodeTileQuantizerState.h"

typedef Bool (*JxrTranscodeBitSinkWrite)(Void* context, U32 value, U32 count);

typedef struct JxrTranscodeBitSink {
    Void* context;
    JxrTranscodeBitSinkWrite write;
} JxrTranscodeBitSink;

Void JxrTranscodeBitSinkInit(JxrTranscodeBitSink* sink, Void* context,
    JxrTranscodeBitSinkWrite write);
Void JxrTranscodeBitSinkInitLegacy(JxrTranscodeBitSink* sink, BitIOInfo* nativeOutput);
Bool JxrTranscodeBitSinkWriteBits(JxrTranscodeBitSink* sink, U32 value, U32 count);

Bool JxrTranscodeQuantizerWriterWriteQuantizer(JxrTranscodeBitSink* sink,
    const U8 indices[MAX_CHANNELS], U8 channelMode, size_t channelCount);
Bool JxrTranscodeQuantizerWriterWriteQuantizers(JxrTranscodeBitSink* sink,
    const U8 indices[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS],
    const U8 channelModes[JXR_TRANSCODE_MAX_QUANTIZERS], U32 quantizerCount,
    size_t channelCount, Bool copyPrevious);
Bool JxrTranscodeQuantizerWriterWriteAlphaQuantizers(JxrTranscodeBitSink* sink,
    const U8 indices[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS], U32 quantizerCount,
    size_t alphaChannelIndex, Bool copyPrevious);

#endif
