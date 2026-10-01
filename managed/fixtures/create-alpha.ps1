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
$stride = $width * 4
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
Put16 28 32
Put32 34 ($stride * $height)
Put32 38 3780
Put32 42 3780
for ($y = 0; $y -lt $height; $y++) {
    for ($x = 0; $x -lt $width; $x++) {
        $index = 54 + ($height - 1 - $y) * $stride + $x * 4
        $bmp[$index] = [byte](($x * 7 + $y * 53 + $x * $y) -band 255)
        $bmp[$index + 1] = [byte](($x * 13 + $y * 29 + 3 * $x * $y) -band 255)
        $bmp[$index + 2] = [byte](($x * 37 + $y * 11 + 5 * $x * $y) -band 255)
        if (($x + $y) % 4 -eq 0) { $alpha = 0 }
        elseif (($x + $y) % 4 -eq 1) { $alpha = 255 }
        else { $alpha = ($x * 9 + $y * 17) -band 255 }
        $bmp[$index + 3] = [byte]$alpha
    }
}
$source = Join-Path $fixtureDir 'alpha-bgra-32x32.bmp'
[IO.File]::WriteAllBytes($source, $bmp)

foreach ($mode in @(2, 3)) {
    $tag = if ($mode -eq 2) { 'planar' } else { 'interleaved' }
    foreach ($quality in @(1, 16)) {
        $name = "alpha-$tag-q${quality}-32x32"
        $jxr = Join-Path $fixtureDir ($name + '.jxr')
        $full = Join-Path $fixtureDir ($name + '-restored.bmp')
        $color = Join-Path $fixtureDir ($name + '-color.bmp')
        $only = Join-Path $fixtureDir ($name + '-only.bmp')
        & $encoder -i $source -o $jxr -c 22 -a $mode -q 16 -Q $quality -l 0 -f -p
        if ($LASTEXITCODE -ne 0) { throw "Native encoding failed: $name" }
        & $decoder -i $jxr -o $full -c 22 -a 2 -p 0
        if ($LASTEXITCODE -ne 0) { throw "Native full decode failed: $name" }
        & $decoder -i $jxr -o $color -c 22 -a 0 -p 0
        if ($LASTEXITCODE -ne 0) { throw "Native color-only decode failed: $name" }
        if ($mode -eq 2) {
            & $decoder -i $jxr -o $only -c 2 -a 1 -p 0
            if ($LASTEXITCODE -ne 0) { throw "Native alpha-only decode failed: $name" }
        }
    }
}
