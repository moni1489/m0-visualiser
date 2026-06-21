<div align="center">
  <h1>🌊 Retro Audio Visualizer</h1>
  <p>A beautiful, highly-responsive, PSP-style audio visualizer written in C++ and OpenGL.</p>

  <!-- Замените эту ссылку на вашу реальную гифку или скриншот -->
  <img src="https://via.placeholder.com/800x400?text=Your+Awesome+GIF+Here" alt="Visualizer Demo" width="800"/>
</div>

## ✨ Features

* **🎙️ System-Wide Audio Capture**: Reacts to any sound playing on your Windows machine (WASAPI Loopback).
* **🎨 Spotify / Media Auto-Color Sync**: Automatically extracts the cover art of the currently playing song (via Windows Media API) and smoothly transitions the visualizer waves to the dominant color of the album!
* **🌊 PSP-Style 3D Waves**: Multi-layered parallax ribbons with fluid, eye-pleasing physics and post-smoothing.
* **🎛️ Automatic Gain Control (AGC)**: Automatically balances its sensitivity whether you are listening to quiet classical music or loud bass-heavy tracks.
* **⚙️ Minimalist Overlay Menu**: Built with ImGui. Press `TAB` to toggle the settings menu on the fly.

## 🚀 Getting Started

### Prerequisites

* **Windows 10/11** (Required for WASAPI audio capture and Windows Media WinRT features)
* **CMake** (v3.14+)
* **Visual Studio 2019/2022** (with Desktop development with C++ workload)

### Building the Project

The project uses CMake and will automatically download dependencies like `GLFW`, `ImGui`, `KissFFT`, and `stb_image`.

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

## 🎮 How to Use

1. Launch the application.
2. Play any music on your PC (Spotify, YouTube, local files).
3. Press **`TAB`** to open the settings menu.
4. **Auto-Color from Spotify/Media** is enabled by default. If your media player broadcasts its cover art to Windows, the visualizer will sync its colors automatically!
5. Adjust **Wave Speed**, **Color Reactivity**, and **Intensity** to your liking.

## 🛠️ Built With

* [GLFW](https://www.glfw.org/) - Window creation and OpenGL context
* [Dear ImGui](https://github.com/ocornut/imgui) - Bloat-free graphical user interface
* [KissFFT](https://github.com/mborgerding/kissfft) - Fast Fourier Transform for audio spectrum analysis
* [stb_image](https://github.com/nothings/stb) - Image decoding for cover art color extraction
* **C++/WinRT** & **WASAPI** - Deep Windows integration

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
