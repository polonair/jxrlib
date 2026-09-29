$ErrorActionPreference = 'Stop'
$fixtureDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $fixtureDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}
$width = 15
$height = 17
$stride = ($width * 3 + 3) -band (-bnot 3)
$bmp = New-Object byte[] (54 + $stride * $height)
$bmp[0] = [byte][char]'B'
$bmp[1] = [byte][char]'M'
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
        $bmp[$index] = [byte](($x * 7 + $y * 53) -band 255)
        $bmp[$index + 1] = [byte](($x * 13 + $y * 29) -band 255)
        $bmp[$index + 2] = [byte](($x * 37 + $y * 11) -band 255)
    }
}
$source = Join-Path $fixtureDir 'rgb444-15x17.bmp'
$jxr = Join-Path $fixtureDir 'rgb444-15x17.jxr'
$restored = Join-Path $fixtureDir 'rgb444-15x17-restored.bmp'
[IO.File]::WriteAllBytes($source, $bmp)
& $encoder -i $source -o $jxr -c 0 -d 3 -q 1 -l 0 -f -p
if ($LASTEXITCODE -ne 0) { throw 'Native RGB444 encoding failed.' }
& $decoder -i $jxr -o $restored -c 0 -a 0 -p 0
if ($LASTEXITCODE -ne 0) { throw 'Native RGB444 decoding failed.' }
