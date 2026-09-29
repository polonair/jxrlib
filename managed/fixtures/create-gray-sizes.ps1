$ErrorActionPreference = 'Stop'
$profileDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $profileDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}

foreach ($size in @(@(1,1), @(15,17), @(16,16), @(32,32), @(31,19),
    @(17,1), @(33,18), @(257,17))) {
    [int]$width = $size[0]
    [int]$height = $size[1]
    [int]$stride = ($width + 3) -band (-bnot 3)
    [int]$pixelOffset = 1078
    [int]$fileSize = $pixelOffset + $stride * $height
    $bytes = [byte[]]::new($fileSize)
    $bytes[0] = 66; $bytes[1] = 77
    [Array]::Copy([BitConverter]::GetBytes($fileSize), 0, $bytes, 2, 4)
    [Array]::Copy([BitConverter]::GetBytes($pixelOffset), 0, $bytes, 10, 4)
    [Array]::Copy([BitConverter]::GetBytes(40), 0, $bytes, 14, 4)
    [Array]::Copy([BitConverter]::GetBytes($width), 0, $bytes, 18, 4)
    [Array]::Copy([BitConverter]::GetBytes($height), 0, $bytes, 22, 4)
    $bytes[26] = 1; $bytes[28] = 8
    [Array]::Copy([BitConverter]::GetBytes($stride * $height), 0, $bytes, 34, 4)
    [Array]::Copy([BitConverter]::GetBytes(3779), 0, $bytes, 38, 4)
    [Array]::Copy([BitConverter]::GetBytes(3779), 0, $bytes, 42, 4)
    for ($index = 0; $index -lt 256; $index++) {
        $entry = 54 + $index * 4
        $bytes[$entry] = $bytes[$entry + 1] = $bytes[$entry + 2] = [byte]$index
    }
    for ($y = 0; $y -lt $height; $y++) {
        for ($x = 0; $x -lt $width; $x++) {
            $value = (($x * 73 + $y * 41 + $x * $y * 13) -bxor (($x + 5) * ($y + 3))) -band 255
            $bytes[$pixelOffset + ($height - 1 - $y) * $stride + $x] = [byte]$value
        }
    }
    $name = "gray-$($width)x$($height)"
    $bmp = Join-Path $profileDir "$name.bmp"
    $jxr = Join-Path $profileDir "$name.jxr"
    $restored = Join-Path $profileDir "$name-restored.bmp"
    [IO.File]::WriteAllBytes($bmp, $bytes)
    & $encoder -i $bmp -o $jxr -c 2 -d 0 -q 1 -l 0 -f
    if ($LASTEXITCODE -ne 0) { throw "Encoder failed for $name" }
    & $decoder -i $jxr -o $restored -c 2 -a 0 -p 0
    if ($LASTEXITCODE -ne 0) { throw "Decoder failed for $name" }
    $originalHash = (Get-FileHash -LiteralPath $bmp -Algorithm SHA256).Hash
    $restoredHash = (Get-FileHash -LiteralPath $restored -Algorithm SHA256).Hash
    if ($originalHash -ne $restoredHash) { throw "BMP round-trip differs for $name" }
}
