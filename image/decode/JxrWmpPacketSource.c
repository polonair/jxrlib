#include "JxrWmpPacketSource.h"

static Bool JxrWmpPacketSourceReadAt(Void* context, size_t offset, U8* destination, size_t count)
{
    JxrWmpPacketSource* state = (JxrWmpPacketSource*)context;
    if (state == NULL || state->stream == NULL)
        return FALSE;
    if (state->stream->SetPos(state->stream, offset) != WMP_errSuccess)
        return FALSE;
    /* readIS deliberately ignores short-read errors and advances its state. */
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
