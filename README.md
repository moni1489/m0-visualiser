<div align="center">
  <h1>m0-visualiser
</h1>
  <p>as a psp i think</p>

  

</div>



the project uses CMake and will automatically download dependencies like `GLFW`, `ImGui`, `KissFFT`, and `stb_image`.

```bash
# 1. Open "Developer PowerShell for VS 2022" from Start Menu

# 2. Clone
git clone https://github.com/moni1489/m0-visualiser.git
cd m0-visualiser

# 3. Build
cmake -S . -B build
cmake --build build --config Release

# 4. Run
.\build\Release\m0-visualiser.exe
```

The compiled `.exe` will be located in the `build/Release` (или `build/Debug`) folder.

## how to use this?

1. launch the application.
2. play any music on your PC (Spotify, YouTube, local files).
3. press **`TAB`** to open the settings menu.
4. **Auto-Color from Spotify/Media** is enabled by default. if your media player broadcasts its cover art to Windows, the visualizer will sync its colors automatically!
5. adjust **Wave Speed**, **Color Reactivity**, and **Intensity** to your liking.

## i used

* [GLFW](https://www.glfw.org/) - Window creation and OpenGL context
* [Dear ImGui](https://github.com/ocornut/imgui) - Bloat-free graphical user interface
* [KissFFT](https://github.com/mborgerding/kissfft) - Fast Fourier Transform for audio spectrum analysis
* [stb_image](https://github.com/nothings/stb) - Image decoding for cover art color extraction
* **C++/WinRT** & **WASAPI** - Deep Windows integration
