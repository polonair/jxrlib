$ErrorActionPreference = 'Stop'
$fixtureDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $fixtureDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}

$width = 32
$height = 32
$stride = $width * 3
$bmp = New-Object byte[] (54 + $stride * $height)
$bmp[0] = 66
$bmp[1] = 77
function Put16([int]$offset, [int]$value) {
    [Array]::Copy([BitConverter]::GetBytes([uint16]$value), 0, $bmp, $offset, 2)
}
function Put32([int]$offset, [int]$value) {
    [Array]::Copy([BitConverter]::GetBytes([uint32]$value), 0, $bmp, $offset, 4)
}
Put32 2 $bmp.Length
Put32 10 54
Put32 14 40
Put32 18 $width
Put32 22 $height
Put16 26 1
Put16 28 24
Put32 34 ($stride * $height)
Put32 38 3780
Put32 42 3780
for ($y = 0; $y -lt $height; $y++) {
    for ($x = 0; $x -lt $width; $x++) {
        $index = 54 + ($height - 1 - $y) * $stride + $x * 3
        $bmp[$index] = [byte](($x * 7 + $y * 53 + $x * $y) -band 255)
        $bmp[$index + 1] = [byte](($x * 13 + $y * 29 + 3 * $x * $y) -band 255)
        $bmp[$index + 2] = [byte](($x * 37 + $y * 11 + 5 * $x * $y) -band 255)
    }
}
$rgb = Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'
[IO.File]::WriteAllBytes($rgb, $bmp)

function NewRgbEdgeFixture([int]$imageWidth, [int]$imageHeight) {
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
    for ($py = 0; $py -lt $imageHeight; $py++) {
        for ($px = 0; $px -lt $imageWidth; $px++) {
            $at = 54 + ($imageHeight - 1 - $py) * $rowStride + $px * 3
            $data[$at] = [byte](($px * 7 + $py * 53 + $px * $py) -band 255)
            $data[$at + 1] = [byte](($px * 13 + $py * 29 + 3 * $px * $py) -band 255)
            $data[$at + 2] = [byte](($px * 37 + $py * 11 + 5 * $px * $py) -band 255)
        }
    }
    $path = Join-Path $fixtureDir ("rgb-overlap-${imageWidth}x${imageHeight}.bmp")
    [IO.File]::WriteAllBytes($path, $data)
    return $path
}
$rgb1 = NewRgbEdgeFixture 1 1
$rgbOdd = NewRgbEdgeFixture 31 19
$realSign = Join-Path $root 'real-image-profile\test-sign-334x330.bmp'

$cases = @(
    @{ Name = 'gray-31x19-ol1'; Source = (Join-Path $fixtureDir 'gray-31x19.bmp'); Color = 2; Format = 0; Qp = 1; Overlap = 1 },
    @{ Name = 'gray-31x19-ol2'; Source = (Join-Path $fixtureDir 'gray-31x19.bmp'); Color = 2; Format = 0; Qp = 1; Overlap = 2 },
    @{ Name = 'rgb444-32x32-ol1'; Source = $rgb; Color = 0; Format = 3; Qp = 16; Overlap = 1 },
    @{ Name = 'rgb444-32x32-ol2'; Source = $rgb; Color = 0; Format = 3; Qp = 16; Overlap = 2 },
    @{ Name = 'rgb422-32x32-ol0'; Source = $rgb; Color = 0; Format = 2; Qp = 16; Overlap = 0 },
    @{ Name = 'rgb422-32x32-ol1'; Source = $rgb; Color = 0; Format = 2; Qp = 16; Overlap = 1 },
    @{ Name = 'rgb422-32x32-ol2'; Source = $rgb; Color = 0; Format = 2; Qp = 16; Overlap = 2 },
    @{ Name = 'rgb420-32x32-ol0'; Source = $rgb; Color = 0; Format = 1; Qp = 16; Overlap = 0 },
    @{ Name = 'rgb420-32x32-ol1'; Source = $rgb; Color = 0; Format = 1; Qp = 16; Overlap = 1 },
    @{ Name = 'rgb420-32x32-ol2'; Source = $rgb; Color = 0; Format = 1; Qp = 16; Overlap = 2 },
    @{ Name = 'rgb422-32x32-q1-ol0'; Source = $rgb; Color = 0; Format = 2; Qp = 1; Overlap = 0 },
    @{ Name = 'rgb422-32x32-q1-ol1'; Source = $rgb; Color = 0; Format = 2; Qp = 1; Overlap = 1 },
    @{ Name = 'rgb422-32x32-q1-ol2'; Source = $rgb; Color = 0; Format = 2; Qp = 1; Overlap = 2 },
    @{ Name = 'rgb420-32x32-q1-ol0'; Source = $rgb; Color = 0; Format = 1; Qp = 1; Overlap = 0 },
    @{ Name = 'rgb420-32x32-q1-ol1'; Source = $rgb; Color = 0; Format = 1; Qp = 1; Overlap = 1 },
    @{ Name = 'rgb420-32x32-q1-ol2'; Source = $rgb; Color = 0; Format = 1; Qp = 1; Overlap = 2 },
    @{ Name = 'sign422-q16-ol1'; Source = $realSign; Color = 0; Format = 2; Qp = 16; Overlap = 1 },
    @{ Name = 'sign420-q16-ol2'; Source = $realSign; Color = 0; Format = 1; Qp = 16; Overlap = 2 }
)
foreach ($dimension in @(
    @{ Tag = '1x1'; Source = $rgb1 },
    @{ Tag = '31x19'; Source = $rgbOdd }
)) {
    foreach ($format in @(2, 1)) {
        foreach ($level in @(1, 2)) {
            if ($dimension.Tag -eq '1x1' -and $level -eq 2) { continue }
            $label = if ($format -eq 2) { '422' } else { '420' }
            $cases += @{ Name = "rgb${label}-$($dimension.Tag)-ol${level}";
                Source = $dimension.Source; Color = 0; Format = $format;
                Qp = 16; Overlap = $level }
        }
    }
}
foreach ($format in @(2, 1)) {
    $label = if ($format -eq 2) { '422' } else { '420' }
    foreach ($level in @(1, 2)) {
        $cases += @{ Name = "rgb${label}-31x19-q1-ol${level}";
            Source = $rgbOdd; Color = 0; Format = $format;
            Qp = 1; Overlap = $level }
    }
}
foreach ($case in $cases) {
    $jxr = Join-Path $fixtureDir ($case.Name + '.jxr')
    $restored = Join-Path $fixtureDir ($case.Name + '-restored.bmp')
    if ($case.Qp -eq 1) {
        & $encoder -i $case.Source -o $jxr -c $case.Color -d $case.Format `
            -q $case.Qp -l $case.Overlap -f -p -u
    }
    else {
        & $encoder -i $case.Source -o $jxr -c $case.Color -d $case.Format `
            -q $case.Qp -l $case.Overlap -f -p
    }
    if ($LASTEXITCODE -ne 0) { throw "Native encoding failed: $($case.Name)" }
    & $decoder -i $jxr -o $restored -c $case.Color -a 0 -p 0
    if ($LASTEXITCODE -ne 0) { throw "Native decoding failed: $($case.Name)" }
}
