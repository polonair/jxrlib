#include "JxrTranscodeRoiGeometry.h"

static size_t JxrTranscodeRoiGeometryOverlapExtent(OVERLAP overlap)
{
    return overlap == OL_TWO ? 10 : 2;
}

Bool JxrTranscodeRoiGeometryCalculate(const JxrTranscodeRoiGeometryRequest* request,
    JxrTranscodeRoiGeometryResult* result)
{
    size_t left;
    size_t top;
    size_t width;
    size_t height;
    size_t extent;
    size_t totalWidth;
    size_t totalHeight;

    if (request == NULL || result == NULL ||
        request->requestedLeft + request->requestedWidth > request->imageWidth ||
        request->requestedTop + request->requestedHeight > request->imageHeight) return FALSE;
    left = request->requestedLeft + request->extraLeft;
    top = request->requestedTop + request->extraTop;
    width = request->requestedWidth;
    height = request->requestedHeight;
    totalWidth = request->imageWidth + request->extraLeft + request->extraRight;
    totalHeight = request->imageHeight + request->extraTop + request->extraBottom;
    if (request->overlap != OL_NONE && !request->ignoreOverlap) {
        extent = JxrTranscodeRoiGeometryOverlapExtent(request->overlap);
        if (left > extent) { left -= extent; width += extent; }
        else { width += left; left = 0; }
        if (top > extent) { top -= extent; height += extent; }
        else { height += top; top = 0; }
        width += extent;
        height += extent;
        if (left + width > totalWidth) width = totalWidth - left;
        if (top + height > totalHeight) height = totalHeight - top;
    }
    result->expandedLeft = left;
    result->expandedTop = top;
    result->expandedWidth = width;
    result->expandedHeight = height;
    result->macroblockLeft = left >> 4;
    result->macroblockTop = top >> 4;
    result->macroblockRight = (left + width + 15) >> 4;
    result->macroblockBottom = (top + height + 15) >> 4;
    result->extraLeft = request->extraLeft + request->requestedLeft -
        (result->macroblockLeft << 4);
    result->extraTop = request->extraTop + request->requestedTop -
        (result->macroblockTop << 4);
    result->extraRight = ((result->macroblockRight - result->macroblockLeft) << 4) -
        request->requestedWidth - result->extraLeft;
    result->extraBottom = ((result->macroblockBottom - result->macroblockTop) << 4) -
        request->requestedHeight - result->extraTop;
    result->imageWidth = ((result->macroblockRight - result->macroblockLeft) << 4) -
        result->extraLeft - result->extraRight;
    result->imageHeight = ((result->macroblockBottom - result->macroblockTop) << 4) -
        result->extraTop - result->extraBottom;
    return TRUE;
}
