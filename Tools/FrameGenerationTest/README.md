# Frame generation tests

## Interpolation test (headless)

`RunFrameInterpolatorTest` in `InteropDLL/TestApiWrapper.cpp` feeds synthetic pixel-art frames with known motion to `FrameInterpolator`. It covers scrolling, moving and flickering sprites, a static status bar and a static screen, and compares the generated in-between frame with the expected image pixel by pixel. It also reports the CPU cost.

Run it after building the core (`InteropDLL`, `Release` / `x64`), from the repository root:

```bash
python -c "import ctypes,os; d=os.path.abspath('bin/win-x64/Release'); os.add_dll_directory(d); c=ctypes.CDLL(os.path.join(d,'MesenCore.dll')); c.RunFrameInterpolatorTest(os.path.abspath('build/fi_log.txt').encode()); print(open('build/fi_log.txt').read())"
```

## Live check (Windows)

Checks that generated frames actually reach the screen.

1. Build the scrolling test ROM. It scrolls exactly 2 pixels per frame:
   ```bash
   python Tools/FrameGenerationTest/make_scroll_rom.py build/fgtest/scrolltest.nes
   ```
2. Run RetroStationX with frame generation enabled and load `scrolltest.nes`.
3. While it runs, capture the screen:
   ```bash
   python Tools/FrameGenerationTest/check_capture.py scrolltest 300
   ```

Without frame generation, only even scroll positions appear. With frame generation, odd (in-between) positions appear too. Captures that don't match are expected, because the test pattern repeats less often than the script assumes.
