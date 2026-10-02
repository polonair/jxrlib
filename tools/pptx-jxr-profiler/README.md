# PPTX JXR Profiler

Read-only inventory tool for finding JPEG XR streams embedded in `.pptx` packages and grouping them by codestream profile. It uses only Python's standard library and never extracts files to disk or modifies presentations.

## Run

```text
python tools/pptx-jxr-profiler/profiler.py --root "D:\ASPOSE\SLIDESNET\TestData" --out "D:\JxrReports\TestData"
```

Useful options:

- `--no-cache` rescans every presentation.
- `--max-entry-mb N` limits the uncompressed size of a candidate JXR entry (default 512 MiB).
- `--progress-every N` prints progress every N presentations; `0` disables it.

The scanner looks recursively for `.pptx` files, recognizes JXR by its codestream/container signature as well as common extensions (`.jxr`, `.wdp`, `.hdp`, `.hdr`, `.jpegxr`), and inspects ZIP entries in place. Extension-only candidates with a non-JXR signature and malformed JXR candidates are recorded as issues instead of silently discarded. Relationship links from DrawingML `a:blip` and `a14:imgLayer` elements are included when resolvable.

## Reports

- `summary.md` — totals and the most common feature profiles.
- `run.json` — run metadata and counts.
- `profiles.csv` / `profiles.json` — aggregated profile groups, counts, and examples.
- `occurrences.csv` / `occurrences.json` — every JXR occurrence and its PPTX relationship context.
- `assets.jsonl` — one representative row per unique JXR SHA-256, with duplicate occurrence counts.
- `errors.csv` / `errors.json` — corrupt packages, candidate signature mismatches, and parser/read issues.
- `profiler-cache.sqlite3` — local incremental cache; safe to delete to force a fresh scan.

Each profile describes the metadata actually available in the codestream/container: image size, source pixel format and bit depth, color subsampling, overlap, orientation, alpha signaling, spatial/frequency layout, tile grid, subbands, and frame-level quantizer syntax. The aggregated profile key includes the frame quantizer syntax, including per-channel indices when present. TIFF-like files in the reference corpus use both absolute-end and byte-count interpretations for the planar-alpha range field; the report preserves the raw `alpha_range_tag_value`, emits the selected `alpha_range_interpretation`, and normalizes the result as `alpha_offset`, `alpha_end_offset`, and `alpha_byte_count`. The report marks frame quantizers separately from tile quantizers. **Tile packet quantizer overrides and packet payloads are not decoded by this inventory tool**, so do not interpret inherited/default frame values as the effective quality settings of every tile. This profiler is intended to characterize the PPTX corpus and guide the JPEG XR compatibility profile; it is not a full JPEG XR validator or decoder.

The cache key includes the scanner version, file size and modification time, and the configured maximum entry size. Reports are rebuilt on each run; the cache is only an optimization.

## Tests

```text
python -m unittest discover -s tools/pptx-jxr-profiler/tests -v
```

The tests use the repository's minimal JXR reference fixture and synthetic PPTX ZIP packages, covering raw and TIFF-like JXR headers, hidden extensions, DrawingML relationships, malformed entries, duplicate assets, and cache invalidation.
