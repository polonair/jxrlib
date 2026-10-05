# JPEG XR reference and managed port

This repository keeps three useful states of the codec together:

1. [`original/`](original/PORTING_BASELINE.md) is an untouched snapshot of the upstream C application at the common starting revision, before the porting/refactoring work.
2. The C sources in the repository root are the refactored native reference application. They retain their established directory layout so the checked-in Makefile and Visual Studio projects continue to build them.
3. [`managed/Jxr.Managed.Core`](managed/Jxr.Managed.Core) is the fully managed C# library targeting .NET Framework 2.0. The managed solution also contains its test runner and corpus runner.

The directory map, build entry points, and locations of fixtures and supporting materials are in [REPOSITORY_LAYOUT.md](REPOSITORY_LAYOUT.md).

## Build

Build the refactored native reference application with the portable Makefile (GNU Make and a C compiler required):

```sh
make
```

On Windows with the newer MSVC toolset, the legacy project can be built by overriding the toolset and aliasing the upstream POSIX file calls for Windows:

```powershell
$env:CL = '/Dfopen64=fopen /Dfseeko64=_fseeki64'
msbuild .\jxrencoderdecoder\JXR_vc14.sln /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145
Remove-Item Env:\CL
```

The native conformance executable is built by that solution at `jxrencoderdecoder/Release/JxrConformanceTests/x64/JxrConformanceTests.exe`.

Build the managed library and its companion projects with the .NET Framework 2.0 MSBuild toolchain:

```powershell
& 'C:\Windows\Microsoft.NET\Framework\v2.0.50727\MSBuild.exe' .\managed\Jxr.Managed.sln /p:Configuration=Release
```

Run the managed tests with:

```powershell
& .\managed\Jxr.Managed.Tests\bin\Release\Jxr.Managed.Tests.exe
```

The untouched baseline is independently buildable from `original/`; its Visual Studio solution is `original/jxrencoderdecoder/JXR_vc14.sln` and its portable Makefile is `original/Makefile`.
