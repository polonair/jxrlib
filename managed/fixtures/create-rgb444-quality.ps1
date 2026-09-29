$ErrorActionPreference = 'Stop'
$fixtureDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $fixtureDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
$signSource = Join-Path $root 'real-image-profile\test-sign-334x330.bmp'
$citySource = Join-Path $root 'default-profile\city-park-605x478.bmp'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}
$cases = @(
    @{ Name = 'rgb444-q16-all'; Source = $signSource; Qp = 16; Subband = 0; Trim = 0 },
    @{ Name = 'rgb444-q16-no-flex'; Source = $signSource; Qp = 16; Subband = 1; Trim = 0 },
    @{ Name = 'rgb444-q16-no-hp'; Source = $signSource; Qp = 16; Subband = 2; Trim = 0 },
    @{ Name = 'rgb444-q16-dc-only'; Source = $signSource; Qp = 16; Subband = 3; Trim = 0 },
    @{ Name = 'rgb444-q16-trim3'; Source = $signSource; Qp = 16; Subband = 0; Trim = 3 },
    @{ Name = 'rgb444-city-q16-all'; Source = $citySource; Qp = 16; Subband = 0; Trim = 0 }
)
foreach ($case in $cases) {
    $prefix = Join-Path $fixtureDir $case.Name
    & $encoder -i $case.Source -o ($prefix + '.jxr') -c 0 -d 3 -q $case.Qp `
        -l 0 -s $case.Subband -F $case.Trim -f -p
    if ($LASTEXITCODE -ne 0) { throw "Native encoding failed: $($case.Name)" }
    & $decoder -i ($prefix + '.jxr') -o ($prefix + '-restored.bmp') -c 0 -a 0 -p 0
    if ($LASTEXITCODE -ne 0) { throw "Native decoding failed: $($case.Name)" }
}
