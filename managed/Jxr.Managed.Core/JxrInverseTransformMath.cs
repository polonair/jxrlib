namespace Jxr.Managed.Core
{
    // Managed counterpart of JxrInverseTransformMath.c. Distinct ref int
    // arguments replace PixelI*; arrays remain caller-owned. Intermediate
    // operations intentionally use signed shifts and unchecked Int32 arithmetic.
    public static class JxrInverseTransformMath
    {
        public static void RotateHalf(ref int first, ref int second)
        {
            unchecked
            {
                first -= (second + 1) >> 1;
                second += (first + 1) >> 1;
            }
        }

        public static void RotateThreeEighths(ref int first, ref int second)
        {
            unchecked
            {
                first -= (second * 3 + 4) >> 3;
                second += (first * 3 + 4) >> 3;
            }
        }

        public static JxrError AddCornerPredictionAt(int[] samples,
            int targetOffset, int prediction)
        {
            if (samples == null || targetOffset < 0 || targetOffset >= samples.Length)
                return JxrError.InvalidArgument;
            samples[targetOffset] = unchecked(samples[targetOffset] + prediction);
            return JxrError.None;
        }

        public static JxrError SubtractCornerPredictionAt(int[] samples,
            int targetOffset, int prediction)
        {
            if (samples == null || targetOffset < 0 || targetOffset >= samples.Length)
                return JxrError.InvalidArgument;
            samples[targetOffset] = unchecked(samples[targetOffset] - prediction);
            return JxrError.None;
        }

        public static JxrError NormalizeBlock(int[] samples, bool chroma,
            int sampleCount, int sampleStride)
        {
            if (samples == null || sampleCount < 0 || sampleCount > samples.Length ||
                sampleStride <= 0) return JxrError.InvalidArgument;
            if (chroma)
                for (int index = 0; index < sampleCount; index += sampleStride)
                    samples[index] = unchecked(samples[index] + samples[index]);
            return JxrError.None;
        }

        public static bool ShouldCompensateDc(int directCurrent,
            int highPassQuantizer, bool highPassAbsent)
        {
            if (highPassAbsent) return true;
            if (highPassQuantizer <= 20) return false;
            int absolute = directCurrent < 0 ? unchecked(-directCurrent) : directCurrent;
            return absolute < highPassQuantizer;
        }

        public static void ApplyDcCompensation(ref int topLeft, ref int topRight,
            ref int bottomLeft, ref int bottomRight, int directCurrent)
        {
            unchecked
            {
                int half = directCurrent >> 1;
                topLeft -= half;
                bottomRight -= half;
                topRight += half;
                bottomLeft += half;
            }
        }

        public static int ClipDcWithAlternate(int directCurrent, int alternateCurrent)
        {
            if (directCurrent > 0 && alternateCurrent > 0)
                return directCurrent < alternateCurrent ? directCurrent : alternateCurrent;
            if (directCurrent < 0 && alternateCurrent < 0)
                return directCurrent > alternateCurrent ? directCurrent : alternateCurrent;
            return 0;
        }

        public static int ApplyConditionalDcCompensation(ref int topLeft,
            ref int topRight, ref int bottomLeft, ref int bottomRight,
            int directCurrent, int highPassQuantizer, bool highPassAbsent)
        {
            if (!ShouldCompensateDc(directCurrent, highPassQuantizer, highPassAbsent))
                return directCurrent;
            unchecked
            {
                int alternate = (topLeft - bottomLeft - topRight + bottomRight) >> 1;
                directCurrent = ClipDcWithAlternate(directCurrent, alternate);
                ApplyDcCompensation(ref topLeft, ref topRight,
                    ref bottomLeft, ref bottomRight, directCurrent);
                return directCurrent;
            }
        }

        public static void ApplyPost4(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                a += d;
                b += c;
                d -= (a + 1) >> 1;
                c -= (b + 1) >> 1;
                RotateHalf(ref c, ref d);
                d += (a + 1) >> 1;
                c += (b + 1) >> 1;
                a -= d - ((d * 3 + 16) >> 5);
                b -= c - ((c * 3 + 16) >> 5);
                d += (a * 3 + 8) >> 4;
                c += (b * 3 + 8) >> 4;
                a += (d * 3 + 16) >> 5;
                b += (c * 3 + 16) >> 5;
                first = a; second = b; third = c; fourth = d;
            }
        }

        private static void ApplyAlternatePost4Edge(ref int first, ref int fourth)
        {
            unchecked
            {
                int a = first, d = fourth;
                a += d;
                d = (a >> 1) - d;
                a += (d * 3) >> 3;
                d += (a * 3) >> 4;
                d += a >> 7;
                d -= a >> 10;
                a += (d * 3 + 4) >> 3;
                d -= a >> 1;
                a += d;
                first = a; fourth = -d;
            }
        }

        public static void ApplyAlternatePost4(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                a += d;
                b += c;
                d -= (a + 1) >> 1;
                c -= (b + 1) >> 1;
                ApplyAlternatePost4Edge(ref a, ref d);
                ApplyAlternatePost4Edge(ref b, ref c);
                RotateHalf(ref c, ref d);
                d += (a + 1) >> 1;
                c += (b + 1) >> 1;
                a -= d;
                b -= c;
                first = a; second = b; third = c; fourth = d;
            }
        }

        public static void ApplyHadamardScale4(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                b -= c;
                a += (d * 3 + 4) >> 3;
                d -= b >> 1;
                c = ((a - b) >> 1) - c;
                third = d;
                fourth = c;
                first = a - c;
                second = b + d;
            }
        }

        public static void ApplyHadamardScale2(ref int first, ref int second)
        {
            unchecked
            {
                int a = first, b = second;
                a += b;
                b = (a >> 1) - b;
                a += (b * 3) >> 3;
                b += (a * 3) >> 4;
                first = a; second = b;
            }
        }

        public static void ApplyAlternateHadamardScale2(ref int first, ref int second)
        {
            unchecked
            {
                ApplyHadamardScale2(ref first, ref second);
                second += first >> 7;
                second -= first >> 10;
            }
        }

        private static void ApplyOddOddCore(ref int first, ref int second,
            ref int third, ref int fourth, int firstRounding,
            int secondRounding, bool negateMiddle)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                d += a;
                c -= b;
                int firstHalf = d >> 1;
                int secondHalf = c >> 1;
                a -= firstHalf;
                b += secondHalf;
                a -= (b * 3 + firstRounding) >> 3;
                b += (a * 3 + secondRounding) >> 2;
                a -= (b * 3 + 4) >> 3;
                b -= secondHalf;
                a += firstHalf;
                c += b;
                d -= a;
                first = a;
                second = negateMiddle ? -b : b;
                third = negateMiddle ? -c : c;
                fourth = d;
            }
        }

        public static void ApplyOddOdd(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            ApplyOddOddCore(ref first, ref second, ref third, ref fourth, 3, 3, true);
        }

        public static void ApplyOddOddPost(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            ApplyOddOddCore(ref first, ref second, ref third, ref fourth, 6, 2, false);
        }

        public static void ApplyOdd(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                b += d;
                a -= c;
                d -= b >> 1;
                c += (a + 1) >> 1;
                RotateThreeEighths(ref a, ref b);
                RotateThreeEighths(ref c, ref d);
                c -= (b + 1) >> 1;
                d = ((a + 1) >> 1) - d;
                b += c;
                a -= d;
                first = a; second = b; third = c; fourth = d;
            }
        }

        public static void ApplyScaledDct2x2Down(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            int[] values = { first, second, third, fourth };
            JxrTransformMath.ApplyDct2x2Down(values, 0, 1, 2, 3);
            unchecked
            {
                first = values[0] * 2;
                second = values[1] * 2;
                third = values[2] * 2;
                fourth = values[3] * 2;
            }
        }

        public static void ApplyPost2(ref int first, ref int second)
        {
            unchecked
            {
                int a = first, b = second;
                b += (a + 4) >> 3;
                a += (b + 2) >> 2;
                b += (a + 4) >> 3;
                first = a; second = b;
            }
        }

        public static void ApplyAlternatePost2(ref int first, ref int second)
        {
            unchecked
            {
                int a = first, b = second;
                b += (a + 2) >> 2;
                a += (b + 1) >> 1;
                a += b >> 5;
                a += b >> 9;
                a += b >> 13;
                b += (a + 2) >> 2;
                first = a; second = b;
            }
        }

        public static void ApplyPost2x2(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                a += d;
                b += c;
                d -= (a + 1) >> 1;
                c -= (b + 1) >> 1;
                b += (a + 2) >> 2;
                a += (b + 1) >> 1;
                b += (a + 2) >> 2;
                d += (a + 1) >> 1;
                c += (b + 1) >> 1;
                a -= d;
                b -= c;
                first = a; second = b; third = c; fourth = d;
            }
        }

        public static void ApplyAlternatePost2x2(ref int first, ref int second,
            ref int third, ref int fourth)
        {
            unchecked
            {
                int a = first, b = second, c = third, d = fourth;
                a += d;
                b += c;
                d -= (a + 1) >> 1;
                c -= (b + 1) >> 1;
                b += (a + 2) >> 2;
                a += (b + 1) >> 1;
                a += b >> 5;
                a += b >> 9;
                a += b >> 13;
                b += (a + 2) >> 2;
                d += (a + 1) >> 1;
                c += (b + 1) >> 1;
                a -= d;
                b -= c;
                first = a; second = b; third = c; fourth = d;
            }
        }
    }
}
