$ErrorActionPreference = 'Stop'
$fixtureDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $fixtureDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}

function NewRgbBitmap([int]$imageWidth, [int]$imageHeight, [string]$path) {
    $rowStride = ($imageWidth * 3 + 3) -band (-bnot 3)
    $data = New-Object byte[] (54 + $rowStride * $imageHeight)
    $data[0] = 66
    $data[1] = 77
    [Array]::Copy([BitConverter]::GetBytes([uint32]$data.Length), 0, $data, 2, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint32]54), 0, $data, 10, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint32]40), 0, $data, 14, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint32]$imageWidth), 0, $data, 18, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint32]$imageHeight), 0, $data, 22, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint16]1), 0, $data, 26, 2)
    [Array]::Copy([BitConverter]::GetBytes([uint16]24), 0, $data, 28, 2)
    [Array]::Copy([BitConverter]::GetBytes([uint32]($rowStride * $imageHeight)), 0, $data, 34, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint32]3780), 0, $data, 38, 4)
    [Array]::Copy([BitConverter]::GetBytes([uint32]3780), 0, $data, 42, 4)
    for ($y = 0; $y -lt $imageHeight; $y++) {
        for ($x = 0; $x -lt $imageWidth; $x++) {
            $at = 54 + ($imageHeight - 1 - $y) * $rowStride + $x * 3
            $data[$at] = [byte](($x * 7 + $y * 53 + $x * $y) -band 255)
            $data[$at + 1] = [byte](($x * 13 + $y * 29 + 3 * $x * $y) -band 255)
            $data[$at + 2] = [byte](($x * 37 + $y * 11 + 5 * $x * $y) -band 255)
        }
    }
    [IO.File]::WriteAllBytes($path, $data)
}

$wideRgb = Join-Path $fixtureDir 'rgb-48x32-tiles.bmp'
NewRgbBitmap 48 32 $wideRgb
$cases = @(
    @{ Name = 'gray32-tiles2x2-q1-ol0'; Source = (Join-Path $fixtureDir 'gray-32x32.bmp'); Color = 2; Format = 0; Qp = 1; Overlap = 0; Grid = @('-U', '2', '2') },
    @{ Name = 'gray32-tiles2x2-q16-ol2'; Source = (Join-Path $fixtureDir 'gray-32x32.bmp'); Color = 2; Format = 0; Qp = 16; Overlap = 2; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb444-tiles2x2-q16-ol0'; Source = (Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color = 0; Format = 3; Qp = 16; Overlap = 0; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb444-tiles2x2-q16-ol2'; Source = (Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color = 0; Format = 3; Qp = 16; Overlap = 2; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb422-tiles2x2-q16-ol0'; Source = (Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color = 0; Format = 2; Qp = 16; Overlap = 0; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb422-tiles2x2-q16-ol2'; Source = (Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color = 0; Format = 2; Qp = 16; Overlap = 2; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb420-tiles2x2-q16-ol0'; Source = (Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color = 0; Format = 1; Qp = 16; Overlap = 0; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb420-tiles2x2-q16-ol2'; Source = (Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color = 0; Format = 1; Qp = 16; Overlap = 2; Grid = @('-U', '2', '2') },
    @{ Name = 'rgb422-tiles-v1-2-q16-ol2'; Source = $wideRgb; Color = 0; Format = 2; Qp = 16; Overlap = 2; Grid = @('-V', '1', '2') },
    @{ Name = 'rgb420-tiles31x19-q1-ol2'; Source = (Join-Path $fixtureDir 'rgb-overlap-31x19.bmp'); Color = 0; Format = 1; Qp = 1; Overlap = 2; Grid = @('-U', '2', '2') }
)

foreach ($case in $cases) {
    $jxr = Join-Path $fixtureDir ($case.Name + '.jxr')
    $restored = Join-Path $fixtureDir ($case.Name + '-restored.bmp')
    $arguments = @('-i', $case.Source, '-o', $jxr, '-c', $case.Color,
        '-d', $case.Format, '-q', $case.Qp, '-l', $case.Overlap, '-f', '-p')
    $arguments += $case.Grid
    if ($case.Qp -eq 1) { $arguments += '-u' }
    & $encoder @arguments
    if ($LASTEXITCODE -ne 0) { throw "Native encoding failed: $($case.Name)" }
    & $decoder -i $jxr -o $restored -c $case.Color -a 0 -p 0
    if ($LASTEXITCODE -ne 0) { throw "Native decoding failed: $($case.Name)" }
}
