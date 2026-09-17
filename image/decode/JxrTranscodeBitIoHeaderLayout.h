#ifndef JXR_TRANSCODE_BIT_IO_HEADER_LAYOUT_H
#define JXR_TRANSCODE_BIT_IO_HEADER_LAYOUT_H

#include "strcodec.h"

/* Managed-port shape: byte buffer length and the offset of its BitIO state. */
typedef struct JxrTranscodeBitIoHeaderLayout {
    size_t allocationBytes;
    size_t headerBitIoOffset;
} JxrTranscodeBitIoHeaderLayout;

Bool JxrTranscodeBitIoHeaderLayoutInitialize(
    JxrTranscodeBitIoHeaderLayout* layout, UINTPTR_T allocationAddress,
    size_t bitIoStateBytes);

Bool JxrTranscodeBitIoHeaderLayoutBindCompatibilityPointer(
    CWMImageStrCodec* codec, U8* allocation,
    const JxrTranscodeBitIoHeaderLayout* layout);

#endif
