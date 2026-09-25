using System;

namespace Jxr.Managed.Core
{
    public sealed class JxrTranscodeOrientation
    {
        private readonly bool flipVertical, flipHorizontal, transpose;
        private JxrTranscodeOrientation(bool vertical, bool horizontal, bool transposed)
        {
            flipVertical = vertical; flipHorizontal = horizontal; transpose = transposed;
        }
        public bool FlipVertical { get { return flipVertical; } }
        public bool FlipHorizontal { get { return flipHorizontal; } }
        public bool Transpose { get { return transpose; } }

        public static JxrTranscodeOrientation FromCode(int code)
        {
            if (code < 0 || code > 7) throw new ArgumentOutOfRangeException("code");
            return new JxrTranscodeOrientation(code == 1 || code == 3 ||
                code == 4 || code == 5, code == 2 || code == 3 ||
                code == 5 || code == 7, code >= 4);
        }

        public int MapRow(int row, int height)
        { return flipVertical ? height - row - 1 : row; }
        public int MapColumn(int column, int width)
        { return flipHorizontal ? width - column - 1 : column; }
        public int TileRow(int row, int column)
        { return transpose ? column : row; }
        public int TileColumn(int row, int column)
        { return transpose ? row : column; }
        public long FrameOffset(int row, int column, int width, int height)
        { return transpose ? row + (long)height * column : (long)row * width + column; }
    }

    // Coefficient-domain orientation, matching JxrTranscodeCoefficientTransform.c.
    // Native semantics mutate the source signs before writing the destination.
    public static class JxrTranscoder
    {
        private static readonly int[] DctIndex = {
            0, 5, 1, 6, 10, 12, 8, 14, 2, 4, 3, 7, 9, 13, 11, 15
        };

        private static bool HasRange(int[] data, int offset, int count)
        { return data != null && offset >= 0 && offset <= data.Length &&
            count <= data.Length - offset; }

        // format: 1=YUV_420, 2=YUV_422, other valid formats=full resolution.
        public static JxrError TransformDc(int format, int[] source, int sourceOffset,
            int[] destination, int destinationOffset, JxrTranscodeOrientation orientation)
        {
            if (format < 0 || format > 6 || format == 5 || orientation == null)
                return JxrError.InvalidArgument;
            int count = format == 1 ? 4 : format == 2 ? 8 : 16;
            if (!HasRange(source, sourceOffset, count) ||
                !HasRange(destination, destinationOffset, count))
                return JxrError.InvalidArgument;
            if (format == 2 && orientation.Transpose) return JxrError.UnsupportedFeature;
            unchecked
            {
                if (format == 1)
                {
                    if (orientation.FlipVertical)
                    {
                        source[sourceOffset + 1] = -source[sourceOffset + 1];
                        source[sourceOffset + 3] = -source[sourceOffset + 3];
                    }
                    if (orientation.FlipHorizontal)
                    {
                        source[sourceOffset + 2] = -source[sourceOffset + 2];
                        source[sourceOffset + 3] = -source[sourceOffset + 3];
                    }
                    destination[destinationOffset] = source[sourceOffset];
                    destination[destinationOffset + 3] = source[sourceOffset + 3];
                    destination[destinationOffset + 1] =
                        source[sourceOffset + (orientation.Transpose ? 2 : 1)];
                    destination[destinationOffset + 2] =
                        source[sourceOffset + (orientation.Transpose ? 1 : 2)];
                }
                else if (format == 2)
                {
                    if (orientation.FlipVertical)
                        foreach (int index in new int[] { 1, 3, 4, 5, 7 })
                            source[sourceOffset + index] = -source[sourceOffset + index];
                    if (orientation.FlipHorizontal)
                        foreach (int index in new int[] { 2, 3, 6, 7 })
                            source[sourceOffset + index] = -source[sourceOffset + index];
                    if (orientation.FlipVertical)
                    {
                        int[] mapping = { 0, 5, 6, 7, 4, 1, 2, 3 };
                        for (int index = 0; index < 8; index++)
                            destination[destinationOffset + index] =
                                source[sourceOffset + mapping[index]];
                    }
                    else Array.Copy(source, sourceOffset, destination, destinationOffset, 8);
                }
                else
                {
                    if (orientation.FlipVertical)
                        for (int index = 0; index < 16; index += 4)
                        {
                            source[sourceOffset + index + 1] =
                                -source[sourceOffset + index + 1];
                            source[sourceOffset + index + 3] =
                                -source[sourceOffset + index + 3];
                        }
                    if (orientation.FlipHorizontal)
                        for (int index = 0; index < 4; index++)
                        {
                            source[sourceOffset + index + 4] =
                                -source[sourceOffset + index + 4];
                            source[sourceOffset + index + 12] =
                                -source[sourceOffset + index + 12];
                        }
                    if (!orientation.Transpose)
                        Array.Copy(source, sourceOffset, destination, destinationOffset, 16);
                    else
                        for (int index = 0; index < 16; index++)
                            destination[destinationOffset + index] =
                                source[sourceOffset + (index >> 2) + ((index & 3) << 2)];
                }
            }
            return JxrError.None;
        }

