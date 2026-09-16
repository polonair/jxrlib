#ifndef JXR_TRANSCODE_SESSION_RUNNER_H
#define JXR_TRANSCODE_SESSION_RUNNER_H

#include "windowsmediaphoto.h"

Int JxrTranscodeSessionRunnerRun(struct WMPStream* inputStream,
    struct WMPStream* outputStream, CWMTranscodingParam* parameters);

#endif
