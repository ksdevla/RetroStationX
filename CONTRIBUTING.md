# Contributing to RetroStationX

Thanks for helping out. This page covers how to report problems and how to send changes.

## Reporting bugs and requesting features

- Search the [existing issues](https://github.com/ksdevla/RetroStationX/issues) first.
- Use the **Bug report** or **Feature request** template when you open an issue.
- For emulation bugs, include the system, the game or test ROM, your RetroStationX version or commit (shown in **Help → About**), and steps to reproduce.
- Report RetroStationX problems here, not to MesenCE or Mesen. If you can reproduce the bug in an official MesenCE build, it is also worth reporting it [upstream](https://github.com/nesdev-org/MesenCE/issues).
- Report security vulnerabilities privately. See [SECURITY.md](SECURITY.md).

## Licensing of contributions

RetroStationX is licensed under the **GNU GPL v3.0 or later**. By submitting a contribution, you agree that it is licensed under the same terms (inbound = outbound).

- Only submit code that you wrote or that has a license compatible with GPL v3. Keep existing copyright and license headers.
- Third-party code needs its license listed in [NOTICE.md](NOTICE.md) and in the About window (`UI/Windows/AboutWindow.axaml.cs`).

### Sending changes upstream

Work from this fork can also be offered to [MesenCE](https://github.com/nesdev-org/MesenCE), but MesenCE has stricter rules. It asks that contributions be **MIT-licensed** and must not be based on GPL code, and it **does not accept AI-generated code**. Read [MesenCE's CONTRIBUTING.md](https://github.com/nesdev-org/MesenCE/blob/master/CONTRIBUTING.md) before you submit there.

## Workflow

1. Fork the repository and create a branch from `master`.
2. Make your change. Keep each pull request focused on one topic.
3. Format your code and make sure it builds without warnings (see below).
4. Open a pull request. Explain what changed and why, and attach test ROMs or screenshots when they help.

For large changes such as new systems, big refactors, or UI redesigns, open an issue first to discuss the approach.

## Building

See [COMPILING.md](COMPILING.md). The Windows (MSVC) and C# builds treat warnings as errors, so fix them before you submit.

## Code style

CI enforces formatting with clang-format (C++) and `dotnet format` (C#). Run both before you commit. Visual Studio can also run them on save.

```bash
find ./ -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
```

```bash
dotnet format
```

Naming conventions:

- `ExampleFunction`
- `exampleVariable`
- `_exampleMemberVariable`

When unsure, follow the style of the surrounding code.

## Guidelines

- **Performance:** Measure the maximum FPS before and after your change. Press F9 to unlock the frame rate and F10 to show the FPS counter. Keep extra branches out of hot paths such as CPU and PPU loops.
- **Compatibility:** Don't break save states, movies, or settings files unless there is no other way. If you must, explain why in the PR.
- **Upstream merges:** This fork regularly merges from MesenCE. Avoid renaming or moving upstream files without a good reason, and keep internal names (`Mesen.*` namespaces, `MesenCore`) unchanged. Put fork branding in `UI/AppInfo.cs`.
- **Scope:** Changes may be declined if they are out of scope or too niche to maintain.

## Merging upstream changes

For maintainers:

```bash
git remote add upstream https://github.com/nesdev-org/MesenCE.git
```

```bash
git fetch upstream && git merge upstream/master
```

After a merge, look for new user-visible "Mesen" strings (UI text, window titles, messages) and change them to RetroStationX or to `AppInfo.Name`.
