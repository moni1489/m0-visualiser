<div align="center">
  <h1>m0-visualiser
</h1>
  <p>as a psp i think</p>

  

</div>



the project uses CMake and will automatically download dependencies like `GLFW`, `ImGui`, `KissFFT`, and `stb_image`.

```bash
# 1. Clone the repository
git clone https://github.com/YourUsername/RetroAudioVisualizer.git
cd RetroAudioVisualizer

# 2. Generate project files
cmake -S . -B build

# 3. Build the project
cmake --build build --config Release
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
