#include "JxrWmpPacketSource.h"

static Bool JxrWmpPacketSourceReadAt(Void* context, size_t offset, U8* destination, size_t count)
{
    JxrWmpPacketSource* state = (JxrWmpPacketSource*)context;
    if (state == NULL || state->stream == NULL)
        return FALSE;
    /* readIS deliberately ignores short-read errors and advances its state. */
    state->stream->SetPos(state->stream, offset);
    state->lastReadResult = state->stream->Read(state->stream, destination, count);
    return TRUE;
}

Void JxrWmpPacketSourceInit(JxrWmpPacketSource* state, struct WMPStream* stream,
    JxrPacketSource* source)
{
    state->stream = stream;
    state->lastReadResult = WMP_errSuccess;
    source->context = state;
    source->readAt = JxrWmpPacketSourceReadAt;
}
