# PPTX JXR corpus conformance runner

This developer tool compares managed decoding with the native C decoder for
JPEG XR files stored in PPTX packages. It reads each package entry in place,
uses the profiler's SHA-256 inventory, and extracts only one JXR at a time to a
temporary directory. It does not modify PPTX files or store their contents in
the repository.

The current corpus contains 5,890 PPTX files. The profiler report identifies
the unique assets and their profiles. `representatives` selects one asset per
parsed profile; `all` exercises each unique SHA-256 once. Repeated package
occurrences are shown by the profiler but do not cause duplicate decodes.

## Build

Build the managed bridge for the required .NET Framework 2.0 target:

```powershell
& 'C:\Windows\Microsoft.NET\Framework\v2.0.50727\MSBuild.exe' `
  .\managed\Jxr.Managed.CorpusRunner\Jxr.Managed.CorpusRunner.csproj `
  /p:Configuration=Release
```

The native decoder is `JXRDecApp.exe` from `jxrencoderdecoder/JXR_vc14.sln`,
configuration `Release|x64`.

## Run

The first run scans the PPTX root and writes the profiler cache/reports outside
the repository. Later scans reuse the cache:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --refresh-inventory --suite representatives --diagnostic `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl'
```

`--refresh-inventory` forces a fresh profiler scan (`--no-cache`) so parser
changes do not reuse profiles produced by an older parser version.

To validate managed source-profile extraction without decoding pixels or
requiring the native decoder, use:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-only `
  --profile-manifest 'D:\JxrReports\TestData\managed-profile-manifest-v2.jsonl' `
  --output 'D:\JxrReports\TestData\managed-profile-summary-v2.json'
```

This writes a versioned JSONL record per unique JXR with the managed main and
container profile, tile packet spans, tile QP/trim metadata where available,
and an explicit packet-parse completeness/error status. `packet_incomplete`
means the main/container profile was read and independently matched, but the
packet syntax reader declined or could not fully parse the packet map; this is
reported separately from malformed headers or metadata mismatches. Add
`--refresh-inventory` when the independent profiler parser has changed.

Use `--suite all` for every unique JXR. The JSON result contains one row per
asset, profile metadata, native and managed pixel SHA-256 values, and the first
pixel/channel that differs. `--diagnostic` is intended while known managed
gaps are being fixed: it returns success if processing completed, while keeping
managed decode failures and pixel mismatches visible in the report. A native
decode failure still returns nonzero because that sample has no C pixel oracle.
Without `--diagnostic`, any managed mismatch or decode failure also returns a
nonzero exit code.

To exercise the common frequency-coded, no-alpha family independently of
spatial images and planar-alpha files, add
`--profile-filter frequency-no-alpha`. This filter requires both
`bitstream_layout == "frequency"` and `alpha_mode == "none"`; the older
`has_alpha` profile bit alone does not distinguish planar-alpha assets.

The 2026-10-02 frequency/no-alpha corpus check is currently **95/128
pixel-identical**. The other 33 unique images are pre-existing differences in
the legacy `codestream_subversion=0`, `OL_ONE` branch; they are recorded by
input SHA-256 in `known-mismatches-frequency-no-alpha.jsonl`. This is a known
compatibility gap, not a completed decoder mode. Run the strict baseline gate
with:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter frequency-no-alpha `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl' `
  --corpus-manifest-content-only `
  --known-mismatches '.\tools\pptx-jxr-corpus\known-mismatches-frequency-no-alpha.jsonl'
```

The report retains `pixel_mismatch` for these assets and marks only the
allowlisted rows with `known_mismatch`. A new mismatch, decode error, changed
profile, or changed corpus inventory fails the gate. A formerly mismatching
asset becoming pixel-identical is allowed and counted as resolved; remove its
SHA from the allowlist once the fix is reviewed. The committed corpus index
predates a profiler metadata correction, so this command compares input SHA,
locations and parse status but not the profiler's derived profile fields;
the 33 allowed mismatches still require their pinned profile fields. Do not
use this baseline to claim full parity or to suppress failures in unrelated
profiles.

For a single extracted JXR, the managed bridge can also capture decoder state
for one macroblock without changing the normal decode path:

```powershell
Jxr.Managed.CorpusRunner.exe decode-trace input.jxr 0 0 trace.json
```

The trace contains DC/LP/HP bit ranges, coefficient snapshots after entropy,
prediction and dequantization, reconstructed Y/U/V samples, and final BGR
pixels for the requested macroblock. The native decoder's `-X <directory>`
trace uses the same stages; native reconstructed samples are captured when the
output row is consumed and converted from its internal `idxCC` ordering to
row-major pixel order. These traces are diagnostic output, not golden corpus
assets; matching stage names do not yet establish identical intermediate
semantics on the unresolved legacy overlap profile.

To save the full corpus index without running either decoder:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --manifest-only `
  --write-corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl'
```

This index has one row per unique SHA-256, with every relative PPTX/ZIP
location, occurrence count, parse status and profile metadata. It contains no
image bytes or absolute paths.

To verify a later profiler scan against the committed index, add
`--corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl'` to a
runner invocation. The report then flags added, removed, or changed JXR assets
and locations.

Record native pixel digests after reviewing a complete C-decoder run:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --record-reference '.\tools\pptx-jxr-corpus\reference-manifest.jsonl' `
  --diagnostic
```

Reference rows contain the JXR SHA-256, relative PPTX path and ZIP entry,
occurrence list, parsed profile, decoded dimensions, channel, and native pixel
digest; they do not include image bytes or absolute source paths.
`--reference-manifest` checks future native runs for changed profile or pixel
digests and, in the `all` suite, missing or new assets. Review the generated
file before tracking it, and never commit JXR or PPTX data from the external
test corpus. If a native decode fails, reference capture is marked partial in
`corpus-results.json`; successful native decodes still get pixel digests, and
the full source inventory remains available in the corpus index.

For alpha assets the native decoder produces BGRA32 once; the runner compares
color and alpha planes separately against managed color-only and alpha-only
decode. Color is normalized to top-down BGR24 and alpha to Gray8. Files without
alpha are compared as BGR24.

## Unit tests

```powershell
python -m unittest discover -s tools/pptx-jxr-corpus/tests -v
```

The tests cover profile representative selection, BMP orientation/padding and
palette normalization, and mismatch localization. The full corpus run is an
integration test and requires the external PPTX root plus both decoders.

`baseline-summary.json` records the first full run; `corpus-manifest.jsonl`
indexes all unique inputs and `reference-manifest.jsonl` stores native pixel
digests for assets that the C decoder could decode. The managed mismatches and
decode errors are the starting measurements for the next compatibility steps.
