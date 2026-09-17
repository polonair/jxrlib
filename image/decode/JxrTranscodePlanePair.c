#include "JxrTranscodePlanePair.h"

Bool JxrTranscodePlanePairResolveLegacy(JxrTranscodePlanePair* pair,
    CWMImageStrCodec* primaryCodec, Bool hasAlpha)
{
    if (pair == NULL || primaryCodec == NULL) return FALSE;
    pair->primaryCodec = primaryCodec;
    pair->alphaCodec = hasAlpha ? primaryCodec->m_pNextSC : NULL;
    pair->hasAlpha = hasAlpha;
    return !hasAlpha || pair->alphaCodec != NULL;
}
