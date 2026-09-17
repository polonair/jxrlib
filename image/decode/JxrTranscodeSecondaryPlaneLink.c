#include "JxrTranscodeSecondaryPlaneLink.h"

Void JxrTranscodeSecondaryPlaneLinkInitialize(
    JxrTranscodeSecondaryPlaneLink* link)
{
    if (link != NULL) memset(link, 0, sizeof(*link));
}

Bool JxrTranscodeSecondaryPlaneLinkAttach(JxrTranscodeSecondaryPlaneLink* link,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec)
{
    if (link == NULL || primaryCodec == NULL || secondaryCodec == NULL ||
        primaryCodec->m_pNextSC != NULL || secondaryCodec->m_pNextSC != NULL)
        return FALSE;
    primaryCodec->m_pNextSC = secondaryCodec;
    secondaryCodec->m_pNextSC = primaryCodec;
    link->primaryCodec = primaryCodec;
    link->secondaryCodec = secondaryCodec;
    return TRUE;
}

Void JxrTranscodeSecondaryPlaneLinkDetachAndRelease(
    JxrTranscodeSecondaryPlaneLink* link)
{
    if (link == NULL) return;
    if (link->primaryCodec != NULL &&
        link->primaryCodec->m_pNextSC == link->secondaryCodec)
        link->primaryCodec->m_pNextSC = NULL;
    if (link->secondaryCodec != NULL &&
        link->secondaryCodec->m_pNextSC == link->primaryCodec)
        link->secondaryCodec->m_pNextSC = NULL;
    free(link->secondaryCodec);
    JxrTranscodeSecondaryPlaneLinkInitialize(link);
}

Void JxrTranscodeSecondaryPlaneLinkReleaseAttached(CWMImageStrCodec* primaryCodec)
{
    JxrTranscodeSecondaryPlaneLink link;

    if (primaryCodec == NULL || primaryCodec->m_pNextSC == NULL) return;
    JxrTranscodeSecondaryPlaneLinkInitialize(&link);
    link.primaryCodec = primaryCodec;
    link.secondaryCodec = primaryCodec->m_pNextSC;
    JxrTranscodeSecondaryPlaneLinkDetachAndRelease(&link);
}