        public static JxrError TransformAc(int format, int[] source, int sourceOffset,
            int[] destination, int destinationOffset, JxrTranscodeOrientation orientation)
        {
            if (format < 0 || format > 6 || format == 5 || orientation == null)
                return JxrError.InvalidArgument;
            int blocks = format == 1 ? 4 : format == 2 ? 8 : 16;
            int count = blocks * 16;
            if (!HasRange(source, sourceOffset, count) ||
                !HasRange(destination, destinationOffset, count))
                return JxrError.InvalidArgument;
            if (format == 2 && orientation.Transpose) return JxrError.UnsupportedFeature;
            unchecked
            {
                for (int block = 0; block < blocks; block++)
                {
                    int offset = sourceOffset + block * 16;
                    if (orientation.FlipVertical)
                        for (int coefficient = 0; coefficient < 16; coefficient += 4)
                        {
                            int first = offset + DctIndex[coefficient + 1];
                            int second = offset + DctIndex[coefficient + 3];
                            source[first] = -source[first];
                            source[second] = -source[second];
                        }
                    if (orientation.FlipHorizontal)
                        for (int coefficient = 0; coefficient < 4; coefficient++)
                        {
                            int first = offset + DctIndex[coefficient + 4];
                            int second = offset + DctIndex[coefficient + 12];
                            source[first] = -source[first];
                            source[second] = -source[second];
                        }
                }
                int rows = format == 2 || format == 1 ? 2 : 4;
                int columns = format == 1 ? 2 : 4;
                for (int blockRow = 0; blockRow < rows; blockRow++)
                    for (int blockColumn = 0; blockColumn < columns; blockColumn++)
                    {
                        int targetRow = orientation.FlipVertical ?
                            columns - 1 - blockColumn : blockColumn;
                        int targetColumn = orientation.FlipHorizontal ?
                            rows - 1 - blockRow : blockRow;
                        int sourceBlock = sourceOffset +
                            (blockRow * columns + blockColumn) * 16;
                        if (!orientation.Transpose)
                        {
                            int target = destinationOffset +
                                (targetColumn * columns + targetRow) * 16;
                            Array.Copy(source, sourceBlock, destination, target, 16);
                        }
                        else
                        {
                            int target = destinationOffset +
                                (targetRow * rows + targetColumn) * 16;
                            for (int coefficient = 1; coefficient < 16; coefficient++)
                                destination[target + DctIndex[coefficient]] =
                                    source[sourceBlock + DctIndex[(coefficient >> 2) +
                                        ((coefficient & 3) << 2)]];
                        }
                    }
            }
            return JxrError.None;
        }

