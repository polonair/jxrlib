$ErrorActionPreference = 'Stop'

$outputPath = Join-Path $PSScriptRoot 'minimal-gray-16x16.bmp'
$width = 16
$height = 16
$rowStride = (($width + 3) -band (-bnot 3))
$pixelDataSize = $rowStride * $height
$paletteSize = 256 * 4
$pixelOffset = 14 + 40 + $paletteSize
$fileSize = $pixelOffset + $pixelDataSize
$pixelsPerMeter = 3779

$stream = [System.IO.File]::Create($outputPath)
try {
    $writer = New-Object System.IO.BinaryWriter($stream)
    try {
        $writer.Write([byte[]][char[]]'BM')
        $writer.Write([UInt32]$fileSize)
        $writer.Write([UInt16]0)
        $writer.Write([UInt16]0)
        $writer.Write([UInt32]$pixelOffset)

        $writer.Write([UInt32]40)
        $writer.Write([Int32]$width)
        $writer.Write([Int32]$height)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]8)
        $writer.Write([UInt32]0)
        $writer.Write([UInt32]$pixelDataSize)
        $writer.Write([Int32]$pixelsPerMeter)
        $writer.Write([Int32]$pixelsPerMeter)
        $writer.Write([UInt32]0)
        $writer.Write([UInt32]0)

        for ($value = 0; $value -lt 256; $value++) {
            $writer.Write([byte]$value)
            $writer.Write([byte]$value)
            $writer.Write([byte]$value)
            $writer.Write([byte]0)
        }

        for ($y = 0; $y -lt $height; $y++) {
            for ($x = 0; $x -lt $width; $x++) {
                $writer.Write([byte](($x * 17 + $y * 11) -band 255))
            }
        }
    }
    finally {
        $writer.Dispose()
    }
}
finally {
    $stream.Dispose()
}

Write-Output "Created $outputPath"
