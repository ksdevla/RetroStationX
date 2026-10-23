# Building RetroStationX

RetroStationX has two parts:

- **Core** (C++17): the emulation cores, built into a native library named `MesenCore` (`.dll`, `.so`, or `.dylib`).
- **UI** (C#, .NET 10, Avalonia): the user interface, built into the `RetroStationX` executable.

## Windows

Requirements: Visual Studio 2026 (C++ toolset v145) with the **Desktop development with C++** and **.NET desktop development** workloads, plus the [.NET 10 SDK](https://dotnet.microsoft.com/en-us/download/dotnet/10.0).

1. Open `Mesen.sln` in Visual Studio.
2. Select the `Release` / `x64` configuration.
3. Set the `UI` project as the startup project and run it.

The executable is written to `bin\win-x64\Release\RetroStationX.exe`.

To create a release build the same way CI does, run these from a Developer Command Prompt:

```bash
msbuild -m -p:Configuration=Release -p:Platform=x64 -t:Clean,UI -p:TargetFramework=net10.0 -p:OptimizeUi=true
```

```bash
dotnet publish --no-restore -c Release -p:PublishAot=true -p:SelfContained=true -p:OptimizeUi=true -p:Platform="Any CPU" -p:TargetFramework=net10.0 -r win-x64 Mesen.sln /p:PublishProfile=UI\Properties\PublishProfiles\Release.pubxml
```

The output goes to `build/TmpReleaseBuild/`. If `dotnet publish` complains about missing assets, run `dotnet restore -r win-x64 -p:PublishAot=true` first.

## Linux

Requirements:

- Clang or GCC with C++17 support
- SDL2 development files (for example `libsdl2-dev` on Debian/Ubuntu)
- The [.NET 10 SDK](https://learn.microsoft.com/en-us/dotnet/core/install/linux)

Build with Clang:

```bash
make -j$(nproc)
```

Build with GCC instead:

```bash
USE_GCC=true make -j$(nproc)
```

The binary is written to `bin/linux-x64/Release/linux-x64/publish/RetroStationX` (or `linux-arm64` on ARM). After a build, `make run` starts it.

**Note:** Builds made with Clang usually run faster than GCC builds.

To build an AppImage, run `Linux/appimage/appimage.sh` (x64) or `Linux/appimage/appimage-arm64.sh` (ARM64) from the repository root.

## macOS

Install SDL2 (for example with `brew install sdl2`) and the [.NET 10 SDK](https://dotnet.microsoft.com/en-us/download/dotnet/10.0), then run:

```bash
make -j$(sysctl -n hw.logicalcpu)
```

The app bundle is written to `bin/osx-<arch>/Release/osx-<arch>/publish/RetroStationX.app`.

## Optional build flags (Linux and macOS)

| Flag | Effect |
|---|---|
| `USE_AOT=true` | Compile the UI with .NET Native AOT (faster startup, no .NET runtime needed) |
| `LTO=true` | Enable link-time optimization |
| `STATICLINK=true` | Statically link the C++ runtime |
| `USE_GCC=true` | Use GCC instead of Clang |

## Optional dependency: librashader

Shader support uses [librashader](https://github.com/SnowflakePowered/librashader). CI downloads a prebuilt copy into `UI/Dependencies/` (see `.github/actions/get-librashader-action`). Local builds work without it, but shaders are unavailable.
