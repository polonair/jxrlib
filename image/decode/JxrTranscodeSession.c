#include "JxrTranscodeSession.h"

#include "decode.h"
#include "JxrDecoderResourceInitializer.h"
#include "../encode/encode.h"

EXTERN_C Void StrIOEncTerm(CWMImageStrCodec* codec);

Void JxrTranscodeSessionRelease(JxrTranscodeSession* session)
{
    if (session == NULL) return;
    free(session->macroblockBuffer);
    if (session->hasOrientation) {
        free(session->primaryFrameBuffer);
        free(session->primaryFrameMacroblocks);
        if (session->hasAlphaFrame) {
            free(session->alphaFrameBuffer);
            free(session->alphaFrameMacroblocks);
        }
    }
    if (session->decoderCodec != NULL) {
        freePredInfo(session->decoderCodec);
        freeTileInfo(session->decoderCodec);
        JxrDecoderResourceInitializerReleaseIo(session->decoderCodec);
        FreeCodingContextDec(session->decoderCodec);
        if (session->decoderHasAlpha) free(session->decoderCodec->m_pNextSC);
        free(session->decoderCodec);
    }
    free(session->decoderIoHeader);
    if (session->encoderCodec != NULL) {
        if (!session->usedFastTileExtraction) {
            freePredInfo(session->encoderCodec);
            freeTileInfo(session->encoderCodec);
            StrIOEncTerm(session->encoderCodec);
            free(session->tileQuantizers);
            FreeCodingContextEnc(session->encoderCodec);
        }
        free(session->encoderCodec);
    }
    free(session->encoderIoHeader);
    memset(session, 0, sizeof(*session));
}
