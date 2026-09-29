$ErrorActionPreference = 'Stop'
$fixtureDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $fixtureDir '..\..')
$encoder = Join-Path $root 'jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe'
$decoder = Join-Path $root 'jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe'
if (-not (Test-Path -LiteralPath $encoder) -or -not (Test-Path -LiteralPath $decoder)) {
    throw 'Build native Release|x64 encoder and decoder first.'
}
$source = Join-Path $fixtureDir 'gray-31x19.bmp'
$cases = @(
    @{ Name = 'q1-no-flex'; Qp = 1; Subband = 1; Trim = 0 },
    @{ Name = 'q2-all'; Qp = 2; Subband = 0; Trim = 0 },
    @{ Name = 'q16-all'; Qp = 16; Subband = 0; Trim = 0 },
    @{ Name = 'q64-all'; Qp = 64; Subband = 0; Trim = 0 },
    @{ Name = 'q255-all'; Qp = 255; Subband = 0; Trim = 0 },
    @{ Name = 'q16-no-flex'; Qp = 16; Subband = 1; Trim = 0 },
    @{ Name = 'q16-trim3'; Qp = 16; Subband = 0; Trim = 3 },
    @{ Name = 'q16-trim15'; Qp = 16; Subband = 0; Trim = 15 },
    @{ Name = 'q16-no-hp'; Qp = 16; Subband = 2; Trim = 0 },
    @{ Name = 'q16-dc-only'; Qp = 16; Subband = 3; Trim = 0 }
)
foreach ($case in $cases) {
    $prefix = Join-Path $fixtureDir $case.Name
    $trace = Join-Path $fixtureDir ($case.Name + '-trace')
    New-Item -ItemType Directory -Force -Path $trace | Out-Null
    & $encoder -i $source -o ($prefix + '.jxr') -c 2 -d 0 -q $case.Qp `
        -l 0 -s $case.Subband -F $case.Trim -f -X $trace
    if ($LASTEXITCODE -ne 0) { throw "Native encoding failed: $($case.Name)" }
    & $decoder -i ($prefix + '.jxr') -o ($prefix + '-restored.bmp') -c 2 -a 0 -p 0
    if ($LASTEXITCODE -ne 0) { throw "Native decoding failed: $($case.Name)" }
}
