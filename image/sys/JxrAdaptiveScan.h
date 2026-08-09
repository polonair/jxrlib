#ifndef JXR_ADAPTIVE_SCAN_H
#define JXR_ADAPTIVE_SCAN_H

#include "strcodec.h"

Void JxrAdaptiveScanResetTotals(CAdaptiveScan* scan, size_t count);
U32 JxrAdaptiveScanGetCoefficientIndex(const CAdaptiveScan* scan, size_t position);
Void JxrAdaptiveScanObserveNonZero(CAdaptiveScan* scan, size_t position);

#endif
