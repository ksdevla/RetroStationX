# Notices and attribution

## Origin

RetroStationX is a modified version of **MesenCE** (Mesen Community Edition, <https://github.com/nesdev-org/MesenCE>). MesenCE is a community continuation of **Mesen** by Sour (<https://github.com/SourMesen/Mesen2>).

- Mesen: Copyright (C) 2014-2026 Sour
- MesenCE: Copyright (C) 2026 MesenCE contributors
- RetroStationX: Copyright (C) 2026 ksdevla

The whole program is distributed under the GNU General Public License, version 3 or (at your option) any later version. See [LICENSE](LICENSE). The commit history from MesenCE is kept in this repository so each change can be traced to its author.

## Modifications (GPLv3 section 5a)

RetroStationX was forked from MesenCE commit `4b8a196` in October 2026. Changes made by this fork:

- **2026-10:** Renamed the application to RetroStationX: product name, window titles, About window, messages, executable and macOS bundle names, Linux desktop entry, and CI artifacts. Replaced the application icon.
- **2026-10:** Gave the fork its own OS identifiers so it can be installed next to Mesen2 or MesenCE without conflicts. This covers the settings folder, the single-instance ID, Windows file-association ProgID, Linux MIME types, icon and `.desktop` names, and the macOS bundle identifier.
- **2026-10:** Turned off the MesenCE auto-updater feed until RetroStationX publishes its own.
- **2026-10:** Rewind saves an uncompressed snapshot every frame (instead of a compressed one every 30 frames). The default memory limit is now 4 GB. Fixed rewind history corruption when resuming exactly on a snapshot, History Viewer input for ports other than player 1, and "Rewind N seconds" on non-60 FPS systems.
- **2026-10:** Changed the default Xbox controller layout for 2-button controllers (A = A, B = X, Turbo A = B, Turbo B = Y), with movement on the left analog stick.
- **2026-10:** Fixed published builds failing to start ("missing dependencies") after a normal build, and removed debug symbol files from published builds.
- **2026-10:** Added frame generation (Windows): in-between frames generated with block-based motion estimation, shown at the monitor's refresh rate or a chosen target frame rate.

For the full list of changes, see the git history after commit `4b8a196`.

## Trademarks and names

"Mesen" and "MesenCE" are the names of the upstream projects. RetroStationX uses them only to credit those projects and to describe its origin. It is not affiliated with or endorsed by Sour, the MesenCE maintainers, or nesdev.org. The GPL does not grant rights to use any project's name or logo (GPLv3 section 7e).

Some internal identifiers still use the upstream name so that MesenCE changes keep merging cleanly: the `Mesen.*` C# namespaces, the `MesenCore` native library, `Mesen.sln`, Lua API names, and file formats such as `.mmo` movies, `.mlb` label files, and `MesenNesDB.txt`. Users never see these as product branding.

The emulated systems' names (NES, SNES, Game Boy, PC Engine, Master System, Game Gear, WonderSwan, and others) are trademarks of their owners. They are used only to describe compatibility.

## Third-party components

RetroStationX contains or links to the following software. Each one keeps its own license.

| Component | License |
|---|---|
| 7-Zip (SevenZip) | Public domain |
| Avalonia, AvaloniaEdit, Avalonia ColorPicker | MIT |
| blip_buf, nes_ntsc, snes_ntsc (blargg) | LGPL 2.1+ |
| CSCD codec (lsnes) | GPL 2+ |
| DataBox, Dock.Avalonia | MIT |
| DynamicData, ReactiveUI, CommunityToolkit.Mvvm | MIT |
| ELFSharp | MIT |
| emu2413 | MIT |
| GBA multiply algorithm (zaydlang) | zlib |
| HarfBuzz | MIT |
| hqx | LGPL 2.1+ |
| kissfft | BSD 3-clause |
| LED icons (led24.de) | CC BY 4.0 |
| libspng | BSD 2-clause |
| librashader | MPL 2.0 |
| Lua | MIT |
| magic_enum | MIT |
| miniz | MIT |
| orfanidis_eq | MIT |
| scale2x | GPL 2+ |
| SDD-1 decompression (Andreas Naive) | Public domain |
| SDL2 | zlib |
| Skia / SkiaSharp | BSD 3-clause / MIT |
| stb_vorbis | Public domain |
| Super Eagle / 2xSaI (Kreed) | GPL 3 |
| xBRZ | GPL 3 |
| ymfm | BSD 3-clause |
| ZMBV codec (DOSBox) | GPL 2+ |

The license and copyright notices inside these components' source files must be kept. For acknowledgements of people and projects whose work helped Mesen (ares, SameBoy, BizHawk, the NesDev community, and others), see **Help → About** in the app.
