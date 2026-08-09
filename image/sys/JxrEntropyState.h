#ifndef JXR_ENTROPY_STATE_H
#define JXR_ENTROPY_STATE_H

#include "strcodec.h"

/* Deliberately explicit view used as the C-to-C# migration boundary. */
typedef struct JxrEntropyContext {
    CCodingContext* native;
    CAdaptiveModel* dcModel;
    CAdaptiveModel* lpModel;
    CAdaptiveModel* acModel;
    CAdaptiveScan* lowpassScan;
    CAdaptiveScan* horizontalScan;
    CAdaptiveScan* verticalScan;
} JxrEntropyContext;

Void JxrEntropyContextInit(JxrEntropyContext* state, CCodingContext* native);
Void JxrEntropyContextReset(JxrEntropyContext* state);

#endif
