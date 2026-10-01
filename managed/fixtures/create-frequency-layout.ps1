$ErrorActionPreference = 'Stop'
$fixtureDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $fixtureDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}

$cases = @(
    @{ Name='frequency-gray-16x16'; Source=(Join-Path $root 'minimal-profile\minimal-gray-16x16.bmp'); Color=2; Format=0; Extra=@('-u') },
    @{ Name='frequency-gray32-tile2x2'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-u','-U','2','2') },
    @{ Name='frequency-gray32-tile2x2-sequential'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-u','-U','2','2','-p') },
    @{ Name='frequency-rgb444-tile2x2'; Source=(Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color=0; Format=3; Extra=@('-u','-U','2','2') },
    @{ Name='frequency-rgb422-tile2x2'; Source=(Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color=0; Format=2; Extra=@('-u','-U','2','2') },
    @{ Name='frequency-rgb420-tile2x2'; Source=(Join-Path $fixtureDir 'rgb-overlap-32x32.bmp'); Color=0; Format=1; Extra=@('-u','-U','2','2') },
    @{ Name='frequency-gray32-q16'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-q','16') },
    @{ Name='frequency-gray32-q16-trim2'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-q','16','-F','2') },
    @{ Name='frequency-gray32-q16-noflex'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-q','16','-s','1') },
    @{ Name='frequency-gray32-q16-nohp'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-q','16','-s','2') },
    @{ Name='frequency-gray32-q16-dconly'; Source=(Join-Path $fixtureDir 'gray-32x32.bmp'); Color=2; Format=0; Extra=@('-q','16','-s','3') }
)

foreach ($case in $cases) {
    $jxr = Join-Path $fixtureDir ($case.Name + '.jxr')
    $restored = Join-Path $fixtureDir ($case.Name + '-restored.bmp')
    $arguments = @('-i',$case.Source,'-o',$jxr,'-c',$case.Color,
        '-d',$case.Format,'-l','0') + $case.Extra
    & $encoder @arguments
    if ($LASTEXITCODE -ne 0) { throw "Native encoding failed: $($case.Name)" }
    & $decoder -i $jxr -o $restored -c $case.Color -a 0 -p 0
    if ($LASTEXITCODE -ne 0) { throw "Native decoding failed: $($case.Name)" }
}
