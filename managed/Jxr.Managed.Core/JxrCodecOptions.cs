namespace Jxr.Managed.Core
{
    public enum JxrBitstreamLayout
    {
        Spatial = 0,
        Frequency = 1
    }

    public sealed class JxrEncoderOptions
    {
        private int qualityIndex = 1;
        private int overlap = 0;
        private JxrBitstreamLayout layout = JxrBitstreamLayout.Spatial;

        public int QualityIndex { get { return qualityIndex; } set { qualityIndex = value; } }
        public int Overlap { get { return overlap; } set { overlap = value; } }
        public JxrBitstreamLayout Layout { get { return layout; } set { layout = value; } }
    }

    public sealed class JxrDecoderOptions
    {
        private JxrPixelFormat outputFormat = JxrPixelFormat.Gray8;

        public JxrPixelFormat OutputFormat
        {
            get { return outputFormat; }
            set { outputFormat = value; }
        }
    }
}
