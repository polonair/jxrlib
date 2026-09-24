using System;

namespace Jxr.Managed.Core
{
    // The shared integer transform primitives from JxrTransformMath.c.
    // The four offsets replace four PixelI* arguments; all intermediate
    // arithmetic is explicitly Int32 with C-style wraparound and signed shift.
    public static class JxrTransformMath
    {
        private static readonly int[] FirstStageOffsets =
            { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 };
        private static readonly int[] SecondStageOffsets =
            { 0, 192, 48, 240, 64, 128, 112, 176,
              16, 208, 32, 224, 80, 144, 96, 160 };

        public static JxrError ApplyDct2x2Down(int[] values,
            int first, int second, int third, int fourth)
        {
            return ApplyDct2x2(values, first, second, third, fourth, false);
        }

        public static JxrError ApplyDct2x2Up(int[] values,
            int first, int second, int third, int fourth)
        {
            return ApplyDct2x2(values, first, second, third, fourth, true);
        }

        public static JxrError ApplyFourButterfly(int[] values, int[] offsets)
        {
            if (values == null || offsets == null || offsets.Length != 16)
                return JxrError.InvalidArgument;
            for (int index = 0; index < 16; index++)
            {
                if (offsets[index] < 0 || offsets[index] >= values.Length)
                    return JxrError.InvalidArgument;
                for (int previous = 0; previous < index; previous++)
                    if (offsets[index] == offsets[previous])
                        return JxrError.InvalidArgument;
            }
            for (int group = 0; group < 4; group++)
            {
                int start = group * 4;
                ApplyDct2x2(values, offsets[start], offsets[start + 1],
                    offsets[start + 2], offsets[start + 3], false);
            }
            return JxrError.None;
        }

        public static JxrError ApplyFirstStageFourButterfly(int[] values)
        {
            return ApplyFourButterfly(values, FirstStageOffsets);
        }

        public static JxrError ApplySecondStageFourButterfly(int[] values)
        {
            return ApplyFourButterfly(values, SecondStageOffsets);
        }

        private static JxrError ApplyDct2x2(int[] values,
            int first, int second, int third, int fourth, bool roundUp)
        {
            if (values == null || first < 0 || second < 0 || third < 0 ||
                fourth < 0 || first >= values.Length || second >= values.Length ||
                third >= values.Length || fourth >= values.Length ||
                first == second || first == third || first == fourth ||
                second == third || second == fourth || third == fourth)
                return JxrError.InvalidArgument;

            int firstValue = values[first];
            int secondValue = values[second];
            int thirdInput = values[third];
            int fourthValue = values[fourth];
            unchecked
            {
                firstValue += fourthValue;
                secondValue -= thirdInput;
                int average = firstValue - secondValue;
                if (roundUp) average += 1;
                average >>= 1;
                int thirdValue = average - fourthValue;
                fourthValue = average - thirdInput;
                firstValue -= fourthValue;
                secondValue += thirdValue;

                values[first] = firstValue;
                values[second] = secondValue;
                values[third] = thirdValue;
                values[fourth] = fourthValue;
            }
            return JxrError.None;
        }
    }
}
