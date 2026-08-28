#include "JxrDecoderOptimizationEligibility.h"
#include "decode.h"

static Bool JxrDecoderOptimizationEligibilityHasUniformPlaneSpacing(
    const CWMImageStrCodec* codec)
{
    return codec->p1MBbuffer[1] - codec->p1MBbuffer[0] ==
        codec->p1MBbuffer[2] - codec->p1MBbuffer[1];
}

Bool JxrDecoderOptimizationEligibilityCanUseRgb24Output(
    const CWMImageStrCodec* codec)
{
    const CWMImageInfo* image = &codec->WMII;

    return image->fPaddedUserBuffer &&
        image->bdBitDepth == BD_8 &&
        image->cfColorFormat == CF_RGB &&
        image->cBitsPerUnit == 24 &&
        image->bRGB &&
        image->oOrientation == O_NONE &&
        codec->m_param.cfColorFormat == YUV_444 &&
        JxrDecoderOptimizationEligibilityHasUniformPlaneSpacing(codec) &&
        codec->m_Dparam->bDecodeFullFrame;
}

Bool JxrDecoderOptimizationEligibilityCanUseYuv444CenterTransform(
    const CWMImageStrCodec* codec)
{
    return codec->m_param.cfColorFormat == YUV_444 &&
        JxrDecoderOptimizationEligibilityHasUniformPlaneSpacing(codec) &&
        codec->m_Dparam->bDecodeFullWidth &&
        codec->m_param.cSubVersion == CODEC_SUBVERSION_NEWSCALING_SOFT_TILES &&
        codec->m_Dparam->cThumbnailScale == 1;
}
