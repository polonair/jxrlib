#include "JxrAdaptiveScanState.h"

Void JxrAdaptiveScanStateInit(JxrAdaptiveScanState* state, CAdaptiveScan* nativeScan)
{
    state->nativeScan = nativeScan;
}

Void JxrAdaptiveScanStateResetTotals(JxrAdaptiveScanState* state, size_t count)
{
    JxrAdaptiveScanResetTotals(state->nativeScan, count);
}

U32 JxrAdaptiveScanStateGetCoefficientIndex(const JxrAdaptiveScanState* state, size_t position)
{
    return JxrAdaptiveScanGetCoefficientIndex(state->nativeScan, position);
}

Void JxrAdaptiveScanStateObserveNonZero(JxrAdaptiveScanState* state, size_t position)
{
    JxrAdaptiveScanObserveNonZero(state->nativeScan, position);
}
