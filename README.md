<p align="center">
  <img src="UI/Assets/AppIcon.png" width="128" height="128" alt="RetroStationX icon">
</p>

# RetroStationX

RetroStationX is a multi-system emulator and debugger for Windows, Linux, and macOS. It supports NES, SNES, Game Boy (GB/SGB/GBC), Game Boy Advance, PC Engine, SMS/Game Gear, and WonderSwan (WS/WSC).

RetroStationX is a fork of [MesenCE](https://github.com/nesdev-org/MesenCE) (Mesen Community Edition), which continues [Mesen](https://github.com/SourMesen/Mesen2) by Sour. See [NOTICE.md](NOTICE.md) for attribution and what this fork changes.

> RetroStationX is an independent project. It is not affiliated with or endorsed by Sour, the MesenCE team, or nesdev.org. Please report RetroStationX problems [here](https://github.com/ksdevla/RetroStationX/issues), not upstream.

## Downloads

### Releases

Stable builds will be published on the [releases page](https://github.com/ksdevla/RetroStationX/releases).

### Development builds

[![Build](https://github.com/ksdevla/RetroStationX/actions/workflows/build.yml/badge.svg)](https://github.com/ksdevla/RetroStationX/actions/workflows/build.yml?query=branch%3Amaster)

Built automatically from the `master` branch:

* [Windows](https://nightly.link/ksdevla/RetroStationX/workflows/build/master/RetroStationX%20%28Windows%20-%20net10.0%20-%20AoT%29.zip)
  * Windows 7 SP1 or higher, with all updates installed.
* [Linux x64](https://nightly.link/ksdevla/RetroStationX/workflows/build/master/RetroStationX%20%28Linux%20-%20ubuntu-22.04%20-%20clang_aot%29.zip) (requires **SDL2**)
* [Linux ARM64](https://nightly.link/ksdevla/RetroStationX/workflows/build/master/RetroStationX%20%28Linux%20-%20ubuntu-22.04-arm%20-%20clang_aot%29.zip) (requires **SDL2**)
* [Linux x64 AppImage](https://nightly.link/ksdevla/RetroStationX/workflows/build/master/RetroStationX%20%28Linux%20x64%20-%20AppImage%29.zip)
* [macOS, Intel](https://nightly.link/ksdevla/RetroStationX/workflows/build/master/RetroStationX%20%28macOS%20-%20macos-15-intel%20-%20clang_aot%29.zip) (requires **SDL2**)
* [macOS, Apple Silicon](https://nightly.link/ksdevla/RetroStationX/workflows/build/master/RetroStationX%20%28macOS%20-%20macos-15%20-%20clang_aot%29.zip) (requires **SDL2**)

Other build variants are listed in the [Actions](https://github.com/ksdevla/RetroStationX/actions/workflows/build.yml?query=branch%3Amaster) tab.

## Installation

RetroStationX does not need an installer. Extract the archive anywhere and run it.

### Windows

1. Extract the zip and run `RetroStationX.exe`.
2. On first launch, choose where to keep your data (see [Where your data is stored](#where-your-data-is-stored)).

### Linux

1. Install SDL2, for example `sudo apt install libsdl2-2.0-0` (Debian/Ubuntu) or `sudo pacman -S sdl2` (Arch).
2. Extract the zip, then make the binary executable and run it:
   ```bash
   chmod +x RetroStationX && ./RetroStationX
   ```
   For the AppImage, mark `RetroStationX.AppImage` as executable and run it.
3. **SteamOS / Steam Deck:** see [SteamOS.md](SteamOS.md).

### macOS

1. Install SDL2, for example with Homebrew: `brew install sdl2`.
2. Extract the zip and move `RetroStationX.app` to `Applications`.
3. Builds are not notarized by Apple. The first time, right-click the app and choose **Open**, or allow it under **System Settings → Privacy & Security**.

### Where your data is stored

| Mode | Location |
|---|---|
| Default (Windows) | `Documents\RetroStationX` |
| Default (Linux, macOS) | `~/.config/RetroStationX` |
| Portable | The folder that contains the executable (used when a `settings.json` file is next to it) |

RetroStationX keeps its own data folder and never reads or writes the Mesen2 or MesenCE folders, so it can be installed next to them. To bring your existing setup over, close both emulators and copy `settings.json` and any data folders you want (such as `Saves`, `SaveStates`, `Screenshots`, `Movies`, `HdPacks`) from `Documents/Mesen2` or `Documents/MesenCE` into the RetroStationX folder.

### Updates

The built-in updater is turned off until RetroStationX publishes its own update feed. Check the [releases page](https://github.com/ksdevla/RetroStationX/releases) for new versions.

## Building from source

See [COMPILING.md](COMPILING.md).

## Contributing

Bug reports, ideas, and pull requests are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

RetroStationX is free software under the [GNU General Public License v3.0 or later](LICENSE).

Copyright (C) 2014-2026 Sour<br>
Copyright (C) 2026 MesenCE contributors<br>
Copyright (C) 2026 ksdevla

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

Third-party components keep their own licenses. They are listed in [NOTICE.md](NOTICE.md) and in the app under **Help → About**.
