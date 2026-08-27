#include "JxrDecoderUvInterpolator.h"

Void JxrDecoderUvInterpolatorInterpolate(CWMImageStrCodec* codec)
{
    const COLORFORMAT outputColorFormat = codec->WMII.cfColorFormat;
    const size_t macroblockWidth = codec->cmbWidth * 16;
    PixelI* sourceU = codec->a0MBbuffer[1];
    PixelI* sourceV = codec->a0MBbuffer[2];
    PixelI* destinationU = codec->pResU;
    PixelI* destinationV = codec->pResV;
    size_t row;
    size_t column;
    size_t sourceIndex = 0;
    size_t destinationIndex = 0;

    if (codec->m_param.cfColorFormat == YUV_422) {
        for (row = 0; row < 16; row++) {
            for (column = 0; column < macroblockWidth; column += 2) {
                sourceIndex = ((column >> 4) << 7) + idxCC[row][(column >> 1) & 7];
                destinationIndex = ((column >> 4) << 8) + idxCC[row][column & 15];
                destinationU[destinationIndex] = sourceU[sourceIndex];
                destinationV[destinationIndex] = sourceV[sourceIndex];

                if (column > 0) {
                    size_t leftColumn = column - 2;
                    size_t leftIndex = ((leftColumn >> 4) << 8) +
                        idxCC[row][leftColumn & 15];
                    size_t interpolatedColumn = column - 1;
                    size_t interpolatedIndex = ((interpolatedColumn >> 4) << 8) +
                        idxCC[row][interpolatedColumn & 15];

                    destinationU[interpolatedIndex] =
                        (destinationU[leftIndex] + destinationU[destinationIndex] + 1) >> 1;
                    destinationV[interpolatedIndex] =
                        (destinationV[leftIndex] + destinationV[destinationIndex] + 1) >> 1;
                }
            }

            sourceIndex = (((column - 1) >> 4) << 8) + idxCC[row][(column - 1) & 15];
            destinationU[sourceIndex] = destinationU[destinationIndex];
            destinationV[sourceIndex] = destinationV[destinationIndex];
        }
    }
    else {
        const size_t horizontalShift = outputColorFormat == YUV_422 ? 3 : 4;

        for (column = 0; column < macroblockWidth; column += 2) {
            const size_t macroblockOffset = (column >> 4) << (4 + horizontalShift);
            const size_t pixelOffset = (column >> (4 - horizontalShift)) &
                ((1 << horizontalShift) - 1);

            for (row = 0; row < 16; row += 2) {
                sourceIndex = ((column >> 4) << 6) + idxCC_420[row >> 1][(column >> 1) & 7];
                destinationIndex = macroblockOffset + idxCC[row][pixelOffset];
                destinationU[destinationIndex] = sourceU[sourceIndex];
                destinationV[destinationIndex] = sourceV[sourceIndex];

                if (row > 0) {
                    size_t topIndex = macroblockOffset + idxCC[row - 2][pixelOffset];
                    size_t interpolatedIndex = macroblockOffset + idxCC[row - 1][pixelOffset];
                    destinationU[interpolatedIndex] =
                        (destinationU[topIndex] + destinationU[destinationIndex] + 1) >> 1;
                    destinationV[interpolatedIndex] =
                        (destinationV[topIndex] + destinationV[destinationIndex] + 1) >> 1;
                }
            }

            sourceIndex = macroblockOffset + idxCC[15][pixelOffset];
            if (codec->cRow == codec->cmbHeight) {
                destinationU[sourceIndex] = destinationU[destinationIndex];
                destinationV[sourceIndex] = destinationV[destinationIndex];
            }
            else {
                size_t nextRowIndex = ((column >> 4) << 6) +
                    idxCC_420[0][(column >> 1) & 7];
                destinationU[sourceIndex] =
                    (codec->a1MBbuffer[1][nextRowIndex] + destinationU[destinationIndex] + 1) >> 1;
                destinationV[sourceIndex] =
                    (codec->a1MBbuffer[2][nextRowIndex] + destinationV[destinationIndex] + 1) >> 1;
            }
        }

        if (outputColorFormat != YUV_422) {
            for (row = 0; row < 16; row++) {
                for (column = 1; column < macroblockWidth - 2; column += 2) {
                    size_t leftIndex = (((column - 1) >> 4) << 8) +
                        idxCC[row][(column - 1) & 15];
                    destinationIndex = ((column >> 4) << 8) + idxCC[row][column & 15];
                    sourceIndex = (((column + 1) >> 4) << 8) +
                        idxCC[row][(column + 1) & 15];
                    destinationU[destinationIndex] =
                        (destinationU[sourceIndex] + destinationU[leftIndex] + 1) >> 1;
                    destinationV[destinationIndex] =
                        (destinationV[sourceIndex] + destinationV[leftIndex] + 1) >> 1;
                }

                destinationIndex = (((macroblockWidth - 1) >> 4) << 8) +
                    idxCC[row][(macroblockWidth - 1) & 15];
                destinationU[destinationIndex] = destinationU[sourceIndex];
                destinationV[destinationIndex] = destinationV[sourceIndex];
            }
        }
    }
}
