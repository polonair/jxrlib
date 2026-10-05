# Untouched source baseline

This directory is an extracted copy of the complete tracked tree at upstream commit `dc6a33ae4d6563641dad2ec3ef9e663f35b92a1a` (`master`). No codec source or project file in this snapshot was edited as part of the managed port/refactoring work.

The original C application has been built with the installed MSVC toolset. On this Windows environment, the legacy project files build with the newer toolset by defining compatibility aliases for the POSIX `fopen64`/`fseeko64` calls used by the upstream memory-stream helper:

```powershell
$previousCL = $env:CL
$env:CL = '/Dfopen64=fopen /Dfseeko64=_fseeki64'
try {
    & 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' .\original\jxrencoderdecoder\JXR_vc14.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed: $LASTEXITCODE" }
}
finally {
    if ($null -eq $previousCL) { Remove-Item Env:\CL -ErrorAction SilentlyContinue }
    else { $env:CL = $previousCL }
}
```

The encoder and decoder are produced at:

- `jxrencoderdecoder/Release/JXREncApp/x64/JXREncApp.exe`
- `jxrencoderdecoder/Release/JXRDecApp/x64/JXRDecApp.exe`

The 4:2:0 utility variants are built by the same solution. The source and project files remain unchanged; the aliases are supplied only to the compiler invocation. On POSIX, the included `Makefile` can be used with a compatible C compiler and GNU Make.

The Git commit is the canonical provenance for this snapshot; the copy is placed here for convenient side-by-side inspection and building.
