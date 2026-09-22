using System;

namespace Jxr.Managed.Core
{
    public sealed class JxrHuffmanStateSet
    {
        private readonly JxrAdaptiveHuffman[] states;

        public JxrHuffmanStateSet(JxrAdaptiveHuffman[] source)
        {
            if (source == null) throw new ArgumentNullException("source");
            states = source;
        }

        public JxrAdaptiveHuffman Get(int index)
        {
            if (index < 0 || index >= states.Length) return null;
            return states[index];
        }

        public JxrError Observe(int index, int symbol)
        {
            JxrAdaptiveHuffman state = Get(index);
            if (state == null) return JxrError.InvalidArgument;
            return state.ObserveSymbol(symbol);
        }

        public JxrError Adapt(int index)
        {
            JxrAdaptiveHuffman state = Get(index);
            if (state == null) return JxrError.InvalidArgument;
            return state.Adapt();
        }
    }
}
