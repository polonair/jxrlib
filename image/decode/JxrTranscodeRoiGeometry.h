#ifndef JXR_TRANSCODE_ROI_GEOMETRY_H
#define JXR_TRANSCODE_ROI_GEOMETRY_H

#include "windowsmediaphoto.h"

typedef struct JxrTranscodeRoiGeometryRequest {
    size_t imageWidth;
    size_t imageHeight;
    size_t extraLeft;
    size_t extraTop;
    size_t extraRight;
    size_t extraBottom;
    size_t requestedLeft;
    size_t requestedTop;
    size_t requestedWidth;
    size_t requestedHeight;
    OVERLAP overlap;
    Bool ignoreOverlap;
} JxrTranscodeRoiGeometryRequest;

typedef struct JxrTranscodeRoiGeometryResult {
    size_t expandedLeft;
    size_t expandedTop;
    size_t expandedWidth;
    size_t expandedHeight;
    size_t macroblockLeft;
    size_t macroblockTop;
    size_t macroblockRight;
    size_t macroblockBottom;
    size_t extraLeft;
    size_t extraTop;
    size_t extraRight;
    size_t extraBottom;
    size_t imageWidth;
    size_t imageHeight;
} JxrTranscodeRoiGeometryResult;

Bool JxrTranscodeRoiGeometryCalculate(const JxrTranscodeRoiGeometryRequest* request,
    JxrTranscodeRoiGeometryResult* result);

#endif
