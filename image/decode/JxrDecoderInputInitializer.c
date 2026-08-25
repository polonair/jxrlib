#include "JxrDecoderInputInitializer.h"
#include "decode.h"
#include "JxrIndexTableReader.h"

// Index-table syntax is implemented by JxrIndexTableReader.

static Bool JxrIndexTableReaderStoreLegacyEntry(Void* context, U32 index, U64 value)
{
    ((size_t*)context)[index] = (size_t)value;
    return TRUE;
}

//================================================================
static Int JxrDecoderInputInitializerReadIndexTable(CWMImageStrCodec * pSC)
{
    BitIOInfo* pIO = pSC->pIOHeader;
    JxrIndexTableReader reader;
    JxrIndexTableLegacyContext legacyContext;
    U64 headerSize;
    U32 entryCount = (U32)pSC->cNumBitIO * (pSC->WMISCP.cNumOfSliceMinus1H + 1);

    legacyContext.codec = pSC;
    legacyContext.input = pIO;
    JxrIndexTableReaderInitLegacy(&reader, &legacyContext);
    if (!JxrIndexTableReaderRead(&reader, entryCount,
        entryCount == 0 ? NULL : JxrIndexTableReaderStoreLegacyEntry,
        pSC->pIndexTable, &headerSize))
    {
        return ICERR_ERROR;
    }

    pSC->cHeaderSize = (size_t)headerSize;
    return ICERR_OK;
}

Int JxrDecoderInputInitializerInitialize(CWMImageStrCodec* pSC)
{
    if (allocateBitIOInfo(pSC) != ICERR_OK)
        return ICERR_ERROR;
    if (attachISRead(pSC->pIOHeader, pSC->WMISCP.pWStream, pSC) != WMP_errSuccess)
        return ICERR_ERROR;
    if (JxrDecoderInputInitializerReadIndexTable(pSC) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->WMISCP.bVerbose){
        U32 i, j;

        printf("\n%d horizontal tiles:\n", pSC->WMISCP.cNumOfSliceMinus1H + 1);
        for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1H; i ++){
            printf("    offset of tile %d in MBs: %d\n", i, pSC->WMISCP.uiTileY[i]);
        }

        printf("\n%d vertical tiles:\n", pSC->WMISCP.cNumOfSliceMinus1V + 1);
        for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
            printf("    offset of tile %d in MBs: %d\n", i, pSC->WMISCP.uiTileX[i]);
        }

        if(pSC->WMISCP.bfBitstreamFormat == SPATIAL){
            printf("\nSpatial order bitstream\n");
        }
        else{
            printf("\nFrequency order bitstream\n");
        }

        if(!pSC->m_param.bIndexTable){
            printf("\nstreaming mode, no index table.\n");
        }
        else if(pSC->WMISCP.bfBitstreamFormat == SPATIAL){
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    size_t * p = &pSC->pIndexTable[j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i];
                    if(i + j != pSC->WMISCP.cNumOfSliceMinus1H + pSC->WMISCP.cNumOfSliceMinus1V){
                        printf("bitstream size for tile (%d, %d): %d.\n", j, i, (int) (p[1] - p[0]));
                    }
                    else{
                        printf("bitstream size for tile (%d, %d): unknown.\n", j, i);
                    }
                }
            }
        }
        else{
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    size_t * p = &pSC->pIndexTable[(j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i) * 4];
                    if(i + j != pSC->WMISCP.cNumOfSliceMinus1H + pSC->WMISCP.cNumOfSliceMinus1V){
                        printf("bitstream size of (DC, LP, AC, FL) for tile (%d, %d): %d %d %d %d.\n", j, i,
                            (int) (p[1] - p[0]), (int) (p[2] - p[1]), (int) (p[3] - p[2]), (int) (p[4] - p[3]));
                    }
                    else{
                        printf("bitstream size of (DC, LP, AC, FL) for tile (%d, %d): %d %d %d unknown.\n", j, i,
                            (int) (p[1] - p[0]), (int) (p[2] - p[1]), (int) (p[3] - p[2]));
                    }
                }
            }
        }
    }

    return 0;
}