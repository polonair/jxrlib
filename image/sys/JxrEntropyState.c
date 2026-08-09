#include "JxrEntropyState.h"

Void JxrEntropyContextInit(JxrEntropyContext* state, CCodingContext* native)
{
    state->native = native;
    state->dcModel = native ? &native->m_aModelDC : NULL;
    state->lpModel = native ? &native->m_aModelLP : NULL;
    state->acModel = native ? &native->m_aModelAC : NULL;
    state->lowpassScan = native ? native->m_aScanLowpass : NULL;
    state->horizontalScan = native ? native->m_aScanHoriz : NULL;
    state->verticalScan = native ? native->m_aScanVert : NULL;
}

Void JxrEntropyContextReset(JxrEntropyContext* state)
{
    if (!state || !state->native) return;
    ResetCodingContext(state->native);
    InitZigzagScan(state->native);
    JxrEntropyContextInit(state, state->native);
}
