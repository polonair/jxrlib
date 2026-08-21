#include "JxrEncoderColorTransform.h"

Void JxrEncoderColorTransformApplyRgb(PixelI* red, PixelI* green, PixelI* blue)
{
    *blue -= *red;
    *red += ((*blue + 1) >> 1) - *green;
    *green += (*red >> 1);
}

Void JxrEncoderColorTransformApplyCmyk(PixelI* cyan, PixelI* magenta,
    PixelI* yellow, PixelI* black)
{
    *yellow -= *cyan;
    *cyan += ((*yellow + 1) >> 1) - *magenta;
    *magenta += (*cyan >> 1) - *black;
    *black += ((*magenta + 1) >> 1);
}
