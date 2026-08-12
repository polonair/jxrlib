#include "JxrStreamPositionScope.h"
#include <string.h>

Bool JxrStreamPositionScopeCapture(JxrStreamPositionScope* scope,
    struct WMPStream* stream)
{
    if (scope == NULL || stream == NULL || stream->GetPos == NULL ||
        stream->SetPos == NULL) return FALSE;
    memset(scope, 0, sizeof(*scope));
    if (stream->GetPos(stream, &scope->position) != WMP_errSuccess) return FALSE;
    scope->stream = stream;
    scope->captured = TRUE;
    return TRUE;
}

Bool JxrStreamPositionScopeRestore(JxrStreamPositionScope* scope)
{
    Bool restored;
    if (scope == NULL || !scope->captured || scope->stream == NULL) return FALSE;
    restored = scope->stream->SetPos(scope->stream, scope->position) == WMP_errSuccess;
    scope->stream = NULL;
    scope->captured = FALSE;
    return restored;
}
