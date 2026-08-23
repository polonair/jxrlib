#include "JxrEncoderChromaDownsampler.h"

Void JxrEncoderChromaDownsamplingPlanInitialize(JxrEncoderChromaDownsamplingPlan* plan,
    COLORFORMAT sourceFormat, COLORFORMAT targetFormat)
{
    plan->performsHorizontalDownsampling = sourceFormat != YUV_422;
    plan->writesHorizontalResultToMacroblockBuffer = targetFormat == YUV_422;
    plan->performsVerticalDownsampling = targetFormat == YUV_420;
}

PixelI JxrEncoderChromaDownsamplerFilterOdd(PixelI first, PixelI second,
    PixelI center, PixelI fourth, PixelI fifth)
{
    return ((((second + center + fourth) << 2) + (center << 1) + first + fifth + 8) >> 4);
}

Void JxrEncoderChromaDownsamplerApply(CWMImageStrCodec* codec)
{
    const COLORFORMAT targetFormat = codec->m_param.cfColorFormat;
    const COLORFORMAT sourceFormat = codec->WMII.cfColorFormat;
    JxrEncoderChromaDownsamplingPlan plan;
    size_t channel;

    JxrEncoderChromaDownsamplingPlanInitialize(&plan, sourceFormat, targetFormat);
    for (channel = 1; channel < 3; ++channel) {
        PixelI* source;
        PixelI* destination;
        PixelI first;
        PixelI second;
        PixelI center;
        PixelI fourth;
        PixelI fifth;
        size_t row;
        size_t column;

        if (plan.performsHorizontalDownsampling) {
            const size_t horizontalShift = targetFormat == YUV_422 ? 1 : 0;
            source = channel == 1 ? codec->pResU : codec->pResV;
            destination = plan.writesHorizontalResultToMacroblockBuffer ?
                codec->p1MBbuffer[channel] : source;

            for (row = 0; row < 16; ++row) {
                first = fifth = source[idxCC[row][2]];
                second = fourth = source[idxCC[row][1]];
                center = source[idxCC[row][0]];
                for (column = 0; column + 2 < codec->cmbWidth * 16; column += 2) {
                    destination[((column >> 4) << (8 - horizontalShift)) +
                        idxCC[row][(column & 15) >> horizontalShift]] =
                        JxrEncoderChromaDownsamplerFilterOdd(first, second, center, fourth, fifth);
                    first = center;
                    second = fourth;
                    center = fifth;
                    fourth = source[(((column + 3) >> 4) << 8) +
                        idxCC[row][(column + 3) & 0xf]];
                    fifth = source[(((column + 4) >> 4) << 8) +
                        idxCC[row][(column + 4) & 0xf]];
                }

                fifth = center;
                destination[((column >> 4) << (8 - horizontalShift)) +
                    idxCC[row][(column & 15) >> horizontalShift]] =
                    JxrEncoderChromaDownsamplerFilterOdd(first, second, center, fourth, fifth);
            }
        }

        if (plan.performsVerticalDownsampling) {
            const size_t verticalShift = sourceFormat == YUV_422 ? 0 : 1;
            PixelI* bufferedRows[4];
            size_t macroblockOffset;
            size_t pixelOffset;

            destination = codec->p1MBbuffer[channel];
            source = channel == 1 ? codec->pResU : codec->pResV;
            bufferedRows[0] = source + (codec->cmbWidth <<
                (sourceFormat == YUV_422 ? 7 : 8));
            bufferedRows[1] = bufferedRows[0] + codec->cmbWidth * 8;
            bufferedRows[2] = bufferedRows[1] + codec->cmbWidth * 8;
            bufferedRows[3] = bufferedRows[2] + codec->cmbWidth * 8;

            for (column = 0; column < codec->cmbWidth * 8; ++column) {
                macroblockOffset = (column >> 3) << (7 + verticalShift);
                pixelOffset = (column & 7) << verticalShift;
                if (codec->cRow == 0) {
                    first = fifth = source[macroblockOffset + idxCC[2][pixelOffset]];
                    second = fourth = source[macroblockOffset + idxCC[1][pixelOffset]];
                    center = source[macroblockOffset + idxCC[0][pixelOffset]];
                }
                else {
                    first = bufferedRows[0][column];
                    second = bufferedRows[1][column];
                    center = bufferedRows[2][column];
                    fourth = bufferedRows[3][column];
                    fifth = source[macroblockOffset + idxCC[0][pixelOffset]];
                    codec->p0MBbuffer[channel][((column >> 3) << 6) +
                        idxCC_420[7][column & 7]] =
                        JxrEncoderChromaDownsamplerFilterOdd(first, second, center, fourth, fifth);

                    first = bufferedRows[2][column];
                    second = bufferedRows[3][column];
                    center = source[macroblockOffset + idxCC[0][pixelOffset]];
                    fourth = source[macroblockOffset + idxCC[1][pixelOffset]];
                    fifth = source[macroblockOffset + idxCC[2][pixelOffset]];
                }

                for (row = 0; row < 12; row += 2) {
                    destination[((column >> 3) << 6) + idxCC_420[row >> 1][column & 7]] =
                        JxrEncoderChromaDownsamplerFilterOdd(first, second, center, fourth, fifth);
                    first = center;
                    second = fourth;
                    center = fifth;
                    fourth = source[macroblockOffset + idxCC[row + 3][pixelOffset]];
                    fifth = source[macroblockOffset + idxCC[row + 4][pixelOffset]];
                }

                destination[((column >> 3) << 6) + idxCC_420[6][column & 7]] =
                    JxrEncoderChromaDownsamplerFilterOdd(first, second, center, fourth, fifth);
                first = center;
                second = fourth;
                center = fifth;
                fourth = source[macroblockOffset + idxCC[row + 3][pixelOffset]];

                if (codec->cRow + 1 == codec->cmbHeight) {
                    fifth = center;
                    destination[((column >> 3) << 6) + idxCC_420[7][column & 7]] =
                        JxrEncoderChromaDownsamplerFilterOdd(first, second, center, fourth, fifth);
                }
                else {
                    for (row = 0; row < 4; ++row)
                        bufferedRows[row][column] = source[macroblockOffset +
                            idxCC[row + 12][pixelOffset]];
                }
            }
        }
    }
}
