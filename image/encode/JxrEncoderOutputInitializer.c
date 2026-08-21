#include "JxrEncoderOutputInitializer.h"
#include "JxrEncoderPacketStreamInitializer.h"
#include "JxrEncoderMainHeaderWriter.h"

Void JxrEncoderOutputPlanInitialize(JxrEncoderOutputPlan* plan, Bool isSecondary)
{
    plan->initializesPrimaryOutput = !isSecondary;
    plan->sharesPrimaryOutput = isSecondary;
    plan->writesMainHeader = !isSecondary;
}

Int JxrEncoderOutputInitializerInitialize(CWMImageStrCodec* codec)
{
    JxrEncoderOutputPlan plan;

    JxrEncoderOutputPlanInitialize(&plan, codec->m_bSecondary);
    if (plan.sharesPrimaryOutput) {
        codec->pIOHeader = codec->m_pNextSC->pIOHeader;
        codec->m_ppBitIO = codec->m_pNextSC->m_ppBitIO;
        codec->cNumBitIO = codec->m_pNextSC->cNumBitIO;
        codec->cSB = codec->m_pNextSC->cSB;
        codec->ppWStream = codec->m_pNextSC->ppWStream;
        codec->pIndexTable = codec->m_pNextSC->pIndexTable;
        setBitIOPointers(codec);
        return ICERR_OK;
    }

    /* Preserve the legacy StrEncInit error-handling behavior. */
    JxrEncoderPacketStreamInitializerInitialize(codec);
    setBitIOPointers(codec);
    JxrEncoderMainHeaderWriterWrite(codec);
    return ICERR_OK;
}
