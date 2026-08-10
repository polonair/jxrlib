#ifndef JXR_ADAPTIVE_SCAN_STATE_H
#define JXR_ADAPTIVE_SCAN_STATE_H

#include "JxrAdaptiveScan.h"

/* Explicit adaptive scan state used by LP and HP coefficient decoders. */
typedef struct JxrAdaptiveScanState {
    CAdaptiveScan* nativeScan;
} JxrAdaptiveScanState;

Void JxrAdaptiveScanStateInit(JxrAdaptiveScanState* state, CAdaptiveScan* nativeScan);
Void JxrAdaptiveScanStateResetTotals(JxrAdaptiveScanState* state, size_t count);
U32 JxrAdaptiveScanStateGetCoefficientIndex(const JxrAdaptiveScanState* state, size_t position);
Void JxrAdaptiveScanStateObserveNonZero(JxrAdaptiveScanState* state, size_t position);

#endif
