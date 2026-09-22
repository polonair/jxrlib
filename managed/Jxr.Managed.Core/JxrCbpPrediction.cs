namespace Jxr.Managed.Core
{
    // The three native predCBPC*Dec variants use the same adaptive-count
    // transition.  Width and neighbour shifts are explicit here.
    internal static class JxrCbpPrediction
    {
        private static int Saturate(int count)
        {
            return unchecked((uint)(count + 16)) >= 32U ?
                count < 0 ? -16 : 15 : count;
        }

        private static int CountOnes(int value)
        {
            int count = 0;
            value &= 65535;
            while (value != 0) { count += value & 1; value >>= 1; }
            return count;
        }

        internal static JxrError Decode(JxrCodecState codec, int channel,
            int differential, int width, out int cbp)
        {
            cbp = differential;
            int top, left, zeroCount, oneCount, predictionState;
            int modelIndex = channel == 0 ? 0 : 1;
            JxrError error = codec.GetNeighborCbp(channel, out top, out left);
            if (error != JxrError.None) return error;
            error = codec.HighpassCbp.PredictionModel.Get(modelIndex,
                out zeroCount, out oneCount, out predictionState);
            if (error != JxrError.None) return error;
            if (predictionState == 0)
            {
                if (codec.AtLeftBoundary)
                    cbp ^= codec.AtTopBoundary ? 1 :
                        (top >> (width == 16 ? 10 : width == 8 ? 6 : 2)) & 1;
                else cbp ^= (left >> (width == 16 ? 5 : 1)) & 1;
                cbp ^= (cbp & 1) << 1;
                if (width == 16)
                {
                    cbp ^= 0x10 & (cbp << 3);
                    cbp ^= 0x20 & (cbp << 1);
                    cbp ^= (cbp & 0x33) << 2;
                    cbp ^= (cbp & 0xcc) << 6;
                    cbp ^= (cbp & 0x3300) << 2;
                }
                else if (width == 4) cbp ^= (cbp & 3) << 2;
                else
                {
                    cbp ^= (cbp & 3) << 2;
                    cbp ^= (cbp & 12) << 2;
                    cbp ^= (cbp & 48) << 2;
                }
            }
            else if (predictionState == 2) cbp ^= (1 << width) - 1;
            int ones = CountOnes(cbp) * (16 / width);
            zeroCount = Saturate(zeroCount + ones - 3);
            oneCount = Saturate(oneCount + 16 - ones - 3);
            predictionState = zeroCount < 0 ?
                (zeroCount < oneCount ? 1 : 2) : oneCount < 0 ? 2 : 0;
            error = codec.HighpassCbp.PredictionModel.Set(modelIndex,
                zeroCount, oneCount, predictionState);
            if (error != JxrError.None) return error;
            error = codec.MacroblockCbp.SetCbp(channel, cbp);
            if (error != JxrError.None) return error;
            return codec.SetCurrentCbp(channel, cbp);
        }
    }
}