        public static JxrError CalculateRoi(int imageWidth, int imageHeight,
            int extraLeft, int extraTop, int extraRight, int extraBottom,
            int requestedLeft, int requestedTop, int requestedWidth,
            int requestedHeight, int overlap, bool ignoreOverlap,
            out JxrTranscodeRoi result)
        {
            result = null;
            if (imageWidth < 0 || imageHeight < 0 || extraLeft < 0 ||
                extraTop < 0 || extraRight < 0 || extraBottom < 0 ||
                requestedLeft < 0 || requestedTop < 0 ||
                requestedWidth < 0 || requestedHeight < 0 ||
                overlap < 0 || overlap > 2 ||
                (long)requestedLeft + requestedWidth > imageWidth ||
                (long)requestedTop + requestedHeight > imageHeight)
                return JxrError.InvalidArgument;
            long left = (long)requestedLeft + extraLeft;
            long top = (long)requestedTop + extraTop;
            long width = requestedWidth, height = requestedHeight;
            long totalWidth = (long)imageWidth + extraLeft + extraRight;
            long totalHeight = (long)imageHeight + extraTop + extraBottom;
            if (overlap != 0 && !ignoreOverlap)
            {
                long extent = overlap == 2 ? 10 : 2;
                if (left > extent) { left -= extent; width += extent; }
                else { width += left; left = 0; }
                if (top > extent) { top -= extent; height += extent; }
                else { height += top; top = 0; }
                width += extent;
                height += extent;
                if (left + width > totalWidth) width = totalWidth - left;
                if (top + height > totalHeight) height = totalHeight - top;
            }
            long blockLeft = left >> 4, blockTop = top >> 4;
            long blockRight = (left + width + 15) >> 4;
            long blockBottom = (top + height + 15) >> 4;
            long paddingLeft = extraLeft + (long)requestedLeft - (blockLeft << 4);
            long paddingTop = extraTop + (long)requestedTop - (blockTop << 4);
            long paddingRight = ((blockRight - blockLeft) << 4) -
                requestedWidth - paddingLeft;
            long paddingBottom = ((blockBottom - blockTop) << 4) -
                requestedHeight - paddingTop;
            long outputWidth = ((blockRight - blockLeft) << 4) -
                paddingLeft - paddingRight;
            long outputHeight = ((blockBottom - blockTop) << 4) -
                paddingTop - paddingBottom;
            if (blockRight > Int32.MaxValue || blockBottom > Int32.MaxValue ||
                left > Int32.MaxValue || top > Int32.MaxValue ||
                width > Int32.MaxValue || height > Int32.MaxValue ||
                paddingLeft > Int32.MaxValue || paddingTop > Int32.MaxValue ||
                paddingRight > Int32.MaxValue || paddingBottom > Int32.MaxValue)
                return JxrError.UnsupportedFeature;
            result = new JxrTranscodeRoi((int)left, (int)top, (int)width,
                (int)height, (int)blockLeft, (int)blockTop, (int)blockRight,
                (int)blockBottom, (int)paddingLeft, (int)paddingTop,
                (int)paddingRight, (int)paddingBottom, (int)outputWidth,
                (int)outputHeight);
            return JxrError.None;
        }
    }

    public sealed class JxrTranscodeRoi
    {
        private readonly int[] values;
        internal JxrTranscodeRoi(params int[] values) { this.values = values; }
        public int ExpandedLeft { get { return values[0]; } }
        public int ExpandedTop { get { return values[1]; } }
        public int ExpandedWidth { get { return values[2]; } }
        public int ExpandedHeight { get { return values[3]; } }
        public int MacroblockLeft { get { return values[4]; } }
        public int MacroblockTop { get { return values[5]; } }
        public int MacroblockRight { get { return values[6]; } }
        public int MacroblockBottom { get { return values[7]; } }
        public int ExtraLeft { get { return values[8]; } }
        public int ExtraTop { get { return values[9]; } }
        public int ExtraRight { get { return values[10]; } }
        public int ExtraBottom { get { return values[11]; } }
        public int ImageWidth { get { return values[12]; } }
        public int ImageHeight { get { return values[13]; } }
    }
}
