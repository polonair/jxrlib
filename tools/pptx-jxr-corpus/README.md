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

The `--encode-profile-round-trip` mode verifies profile-guided encoding for
this family. It decodes source pixels with the native C decoder (so the known
managed legacy-overlap decode mismatches do not contaminate encoder testing),
re-encodes them through `JxrCodec.Encode(image, sourceProfile)`, checks the
resulting stream's semantic profile, and verifies that the native C decoder
accepts it. It records pixel drift from the lossy re-encode; the initial
quality gate is mean absolute component error <= 8 and maximum component
delta <= 96 against the native-decoded source. Packet offsets, codestream
length, and encoded bytes are intentionally not required to match the source.

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter frequency-no-alpha --encode-profile-round-trip `
  --output 'D:\JxrReports\TestData\frequency-no-alpha-reencode.json'
```

The frequency/no-alpha corpus check is currently **95/128 pixel-identical**.
The other 33 unique images are differences in the legacy
`codestream_subversion=0`, `OL_ONE` branch; they are recorded by input SHA-256
and exact per-channel pixel hashes/metrics in
`known-mismatches-frequency-no-alpha.jsonl`. This is a known compatibility gap,
not a completed decoder mode. Run the strict baseline gate with:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter frequency-no-alpha `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl' `
  --known-mismatches '.\tools\pptx-jxr-corpus\known-mismatches-frequency-no-alpha.jsonl'
```

The report retains `pixel_mismatch` for these assets and marks only the
allowlisted rows with `known_mismatch`. A new mismatch, decode error, changed
profile, or changed corpus inventory fails the gate. A formerly mismatching
asset becoming pixel-identical is allowed and counted as resolved; remove its
SHA from the allowlist once the fix is reviewed. The allowlist is specific to
this profile and must not suppress failures in unrelated profiles.

Frequency-coded planar-alpha files can be checked separately. The refreshed
corpus scan found 83 planar-alpha assets in total; 82 belong to the frequency
family (one spatial-alpha profile remains outside this step). Current alpha
range tags in all 83 use `byte_count`; the corpus index is refreshed to retain
the corrected interpretation. The frequency/planar-alpha gate decodes color
and alpha channels against C, and checks two `Pbgra32` (`.c910`) assets against
independent references. One C reference is produced from the byte-identical
stream after changing only its pixel-format GUID to straight BGRA; this
test-only normalization is valid because every alpha sample in that asset is
255. Windows Imaging Component also decodes it independently. The second
`.c910` asset matches both C (after GUID normalization) and WIC exactly.

The current frequency/planar-alpha baseline is **71/82 pixel-identical**; the
other 11 exact inputs are pinned with per-channel digests and mismatch metrics
in `known-mismatches-frequency-planar-alpha.jsonl`. All 11 mismatches are in
`codestream_subversion=0`, `OL_ONE`. They remain visible as mismatches; only
these exact pinned cases are accepted by the strict gate:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter frequency-planar-alpha `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl' `
  --independent-reference-manifest '.\tools\pptx-jxr-corpus\independent-pbgra-references.jsonl' `
  --known-mismatches '.\tools\pptx-jxr-corpus\known-mismatches-frequency-planar-alpha.jsonl'
```

This gate requires all 82 inputs to be selected, C and managed readers to
agree with current profile metadata, zero decode/native errors, zero unmatched
independent references, and no changed or new pixel mismatch. This is a
decode-only gate; the separate encode gate follows.

Profile-guided re-encoding of frequency/planar-alpha inputs is exercised
separately with `--encode-profile-round-trip`. It feeds C-decoded color and
alpha pixels to the managed encoder, then checks both output plane profiles,
the output's C-decoder acceptance, color drift (MAE <= 8 and max delta <= 96),
and exact alpha preservation (the corpus alpha QP is 0):

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter frequency-planar-alpha --encode-profile-round-trip `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl' `
  --independent-reference-manifest '.\tools\pptx-jxr-corpus\independent-pbgra-references.jsonl' `
  --output 'D:\JxrReports\TestData\frequency-planar-alpha-reencode.json'
```

