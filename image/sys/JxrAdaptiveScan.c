#include "JxrAdaptiveScan.h"

Void JxrAdaptiveScanResetTotals(CAdaptiveScan* scan, size_t count)
{
    size_t i; U32 weight = 32;
    if (!scan || count == 0) return;
    scan[0].uTotal = MAXTOTAL;
    for (i = 1; i < count; ++i) { scan[i].uTotal = weight; weight -= 2; }
}

U32 JxrAdaptiveScanGetCoefficientIndex(const CAdaptiveScan* scan, size_t position)
{
    return scan[position].uScan;
}

Void JxrAdaptiveScanObserveNonZero(CAdaptiveScan* scan, size_t position)
{
    CAdaptiveScan previous;
    scan[position].uTotal++;
    if (position == 0 || scan[position].uTotal <= scan[position - 1].uTotal) return;
    previous = scan[position]; scan[position] = scan[position - 1]; scan[position - 1] = previous;
}
