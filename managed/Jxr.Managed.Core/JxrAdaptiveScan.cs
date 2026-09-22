using System;

namespace Jxr.Managed.Core
{
    // Managed counterpart of CAdaptiveScan and JxrAdaptiveScan.c.  The two
    // arrays replace an array of native structs and are always owned here.
    public sealed class JxrAdaptiveScan
    {
        public const uint MaximumTotal = 32767U;
        private readonly uint[] totals;
        private readonly uint[] coefficientIndexes;

        public JxrAdaptiveScan(uint[] indexes)
        {
            int index;
            if (indexes == null) throw new ArgumentNullException("indexes");
            totals = new uint[indexes.Length];
            coefficientIndexes = new uint[indexes.Length];
            for (index = 0; index < indexes.Length; index++) coefficientIndexes[index] = indexes[index];
        }

        public int Count { get { return coefficientIndexes.Length; } }

        // InitZigzagScan writes uScan only; uTotal survives this operation.
        public JxrError ResetIndexes(uint[] indexes)
        {
            int index;
            if (indexes == null || indexes.Length != Count) return JxrError.InvalidArgument;
            for (index = 0; index < Count; index++) coefficientIndexes[index] = indexes[index];
            return JxrError.None;
        }

        public JxrError ResetTotals(int count)
        {
            int index;
            uint weight = 32U;
            if (count < 0 || count > Count) return JxrError.InvalidArgument;
            if (count == 0) return JxrError.None;
            totals[0] = MaximumTotal;
            for (index = 1; index < count; index++)
            {
                totals[index] = weight;
                weight = unchecked(weight - 2U);
            }
            return JxrError.None;
        }

        public JxrError GetCoefficientIndex(int position, out uint coefficientIndex)
        {
            coefficientIndex = 0;
            if (position < 0 || position >= Count) return JxrError.InvalidArgument;
            coefficientIndex = coefficientIndexes[position];
            return JxrError.None;
        }

        public JxrError SetCoefficientIndex(int position, uint coefficientIndex)
        {
            if (position < 0 || position >= Count) return JxrError.InvalidArgument;
            coefficientIndexes[position] = coefficientIndex;
            return JxrError.None;
        }

        public JxrError GetTotal(int position, out uint total)
        {
            total = 0;
            if (position < 0 || position >= Count) return JxrError.InvalidArgument;
            total = totals[position];
            return JxrError.None;
        }

        public JxrError ObserveNonZero(int position)
        {
            uint previousIndex;
            uint previousTotal;
            if (position < 0 || position >= Count) return JxrError.InvalidArgument;
            totals[position] = unchecked(totals[position] + 1U);
            if (position == 0 || totals[position] <= totals[position - 1]) return JxrError.None;

            previousIndex = coefficientIndexes[position];
            previousTotal = totals[position];
            coefficientIndexes[position] = coefficientIndexes[position - 1];
            totals[position] = totals[position - 1];
            coefficientIndexes[position - 1] = previousIndex;
            totals[position - 1] = previousTotal;
            return JxrError.None;
        }
    }

    // Managed form of InitZigzagScan's three scan arrays.
    public sealed class JxrAdaptiveScanSet
    {
        private readonly JxrAdaptiveScan lowpass;
        private readonly JxrAdaptiveScan horizontal;
        private readonly JxrAdaptiveScan vertical;

        private JxrAdaptiveScanSet(JxrAdaptiveScan lowpass, JxrAdaptiveScan horizontal,
            JxrAdaptiveScan vertical)
        {
            this.lowpass = lowpass;
            this.horizontal = horizontal;
            this.vertical = vertical;
        }

        public JxrAdaptiveScan Lowpass { get { return lowpass; } }
        public JxrAdaptiveScan Horizontal { get { return horizontal; } }
        public JxrAdaptiveScan Vertical { get { return vertical; } }

        public static JxrAdaptiveScanSet CreateDefault()
        {
            return new JxrAdaptiveScanSet(
                new JxrAdaptiveScan(new uint[] { 0, 1, 4, 5, 2, 8, 6, 9, 3, 12, 10, 7, 13, 11, 14, 15 }),
                new JxrAdaptiveScan(new uint[] { 0, 5, 10, 12, 1, 2, 8, 4, 6, 9, 3, 14, 13, 7, 11, 15 }),
                new JxrAdaptiveScan(new uint[] { 0, 10, 2, 12, 5, 9, 4, 8, 1, 13, 6, 15, 14, 3, 11, 7 }));
        }
    }
}
