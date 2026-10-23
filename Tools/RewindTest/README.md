# Rewind test

Headless test that checks rewinding never changes the outcome of emulation. It plays a tiny NES test ROM with different controller input on every frame. Then it rewinds in several ways (holding rewind, quick taps, repeated rewinds across earlier resume points, "rewind N seconds") and compares the console's RAM at frame 3000 with a run that never rewound. It also checks that the stored rewind history is consistent frame by frame.

The test itself is the `RunRewindTest` function in `InteropDLL/TestApiWrapper.cpp`.

## Running it (Windows)

1. Build the core (`InteropDLL`) in `Release` / `x64`.
2. From the repository root, generate the test ROM:
   ```bash
   python Tools/RewindTest/make_rom.py build/rewindtest/rewindtest.nes
   ```
3. Run the test:
   ```bash
   python Tools/RewindTest/run_test.py bin/win-x64/Release/MesenCore.dll build/rewindtest/rewindtest.nes build/rewindtest/log.txt build/rewindtest/home
   ```

The script prints one line per scenario (`PASS`/`FAIL`), the emulation speed with and without rewind, and exits with code 0 when everything passes.
