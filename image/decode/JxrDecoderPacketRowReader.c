#include "JxrDecoderPacketRowReader.h"

Bool JxrDecoderPacketRowReaderRead(CWMImageStrCodec* codec,
    const JxrDecoderPacketRowReaderOperations* operations)
{
    JxrDecoderBitstreamSet bitstreams;
    JxrDecoderPacketAttachmentConfig attachment;
    JxrDecoderPacketHeaderReaderConfig header;
    JxrDecoderCodingContextResetterConfig resetter;
    U32 tileRowCount;
    U32 externalStreamCount;

    if (codec == NULL || operations == NULL) return FALSE;
    if (!JxrDecoderBitstreamSetInit(&bitstreams, codec->m_param.bIndexTable,
        codec->WMISCP.bfBitstreamFormat, codec->WMISCP.cNumOfSliceMinus1V,
        codec->WMISCP.cNumOfSliceMinus1H, codec->WMISCP.sbSubband) ||
        bitstreams.bitstreamCount != codec->cNumBitIO)
        return FALSE;

    tileRowCount = codec->WMISCP.cNumOfSliceMinus1H + 1;
    externalStreamCount = tileRowCount *
        (codec->cNumBitIO == 0 ? 1 : (U32)codec->cNumBitIO);
    JxrDecoderPacketAttachmentConfigInit(&attachment, &bitstreams, (U32)codec->cTileRow,
        tileRowCount, codec->ppWStream != NULL, codec->pIOHeader, codec->m_ppBitIO,
        codec->pIndexTable, (U64)codec->cHeaderSize, codec->WMISCP.pWStream,
        codec->ppWStream, externalStreamCount);
    if (!JxrDecoderPacketAttachmentAttachRow(&attachment, &operations->attachment))
        return FALSE;

    JxrDecoderPacketHeaderReaderConfigInit(&header, &bitstreams, (U32)codec->cTileRow,
        codec->m_param.bTrimFlexbitsFlag, codec->pIOHeader, codec->m_ppBitIO);
    if (!JxrDecoderPacketHeaderReaderReadRow(&header, &operations->header)) return FALSE;

    resetter.contexts = codec->m_pCodingContext;
    resetter.contextCount = codec->WMISCP.cNumOfSliceMinus1V + 1;
    resetter.resetAll = TRUE;
    return JxrDecoderCodingContextResetterResetContexts(&resetter,
        &operations->resetter);
}
