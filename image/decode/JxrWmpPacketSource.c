#include "JxrWmpPacketSource.h"

static JxrPacketReadResult JxrWmpPacketSourceReadAt(Void* context, size_t offset,
    U8* destination, size_t count)
{
    JxrWmpPacketSource* state = (JxrWmpPacketSource*)context;
    JxrPacketReadResult result;
    size_t position = offset;

    result.status = JxrPacketReadFailed;
    result.bytesRead = 0;
    result.nativeError = WMP_errFileIO;
    if (state == NULL || state->stream == NULL)
        return result;
    result.nativeError = state->stream->SetPos(state->stream, offset);
    if (result.nativeError != WMP_errSuccess)
        return result;
    /* readIS advances after short reads and leaves the unwritten packet tail intact. */
    result.nativeError = state->stream->Read(state->stream, destination, count);
    if (state->stream->GetPos(state->stream, &position) != WMP_errSuccess)
        return result;
    result.bytesRead = position >= offset ? position - offset : 0;
    if (result.bytesRead > count)
        result.bytesRead = count;
    result.status = result.bytesRead == count ? JxrPacketReadCompleted : JxrPacketReadShort;
    return result;
}

Void JxrWmpPacketSourceInit(JxrWmpPacketSource* state, struct WMPStream* stream,
    JxrPacketSource* source)
{
    state->stream = stream;
    source->context = state;
    source->readAt = JxrWmpPacketSourceReadAt;
}
