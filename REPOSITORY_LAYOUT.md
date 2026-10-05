# Repository layout

The native and managed implementations are kept side by side so that the C application remains a reference oracle for the managed port.

| Area | Contents | Build or use |
| --- | --- | --- |
| `original/` | Untouched upstream C application snapshot from commit `dc6a33ae4d6563641dad2ec3ef9e663f35b92a1a` (`master`), the common base before porting/refactoring changes. | `original/jxrencoderdecoder/JXR_vc14.sln` or `original/Makefile`; provenance and instructions in [`original/PORTING_BASELINE.md`](original/PORTING_BASELINE.md). |
| Repository root: `common/`, `image/`, `jxrencoderdecoder/`, `jxrgluelib/`, `jxrtestlib/` | Refactored native C application and its conformance runner. This is the post-refactoring C reference implementation, not the managed product. | `make` with GNU Make and a C compiler, or `jxrencoderdecoder/JXR_vc14.sln` with the Windows compatibility build described in the root README; `JxrConformanceTests` is included in the solution. |
| `managed/Jxr.Managed.Core/` | Final fully managed codec library, targeting .NET Framework 2.0 and using no P/Invoke. | `managed/Jxr.Managed.sln`; output is `managed/Jxr.Managed.Core/bin/Release/Jxr.Managed.Core.dll`. |
| `managed/Jxr.Managed.Tests/`, `managed/Jxr.Managed.CorpusRunner/` | Managed regression tests and PPTX/JXR corpus runner. | Also built by `managed/Jxr.Managed.sln`. |
| `analysis/`, `doc/` | Design notes, compatibility analysis, algorithm documentation, and upstream specifications. | Reference material. |
| `managed/fixtures/`, `default-profile/`, `minimal-profile/`, `real-image-profile/` | Frozen input/output samples used for native-vs-managed conformance. | Used by the managed and native test suites; do not move without updating fixture path resolution. |
| `tests/` | Native conformance test harness and temporary test workspace location. | Included in the native Visual Studio solution. |
| `tools/` | PPTX JXR profiler, corpus tools, and supporting scripts. | Tool-specific instructions are in each tool directory. |
| `bin/` | Small upstream utility binaries retained with the original project data. | Not required to build either codec implementation. |

## Why the native source directories remain at the root

The native Makefile and Visual Studio project files use paths relative to the existing upstream directory layout. Keeping those paths avoids a broad mechanical rewrite and lets the refactored C application remain directly buildable. The pristine baseline is isolated under `original/`; the managed implementation is already isolated under `managed/`.

The checked-in VC11/VC12 project files are retained as historical project variants. The maintained post-refactor Windows entry point is the VC14 solution; the older project variants have not been updated to the extracted module lists.

## Build commands

From the repository root in a Visual Studio Developer PowerShell:

```powershell
msbuild .\jxrencoderdecoder\JXR_vc14.sln /p:Configuration=Release /p:Platform=x64
```

Build the managed solution using the .NET Framework 2.0 MSBuild:

```powershell
& 'C:\Windows\Microsoft.NET\Framework\v2.0.50727\MSBuild.exe' .\managed\Jxr.Managed.sln /p:Configuration=Release
& .\managed\Jxr.Managed.Tests\bin\Release\Jxr.Managed.Tests.exe
```

The untouched baseline can be built separately by opening `original/jxrencoderdecoder/JXR_vc14.sln` or running its `Makefile` in an environment with a C compiler and Make.
