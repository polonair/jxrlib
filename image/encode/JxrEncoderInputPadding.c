#include "JxrEncoderInputPadding.h"

Void JxrEncoderInputPaddingPlanInitialize(JxrEncoderInputPaddingPlan* plan,
    size_t imageWidth, size_t macroblockWidth, Bool inputIsYuvData,
    COLORFORMAT sourceFormat, COLORFORMAT targetFormat, size_t channelCount)
{
    plan->requiresPadding = imageWidth != macroblockWidth * 16;
    plan->sourceFormat = inputIsYuvData ? targetFormat : sourceFormat;
    plan->fullResolutionChannelCount = channelCount;
    if (plan->sourceFormat == YUV_420 || plan->sourceFormat == YUV_422 ||
        plan->sourceFormat == Y_ONLY)
    {
        plan->fullResolutionChannelCount = 1;
    }
    plan->padsYuv422Chroma = plan->sourceFormat == YUV_422;
    plan->padsYuv420Chroma = plan->sourceFormat == YUV_420;
}

Void JxrEncoderInputPaddingApply(CWMImageStrCodec* codec)
{
    JxrEncoderInputPaddingPlan plan;
    PixelI* channels[16];
    size_t channel;
    size_t column;
    size_t row;
    size_t lastFullResolutionColumn;

    JxrEncoderInputPaddingPlanInitialize(&plan, codec->WMII.cWidth,
        codec->cmbWidth, codec->WMISCP.bYUVData, codec->WMII.cfColorFormat,
        codec->m_param.cfColorFormat, codec->WMISCP.cChannel);
    if (!plan.requiresPadding)
        return;

    assert(plan.fullResolutionChannelCount <= 16);
    assert(codec->WMISCP.cChannel <= 16);
    for (channel = 0; channel < codec->WMISCP.cChannel; ++channel)
        channels[channel & 15] = codec->p1MBbuffer[channel & 15];

    if (codec->m_bUVResolutionChange) {
        channels[1] = codec->pResU;
        channels[2] = codec->pResV;
    }

    lastFullResolutionColumn = codec->WMII.cWidth - 1;
    for (row = 0; row < 16; ++row) {
        const size_t lastPosition = ((lastFullResolutionColumn >> 4) << 8) +
            idxCC[row][lastFullResolutionColumn & 0xf];
        for (column = lastFullResolutionColumn + 1; column < codec->cmbWidth * 16;
            ++column)
        {
            const size_t position = ((column >> 4) << 8) + idxCC[row][column & 0xf];
            for (channel = 0; channel < plan.fullResolutionChannelCount; ++channel)
                channels[channel & 15][position] = channels[channel & 15][lastPosition];
        }
    }

    if (plan.padsYuv422Chroma) {
        const size_t lastChromaColumn = lastFullResolutionColumn >> 1;
        for (row = 0; row < 16; ++row) {
            const size_t lastPosition = ((lastChromaColumn >> 3) << 7) +
                idxCC[row][lastChromaColumn & 7];
            for (column = lastChromaColumn + 1; column < codec->cmbWidth * 8; ++column) {
                const size_t position = ((column >> 3) << 7) + idxCC[row][column & 7];
                for (channel = 1; channel < 3; ++channel)
                    channels[channel][position] = channels[channel][lastPosition];
            }
        }
    }
    else if (plan.padsYuv420Chroma) {
        const size_t lastChromaColumn = lastFullResolutionColumn >> 1;
        for (row = 0; row < 8; ++row) {
            const size_t lastPosition = ((lastChromaColumn >> 3) << 6) +
                idxCC_420[row][lastChromaColumn & 7];
            for (column = lastChromaColumn + 1; column < codec->cmbWidth * 8; ++column) {
                const size_t position = ((column >> 3) << 6) + idxCC_420[row][column & 7];
                for (channel = 1; channel < 3; ++channel)
                    channels[channel][position] = channels[channel][lastPosition];
            }
        }
    }
}
