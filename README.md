# Phase Correlation Tester

A small Qt/QML + C++ diagnostic application for inspecting the *full* phase-correlation surface between two images.

The primary use case is scenes where more than one coherent translation may exist at once, such as a fast nearby parallax layer over a slower distant background. Instead of returning only one translation, the tester keeps the correlation surface and ranks multiple separated peaks.

## What it shows

- Image A and Image B previews.
- Full phase-correlation heatmap.
- Zero-shift marker at the center.
- Numbered candidate peaks.
- `dx`, `dy` for each peak. These are the translations to apply to **Image B to align it to Image A**.
- Raw correlation strength and strength relative to peak #1.
- Candidate match preview with optional custom `dx`/`dy`, continuous `N x N`
  similarity filtering, expanded non-overlap context, and Ctrl+wheel pixel zoom.
- Configurable suppression radius so one broad peak is not mistaken for several independent peaks.
- Optional `±X / ±Y` translation search bounds.
- Optional normalized crop bounds `L/T/R/B`, where `0,0,1,1` uses the full image.
- Optional Hann window.
- Analysis runtime.
- Analysis settings are restored from the previous run.
- Drag-and-drop image loading: one image fills the next empty slot; dropping two
  or more images assigns the first two to Image A and Image B.

The heatmap visualizes positive correlation values after normalizing to the strongest positive response and applying a square-root display curve. The square-root curve is visualization-only; peak ranking uses the original correlation values. This makes weaker secondary motion hypotheses easier to see.

## Video frame sampler helper

`tools/frame_sampler_gui.py` is an independent small Tkinter/OpenCV utility for preparing image pairs or sampled sequences from longer recordings. It accepts a video, start/end times, sampling period, output folder, and PNG/JPEG format.

It seeks once to the requested start point, then decodes forward sequentially and writes only requested sample frames, avoiding repeated random seeks through a long compressed recording. See `tools/FRAME_SAMPLER.md`, or on Windows run `tools/run_frame_sampler.bat` after installing `tools/requirements-frame-sampler.txt`.

## Algorithm

1. Load both images as grayscale `float32`.
2. Apply the optional normalized crop rectangle before correlation.
3. Optionally multiply both by a 2D Hann window.
4. Compute the two 2D DFTs with OpenCV's optimized DFT backend.
5. Form `F1 * conj(F2)`.
6. Divide each complex bin by its magnitude, retaining phase only.
7. Inverse DFT to obtain the phase-correlation surface.
8. Shift zero displacement to the center of the surface.
9. Find the strongest peak in the allowed translation range.
10. Suppress a disk around that peak and repeat to find distinct secondary peaks.
11. Apply a simple 3-point quadratic interpolation independently on X and Y for a subpixel peak estimate.

The phase-correlation logic and multi-peak analysis are implemented in `PhaseCorrelationEngine.cpp`; OpenCV is used as the FFT/DFT backend and for basic matrix/image primitives.

## Build

Requirements:

- CMake 3.21+
- Qt 6.5+ with Quick and Quick Controls 2
- OpenCV with `core` and `imgproc`
- C++20 compiler

On Windows, use a Qt build that matches your compiler (for example, the
`msvc2022_64` Qt kit with Visual Studio 2022). Open a Developer PowerShell for
Visual Studio, then configure from the repository root:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.9.2/msvc2022_64"
cmake --build build --config Release
```

The executable is generated under `build/Release/` with a multi-configuration
generator such as Visual Studio. Qt runtime DLLs may need to be deployed beside
the executable with `windeployqt` before running it outside the Qt development
environment.

Run `run.bat` from the repository root to build and launch the app. The
launcher configures `build/` when needed, builds the `Release` target, and then
starts the executable from `build/Release/` so locally deployed Qt, OpenCV, and
QML files are resolved correctly. If CMake, configure, build, or QML startup
fails, the launcher keeps the error visible; startup diagnostics are written to
`build/Release/phase-correlation-tester.log`.

The repository includes a pinned `vcpkg.json` manifest for native OpenCV. With
vcpkg installed, pass its toolchain when configuring; vcpkg will restore the
declared OpenCV package automatically:

```powershell
cmake -S . -B build `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.9.2/msvc2022_64" `
  -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

The project has no path dependency on another repository. Change the example
Qt and vcpkg locations to match your installation.

## Run the frame sampler

The sampler requires Python 3 with Tkinter and OpenCV. From the repository root:

```powershell
py -m pip install -r tools/requirements-frame-sampler.txt
py tools/frame_sampler_gui.py
```

Alternatively, double-click `tools/run_frame_sampler.bat`. The batch launcher
runs relative to its own directory, so it also works when launched from
Explorer. `tools/run.bat` is an equivalent short launcher for the sampler. See
`tools/FRAME_SAMPLER.md` for input formats and extraction details.

## Reading a two-layer result

If a near layer and a far layer move by different amounts, a useful result may look like:

- peak #1: `dx=-18`, `dy=0`, relative `100%`
- peak #2: `dx=-5`, `dy=0`, relative `61%`

That is evidence for two coherent displacement hypotheses, not proof by itself that they correspond to two depth layers. Repeated texture, wrap-around, occlusion boundaries, and strong edges can also create secondary peaks.

Change **Suppress radius** when peak #2 is merely a neighboring sample of the same broad peak. For real near/far separation, the second peak should remain spatially distinct over a reasonable range of suppression radii.

## Current constraints

- Input images must have the same dimensions.
- Crop bounds are normalized image edges from `0.0` to `1.0`; `L` must be less
  than `R`, and `T` must be less than `B`. The cropped Image A and Image B
  regions must still have the same dimensions.
- Odd image dimensions are cropped by one pixel on the right/bottom for the centered FFT visualization.
- Translation is circular in the Fourier model, so very large shifts near image boundaries should be interpreted carefully.
- This version performs global phase correlation. A later useful extension is tiled/local phase correlation to map which image regions vote for each displacement.
