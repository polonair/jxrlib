#include "JxrDecoderRoiRowRange.h"

size_t JxrDecoderRoiRowRangeGetOutputHeight(size_t visibleHeight, size_t macroblockRow)
{
    const size_t firstRow = (macroblockRow - 1) * 16;
    const size_t remainingRows = visibleHeight - firstRow;
    return remainingRows < 16 ? remainingRows : 16;
}