This covers the 80 straight-BGRA and two premultiplied-BGRA profiles. For the
two PBGRA assets, the legacy C decoder is invoked on a temporary byte-identical
copy with only the TIFF-like pixel-format GUID value changed to BGRA; independent
references pin the source pixels and prevent this compatibility workaround
from silently changing PBGRA interpretation. Alpha mismatches are never
allowlisted by the decoder baseline. In particular, an encode round-trip that
does not preserve alpha exactly remains a visible quality mismatch even when
the same asset is in the legacy decoder mismatch set.

On the 2026-10-05 corpus run, all **82/82** output streams retained their
semantic profile and were accepted by the native C decoder, including both
PBGRA inputs. Color stayed within the quality gate for every file. Exact alpha
round-trip passed for **70/82**; the 12 remaining quality mismatches all use
`codestream_subversion=0`, `OL_ONE`, including 11 inputs already known to
exercise the legacy overlap path. The same alpha deltas are present when the
managed decoder reads those re-encoded outputs, so this is not limited to the
native decoder. They remain strict quality failures, not waived results, and
are tracked for the follow-up legacy-overlap compatibility work. Full JSON
measurements are written outside the repository at
`D:\JxrReports\TestData\frequency-planar-alpha-reencode-2026-10-05-final.json`.

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

## Spatial stream decoding

The corpus contains 28 spatial profiles (27 without alpha and one with planar
alpha). Run all of them against the C decoder with:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter spatial `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl' `
  --known-mismatches '.\tools\pptx-jxr-corpus\known-mismatches-spatial.jsonl'
```

The decoder handles the spatial packet-length forms used here, including the
one-byte escape marker, and applies the shared chroma QP to both U and V. In
the 2026-10-05 corpus run all 28 color streams and the planar-alpha stream
decoded successfully. Alpha matched C exactly; color was not yet pixel-identical
for 28/28 assets. The exact input/profile/output differences are pinned in
`known-mismatches-spatial.jsonl`; they are all version-1, subversion-0,
`OL_ONE` streams and remain visible as `pixel_mismatch`/`known_mismatch`. The
largest observed color delta was 37. A new decode error, changed digest or
metric, changed profile, or changed corpus inventory fails the strict baseline.
This records current behavior; it does not waive or claim exact spatial color
parity.

## Spatial profile re-encoding

Re-encode all spatial corpus assets with their source profile and validate both
the profile and decoded quality against the native C decoder:

```powershell
python .\tools\pptx-jxr-corpus\corpus.py `
  --root 'D:\ASPOSE\SLIDESNET\TestData' `
  --report-dir 'D:\JxrReports\TestData' `
  --native-decoder '.\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe' `
  --managed-runner '.\managed\Jxr.Managed.CorpusRunner\bin\Release\Jxr.Managed.CorpusRunner.exe' `
  --suite all --profile-filter spatial --encode-profile-round-trip `
  --corpus-manifest '.\tools\pptx-jxr-corpus\corpus-manifest.jsonl' `
  --output 'D:\JxrReports\TestData\spatial-profile-reencode.json'
```

The runner requires the output to preserve the semantic packet/profile fields,
be accepted by both decoders, and remain within MAE 8 / maximum component delta
96 of the C-decoded source. For unchanged planar alpha it reuses the original
lossless alpha codestream only after checking that the input alpha pixels match
its decoded samples; changed alpha is encoded normally. The 2026-10-05 run
passed all 28 spatial assets, including planar alpha, with no profile, quality,
decoder, or input-manifest mismatches. The separate known source-decoder color
mismatch baseline remains unchanged and is not used to excuse re-encode errors.

## Unit tests

```powershell
python -m unittest discover -s tools/pptx-jxr-corpus/tests -v
```

The tests cover profile representative selection, BMP orientation/padding and
palette normalization, mismatch localization and metrics, independent PBGRA
reference validation, and a native/managed fixture run. The full corpus run is
an integration test and requires the external PPTX root plus both decoders.

`baseline-summary.json` records the first full run; `corpus-manifest.jsonl`
indexes all unique inputs and `reference-manifest.jsonl` stores native pixel
digests for assets that the C decoder could decode. The managed mismatches and
decode errors are the starting measurements for the next compatibility steps.
