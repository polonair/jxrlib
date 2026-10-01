namespace Jxr.Managed.Core
{
    internal sealed class JxrTileGeometry
    {
        private readonly int[] x;
        private readonly int[] y;

        private JxrTileGeometry(int[] x, int[] y)
        {
            this.x = x;
            this.y = y;
        }

        internal int Columns { get { return x.Length - 1; } }
        internal int Rows { get { return y.Length - 1; } }
        internal int GetX(int index) { return x[index]; }
        internal int GetY(int index) { return y[index]; }
        internal bool IsSingleTile { get { return Columns == 1 && Rows == 1; } }

        internal static JxrError Create(int width, int height,
            JxrTileLayout layout, out JxrTileGeometry geometry)
        {
            geometry = null;
            if (width < 1 || height < 1 || width > int.MaxValue - 15 ||
                height > int.MaxValue - 15) return JxrError.InvalidArgument;
            int columns = (int)(((long)width + 15) / 16);
            int rows = (int)(((long)height + 15) / 16);
            if (layout == null)
                layout = new JxrTileLayout(new int[] { columns },
                    new int[] { rows });
            int[] x, y;
            if (!layout.GetBoundaries(width, height, out x, out y))
                return JxrError.InvalidArgument;
            geometry = new JxrTileGeometry(x, y);
            return JxrError.None;
        }

        internal int[] CopyColumnWidths()
        {
            int[] result = new int[Columns];
            for (int index = 0; index < result.Length; index++)
                result[index] = x[index + 1] - x[index];
            return result;
        }

        internal int[] CopyRowHeights()
        {
            int[] result = new int[Rows];
            for (int index = 0; index < result.Length; index++)
                result[index] = y[index + 1] - y[index];
            return result;
        }
    }
}
