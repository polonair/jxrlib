#ifndef JXR_STREAM_POSITION_SCOPE_H
#define JXR_STREAM_POSITION_SCOPE_H

#include "JxrHeaderStreamReader.h"

typedef struct JxrStreamPositionScope {
    struct WMPStream* stream;
    size_t position;
    Bool captured;
} JxrStreamPositionScope;

Bool JxrStreamPositionScopeCapture(JxrStreamPositionScope* scope,
    struct WMPStream* stream);
Bool JxrStreamPositionScopeRestore(JxrStreamPositionScope* scope);

#endif
