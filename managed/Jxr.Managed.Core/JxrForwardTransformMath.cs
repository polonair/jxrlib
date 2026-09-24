namespace Jxr.Managed.Core
{
    // Exact integer lifting steps from JxrForwardTransformMath.c. Callers pass
    // ref int values from managed coefficient arrays, never native pointers.
    // Signed right shift and unchecked Int32 arithmetic are intentional.
    public static class JxrForwardTransformMath
    {
        public static void RotateHalf(ref int first, ref int second)
        {
            unchecked
            {
                second -= (first + 1) >> 1;
                first += (second + 1) >> 1;
            }
        }

        public static void RotateThreeEighths(ref int first, ref int second)
        {
            unchecked
            {
                second -= (first * 3 + 4) >> 3;
                first += (second * 3 + 4) >> 3;
            }
        }

        public static JxrError NormalizeBlock(int[] samples, bool chroma,
            int sampleCount, int sampleStride)
        {
            if (samples == null || sampleCount < 0 || sampleCount > samples.Length ||
                sampleStride <= 0) return JxrError.InvalidArgument;
            if (chroma)
                for (int index = 0; index < sampleCount; index += sampleStride)
                    samples[index] >>= 1;
            return JxrError.None;
        }

        // Unlike the shared DCT2x2, the encoder first scales each input down.
        public static void ApplyDct2x2Down(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first >> 1;
                int secondValue = second >> 1;
                int thirdInput = third >> 1;
                int fourthValue = fourth >> 1;
                firstValue += fourthValue;
                secondValue -= thirdInput;
                int middle = (firstValue - secondValue) >> 1;
                int thirdValue = middle - fourthValue;
                fourthValue = middle - thirdInput;
                firstValue -= fourthValue;
                secondValue += thirdValue;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }

        public static void ApplyPre2(ref int first, ref int second)
        {
            unchecked
            {
                int firstValue = first, secondValue = second;
                secondValue -= (firstValue + 2) >> 2;
                firstValue -= (secondValue + 1) >> 1;
                firstValue -= secondValue >> 5;
                firstValue -= secondValue >> 9;
                firstValue -= secondValue >> 13;
                secondValue -= (firstValue + 2) >> 2;
                first = firstValue; second = secondValue;
            }
        }

        public static void ApplyPre2x2(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, secondValue = second;
                int thirdValue = third, fourthValue = fourth;
                firstValue += fourthValue;
                secondValue += thirdValue;
                fourthValue -= (firstValue + 1) >> 1;
                thirdValue -= (secondValue + 1) >> 1;
                secondValue -= (firstValue + 2) >> 2;
                firstValue -= (secondValue + 1) >> 1;
                firstValue -= secondValue >> 5;
                firstValue -= secondValue >> 9;
                firstValue -= secondValue >> 13;
                secondValue -= (firstValue + 2) >> 2;
                fourthValue += (firstValue + 1) >> 1;
                thirdValue += (secondValue + 1) >> 1;
                firstValue -= fourthValue;
                secondValue -= thirdValue;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }

        private static void ApplyPre4Edge(ref int first, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, fourthValue = -fourth;
                firstValue -= fourthValue;
                fourthValue += firstValue >> 1;
                firstValue -= (fourthValue * 3 + 4) >> 3;
                fourthValue -= firstValue >> 7;
                fourthValue += firstValue >> 10;
                fourthValue -= (firstValue * 3) >> 4;
                firstValue -= (fourthValue * 3) >> 3;
                fourthValue = (firstValue >> 1) - fourthValue;
                firstValue -= fourthValue;
                first = firstValue; fourth = fourthValue;
            }
        }

        public static void ApplyPre4(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, secondValue = second;
                int thirdValue = third, fourthValue = fourth;
                firstValue += fourthValue;
                secondValue += thirdValue;
                fourthValue -= (firstValue + 1) >> 1;
                thirdValue -= (secondValue + 1) >> 1;
                RotateHalf(ref thirdValue, ref fourthValue);
                ApplyPre4Edge(ref firstValue, ref fourthValue);
                ApplyPre4Edge(ref secondValue, ref thirdValue);
                fourthValue += (firstValue + 1) >> 1;
                thirdValue += (secondValue + 1) >> 1;
                firstValue -= fourthValue;
                secondValue -= thirdValue;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }

        public static void ApplyHst4(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, secondValue = second;
                int thirdValue = fourth, fourthValue = third;
                firstValue += thirdValue;
                secondValue -= fourthValue;
                thirdValue = ((firstValue - secondValue) >> 1) - thirdValue;
                fourthValue += secondValue >> 1;
                secondValue += thirdValue;
                firstValue -= (fourthValue * 3 + 4) >> 3;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }

        public static void ApplyHst1(ref int first, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, fourthValue = fourth;
                fourthValue -= firstValue >> 7;
                fourthValue += firstValue >> 10;
                fourthValue -= (firstValue * 3) >> 4;
                firstValue -= (fourthValue * 3) >> 3;
                fourthValue = (firstValue >> 1) - fourthValue;
                firstValue -= fourthValue;
                first = firstValue; fourth = fourthValue;
            }
        }

        public static void ApplyOddOdd(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, secondValue = -second;
                int thirdValue = -third, fourthValue = fourth;
                fourthValue += firstValue;
                thirdValue -= secondValue;
                int firstTemporary = fourthValue >> 1;
                int secondTemporary = thirdValue >> 1;
                firstValue -= firstTemporary;
                secondValue += secondTemporary;
                firstValue += (secondValue * 3 + 4) >> 3;
                secondValue -= (firstValue * 3 + 3) >> 2;
                firstValue += (secondValue * 3 + 3) >> 3;
                secondValue -= secondTemporary;
                firstValue += firstTemporary;
                thirdValue += secondValue;
                fourthValue -= firstValue;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }

        public static void ApplyOddOddPre(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, secondValue = second;
                int thirdValue = third, fourthValue = fourth;
                fourthValue += firstValue;
                thirdValue -= secondValue;
                int firstTemporary = fourthValue >> 1;
                int secondTemporary = thirdValue >> 1;
                firstValue -= firstTemporary;
                secondValue += secondTemporary;
                firstValue += (secondValue * 3 + 4) >> 3;
                secondValue -= (firstValue * 3 + 2) >> 2;
                firstValue += (secondValue * 3 + 6) >> 3;
                secondValue -= secondTemporary;
                firstValue += firstTemporary;
                thirdValue += secondValue;
                fourthValue -= firstValue;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }

        public static void ApplyOdd(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int firstValue = first, secondValue = second;
                int thirdValue = third, fourthValue = fourth;
                secondValue -= thirdValue;
                firstValue += fourthValue;
                thirdValue += (secondValue + 1) >> 1;
                fourthValue = ((firstValue + 1) >> 1) - fourthValue;
                RotateThreeEighths(ref firstValue, ref secondValue);
                RotateThreeEighths(ref thirdValue, ref fourthValue);
                fourthValue += secondValue >> 1;
                thirdValue -= (firstValue + 1) >> 1;
                secondValue -= fourthValue;
                firstValue += thirdValue;
                first = firstValue; second = secondValue;
                third = thirdValue; fourth = fourthValue;
            }
        }
    }
}
