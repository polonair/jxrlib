#include "JxrInverseColorTransform.h"

Void JxrInverseColorTransformApplyRgb(PixelI* red, PixelI* green, PixelI* blue)
{
    *green -= (*red >> 1);
    *red -= ((*blue + 1) >> 1) - *green;
    *blue += *red;
}

Void JxrInverseColorTransformApplyCmyk(PixelI* cyan, PixelI* magenta,
    PixelI* yellow, PixelI* black)
{
    *black -= (*magenta + 1) >> 1;
    *magenta -= (*cyan >> 1) - *black;
    *cyan -= ((*yellow + 1) >> 1) - *magenta;
    *yellow += *cyan;
}
