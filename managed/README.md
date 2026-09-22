# Managed JPEG XR port

This directory contains the fully managed C# port, kept separate from the
native reference implementation.  Every project targets .NET Framework 2.0
and uses no P/Invoke, `unsafe` code or external test framework.

The first vertical slice is `Jxr.Managed.Core` and its console test runner.
It ports adaptive-Huffman state, the built-in JPEG XR catalogs for alphabets
4/5/6/7/8/9/12, table decoder and MSB-first in-memory bit reader.  Its tests
are direct managed counterparts of the native Huffman state and decoder
vectors.

Build and run on this machine:

```powershell
& 'C:\Windows\Microsoft.NET\Framework\v2.0.50727\MSBuild.exe' .\Jxr.Managed.sln /p:Configuration=Release
& .\Jxr.Managed.Tests\bin\Release\Jxr.Managed.Tests.exe
```

The native `JxrConformanceTests` remains the reference oracle and must stay
green alongside the managed runner.
