#include "JxrEncoderQuantizerInitializer.h"

Void JxrEncoderQuantizerPlanInitialize(JxrEncoderQuantizerPlan* plan, U32 quantizerMode, SUBBAND subband)
{
    plan->initializesDc = (quantizerMode & 1) == 0;
    plan->initializesLp = subband != SB_DC_ONLY && (quantizerMode & 2) == 0;
    plan->initializesHp = subband != SB_DC_ONLY && subband != SB_NO_HIGHPASS && (quantizerMode & 4) == 0;
}

Int JxrEncoderQuantizerInitializerInitialize(CWMImageStrCodec* codec)
{
    U16 iQPIndexY = 0, iQPIndexYLP = 0, iQPIndexYHP = 0;
    U16 iQPIndexU = 0, iQPIndexULP = 0, iQPIndexUHP = 0;
    U16 iQPIndexV = 0, iQPIndexVLP = 0, iQPIndexVHP = 0;
    size_t i;

    if(codec->m_param.bTranscode == FALSE){
        codec->m_param.uQPMode = 0x150;   // 101010 000
                                        // 000    == uniform (not per tile) DC, LP, HP
                                        // 101010 == cChMode == 2 == independent (not same) DC, LP, HP

        /** lossless or Y component lossless condition: all subbands present, uniform quantization with QPIndex 1 **/
        codec->m_param.bScaledArith = !((codec->m_param.uQPMode & 7) == 0 &&
									  1 == codec->WMISCP.uiDefaultQPIndex <= 1 &&
									  codec->WMISCP.sbSubband == SB_ALL &&
									  codec->m_bUVResolutionChange == FALSE) &&
                                     !codec->WMISCP.bUnscaledArith;
        if (BD_32 == codec->WMII.bdBitDepth || BD_32S == codec->WMII.bdBitDepth || BD_32F == codec->WMII.bdBitDepth) {
            codec->m_param.bScaledArith = FALSE;
        }
        codec->m_param.uQPMode |= 0x600;  // don't use DC QP for LP, LP QP for HP

        // default QPs
        iQPIndexY = codec->m_param.bAlphaChannel && codec->m_param.cNumChannels == 1?
            codec->WMISCP.uiDefaultQPIndexAlpha : codec->WMISCP.uiDefaultQPIndex;

		// determine the U,V index
        iQPIndexU = codec->WMISCP.uiDefaultQPIndexU!=0?
			codec->WMISCP.uiDefaultQPIndexU: iQPIndexY;
        iQPIndexV = codec->WMISCP.uiDefaultQPIndexV!=0?
			codec->WMISCP.uiDefaultQPIndexV: iQPIndexY;

		// determine the QPIndexYLP
        iQPIndexYLP = codec->m_param.bAlphaChannel && codec->m_param.cNumChannels == 1 ?
            codec->WMISCP.uiDefaultQPIndexAlpha :
            (codec->WMISCP.uiDefaultQPIndexYLP == 0 ?
			 codec->WMISCP.uiDefaultQPIndex : codec->WMISCP.uiDefaultQPIndexYLP); // default to QPIndex if not set

		// determine the QPIndexYHP
        iQPIndexYHP = codec->m_param.bAlphaChannel && codec->m_param.cNumChannels == 1 ?
            codec->WMISCP.uiDefaultQPIndexAlpha :
            (codec->WMISCP.uiDefaultQPIndexYHP == 0 ?
			 codec->WMISCP.uiDefaultQPIndex : codec->WMISCP.uiDefaultQPIndexYHP); // default to QPIndex if not set

		// determine the U,V LP index
        iQPIndexULP = codec->WMISCP.uiDefaultQPIndexULP!=0?
			codec->WMISCP.uiDefaultQPIndexULP: iQPIndexU;
        iQPIndexVLP = codec->WMISCP.uiDefaultQPIndexVLP!=0?
			codec->WMISCP.uiDefaultQPIndexVLP: iQPIndexV;

		// determine the U,V HP index
        iQPIndexUHP = codec->WMISCP.uiDefaultQPIndexUHP!=0?
			codec->WMISCP.uiDefaultQPIndexUHP: iQPIndexU;
        iQPIndexVHP = codec->WMISCP.uiDefaultQPIndexVHP!=0?
			codec->WMISCP.uiDefaultQPIndexVHP: iQPIndexV;

		// clamp the QPIndex - 0 is lossless mode
        if(iQPIndexY < 2)
            iQPIndexY = 0;
        if (iQPIndexYLP < 2)
            iQPIndexYLP = 0;
        if (iQPIndexYHP < 2)
            iQPIndexYHP = 0;
		if(iQPIndexU < 2)
            iQPIndexU = 0;
        if (iQPIndexULP < 2)
            iQPIndexULP = 0;
        if (iQPIndexUHP < 2)
            iQPIndexUHP = 0;
		if(iQPIndexV < 2)
            iQPIndexV = 0;
		if (iQPIndexVLP < 2)
            iQPIndexVLP = 0;
		if (iQPIndexVHP < 2)
            iQPIndexVHP = 0;
    }

    if((codec->m_param.uQPMode & 1) == 0){ // DC frame uniform quantization
        if(allocateQuantizer(codec->pTile[0].pQuantizerDC, codec->m_param.cNumChannels, 1) != ICERR_OK)
            return ICERR_ERROR;
        setUniformQuantizer(codec, 0);
        for(i = 0; i < codec->m_param.cNumChannels; i ++)
            if(codec->m_param.bTranscode)
                codec->pTile[0].pQuantizerDC[i]->iIndex = codec->m_param.uiQPIndexDC[i];
            else
                codec->pTile[0].pQuantizerDC[i]->iIndex = codec->m_param.uiQPIndexDC[i] = (U8)(((i == 0 ? iQPIndexY : (i == 1) ? iQPIndexU: iQPIndexV)) & 0xff);
        formatQuantizer(codec->pTile[0].pQuantizerDC, (codec->m_param.uQPMode >> 3) & 3, codec->m_param.cNumChannels, 0, TRUE, codec->m_param.bScaledArith);

        for(i = 0; i < codec->m_param.cNumChannels; i ++)
            codec->pTile[0].pQuantizerDC[i]->iOffset = (codec->pTile[0].pQuantizerDC[i]->iQP >> 1);
    }

    if(codec->WMISCP.sbSubband != SB_DC_ONLY){
        if((codec->m_param.uQPMode & 2) == 0){ // LP frame uniform quantization
            if(allocateQuantizer(codec->pTile[0].pQuantizerLP, codec->m_param.cNumChannels, 1) != ICERR_OK)
                return ICERR_ERROR;
            setUniformQuantizer(codec, 1);
            for(i = 0; i < codec->m_param.cNumChannels; i ++)
                if(codec->m_param.bTranscode)
                    codec->pTile[0].pQuantizerLP[i]->iIndex = codec->m_param.uiQPIndexLP[i];
                else
                    codec->pTile[0].pQuantizerLP[i]->iIndex = codec->m_param.uiQPIndexLP[i] = (U8)(((i == 0 ? iQPIndexYLP : (i == 1) ? iQPIndexULP: iQPIndexVLP)) & 0xff);
            formatQuantizer(codec->pTile[0].pQuantizerLP, (codec->m_param.uQPMode >> 5) & 3, codec->m_param.cNumChannels, 0, TRUE, codec->m_param.bScaledArith);
        }

        if(codec->WMISCP.sbSubband != SB_NO_HIGHPASS){
            if((codec->m_param.uQPMode & 4) == 0){ // HP frame uniform quantization
                if(allocateQuantizer(codec->pTile[0].pQuantizerHP, codec->m_param.cNumChannels, 1) != ICERR_OK)
                    return ICERR_ERROR;
                setUniformQuantizer(codec, 2);
                for(i = 0; i < codec->m_param.cNumChannels; i ++)
                    if(codec->m_param.bTranscode)
                        codec->pTile[0].pQuantizerHP[i]->iIndex = codec->m_param.uiQPIndexHP[i];
                    else
                        codec->pTile[0].pQuantizerHP[i]->iIndex = codec->m_param.uiQPIndexHP[i] = (U8)(((i == 0 ? iQPIndexYHP : (i == 1) ? iQPIndexUHP: iQPIndexVHP)) & 0xff);
                formatQuantizer(codec->pTile[0].pQuantizerHP, (codec->m_param.uQPMode >> 7) & 3, codec->m_param.cNumChannels, 0, FALSE, codec->m_param.bScaledArith);
            }
        }
    }

    return ICERR_OK;
}
