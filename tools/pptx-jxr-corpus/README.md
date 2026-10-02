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

Use `--suite all` for every unique JXR. The JSON result contains one row per
asset, profile metadata, native and managed pixel SHA-256 values, and the first
pixel/channel that differs. `--diagnostic` is intended while known managed
gaps are being fixed: it returns success if processing completed, while keeping
managed decode failures and pixel mismatches visible in the report. A native
decode failure still returns nonzero because that sample has no C pixel oracle.
Without `--diagnostic`, any managed mismatch or decode failure also returns a
nonzero exit code.

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
