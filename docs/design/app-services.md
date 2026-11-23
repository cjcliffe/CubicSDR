# AppServices Seam

`AppServices` (`src/AppServices.h`) is the interface between the portable core (SDR, demodulation, DSP, audio, bookmarks, config, sessions) and the UI shell. Core code reaches shell services only through `GetAppServices()`; the wx application object implements the interface today, and the Raylib port will reimplement it.

## Service Groups

- **Managers and config**: `getConfig()`, `getDemodMgr()`, and `getBookmarkMgr()` expose the shared application objects.
- **Stream state**: frequency, sample rate, solo mode, the audio-visual queue, and the SDR thread handle.
- **Thread notifications**: `sdrThreadNotify()`, `sdrEnumThreadNotify()`, and `notifyMainUIOfDeviceChange()` carry device and enumeration progress from worker threads as plain state updates, which the shell applies directly.
- **Platform services**: `getUserDataDir()`, `getExecutableDir()`, `getModulePath()`, and `showMessage()` isolate path resolution and user notification.
- **View bridges**: main-view window and zoom state, bookmark view refresh and branch expand states, and modem-property and demod-busy queries. These keep view dependencies behind the seam instead of letting core code touch widgets.

## Lifetime

The application object registers itself with `SetAppServices()` during construction, before any worker thread starts. Core code treats the returned reference as always valid.

## Porting Notes

Reimplementing the interface is the bulk of the UI-shell port: every method has exactly one wx-based implementation in `CubicSDR.cpp` today. The interface itself depends only on core domain types.
