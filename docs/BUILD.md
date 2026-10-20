# CubicSDR Build Guide

Checkout-and-build with a single script. It builds third-party
dependencies from source into `build/deps`, then configures and builds the
application with the matching preset from `CMakePresets.json`.

## Quick start

```sh
python build.py            # full default build for the current platform
python build.py --dry-run  # preview every step without running anything
```

The application lands under `build/` in a platform directory
(`windows/x64`, `linux`, `macos`), with runtime DLLs and SoapySDR modules
staged alongside it on Windows.

## Prerequisites

Install these before the first run; everything else is built from source.

- All platforms: Python 3, CMake, git.
- Windows: Visual Studio 2022 with the C++ workload. NSIS is needed only
  for `--package nsis` and is picked up from its default install location.
- Linux: `cmake`, `git`, `make`, plus the development packages (the
  CI config lists the exact set: build tools, pulseaudio/GTK/OpenGL
  headers, hamlib, wxWidgets, and SoapySDR). Install them yourself
  with apt; the script never installs system packages. Pass
  `--build-deps` to build wxWidgets, liquid-dsp, and SoapySDR from
  source instead (liquid-dsp is always built from source, since it is
  not packaged).
- macOS: the script installs its dependencies itself via Homebrew.

## Useful options

- `--modules`: `default`, `all`, `none`, or a comma-separated list such as
  `--modules rtlsdr,airspy`. `python build.py --help` lists every flag.
  On Linux the selected modules build from the same sources; the
  AppImage bundles them into exe-relative `modules/`.
- Default modules cover the common devices. Two are left out:
  `netsdr` (hangs device enumeration) and `uhd` (rare hardware and a very
  long build). Enable either explicitly, e.g. `--modules uhd` or
  `--modules all`.
- `sdrplay` is in the default set but needs the vendor SDRplay API,
  which cannot be redistributed: download it manually from SDRplay and
  accept their license terms. Without it the module is skipped with a
  message and the rest of the build continues.
- `--package`: `deb` (Linux), `nsis` (Windows), `dmg` (macOS),
  `appimage` (Linux). The deb is the application only and pairs with
  distro SoapySDR module packages; the AppImage bundles the default
  modules instead.
- `--skip-deps`: reuse a previous `build/deps` without rebuilding it.
- `--build-deps` (Linux): build all dependencies from source instead of
  using system packages; the slow, fully controlled variant.
- `--skip-app` (Windows): rebuild modules and restage them without
  reconfiguring the application; the fast loop for module work.
- `--clean`: remove the `build/` tree. `--deps-prefix`: relocate
  `build/deps`. `--arch x64|Win32`, `--config Release|Debug`,
  `--jobs N` behave as expected.

Dependency builds remember their configure flags: editing a flag in
`build.py` rebuilds that dependency once (`flags changed; rebuilding`)
instead of silently reusing stale output.

## Verifying a module build

`SoapySDRUtil` bakes the dependency prefix in as its module search root
at build time (`SOAPY_SDR_ROOT` overrides it), so the prefix install can
be probed directly. On Windows the module DLLs also need their
dependency DLLs visible, so put the staged exe dir and the prefix `bin`
on `PATH` first:

```powershell
$env:PATH = '<checkout>\build\windows\x64;<checkout>\build\deps\bin;' + $env:PATH
SoapySDRUtil --info
SoapySDRUtil --find='driver=<name>'
```

Check the factory appears in `--info`, then confirm enumeration returns
promptly with `--find`. To probe the exe-relative `modules/` folder (the
layout CubicSDR itself uses) instead of the prefix, set `SOAPY_SDR_ROOT`
to the staged dir.

`--find` reporting no devices (exit 1) is normal with no hardware
attached; hanging or crashing is not.

## Troubleshooting

- On Windows, close CubicSDR and any `SoapySDRUtil` processes before
  rebuilding or restaging; open DLLs cause file-lock failures.
- If device enumeration stalls at startup, suspect a freshly added
  module first: move its DLL out of `modules/` and retry. (`netsdr` is
  off by default because it hangs enumeration this way.)
- CMake presets (`windows`, `linux`, `macos`, plus arch and packaging
  variants; see `cmake --list-presets`) hold the configure flags, so a
  manual configure outside the script should use the preset for the
  matching platform.
