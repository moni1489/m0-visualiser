#include <GLFW/glfw3.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "AudioCapture.h"
#include "AudioAnalyzer.h"
#include <vector>
#include <algorithm>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Modern OpenGL settings (optional, but good for future ImGui/Shaders)
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Get primary monitor resolution for a nice large window
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    int windowWidth = mode->width * 0.8;
    int windowHeight = mode->height * 0.8;

    // Create a standard resizable window
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Retro Audio Visualizer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Set callback for window resize
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initial viewport setup
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Start Audio Capture
    AudioCapture audioCapture;
    if (!audioCapture.Start()) {
        std::cerr << "Failed to start audio capture" << std::endl;
    }

    AudioAnalyzer audioAnalyzer(1024);

    std::cout << "Successfully created visualizer window." << std::endl;

    std::vector<float> audioSamples;
    std::vector<float> frequencySpectrum;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui Settings Window
        ImGui::Begin("Retro Visualizer Settings");
        ImGui::Text("Welcome to the Retro Visualizer!");
        static float color[3] = {0.05f, 0.05f, 0.1f};
        ImGui::ColorEdit3("Background Color", color);
        
        // Fetch latest audio samples
        audioCapture.GetLatestSamples(audioSamples);
        
        // Plot the raw waveform and spectrum in ImGui
        if (!audioSamples.empty()) {
            // Raw Waveform
            int displayCount = std::min((int)audioSamples.size(), 1024);
            ImGui::PlotLines("Waveform", audioSamples.data(), displayCount, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));

            // Frequency Spectrum (FFT)
            audioAnalyzer.ComputeSpectrum(audioSamples, frequencySpectrum);
            if (!frequencySpectrum.empty()) {
                // We typically only want to show the lower/mid frequencies, so we don't need to display all 512 bins
                int specCount = std::min((int)frequencySpectrum.size(), 256);
                ImGui::PlotHistogram("Spectrum", frequencySpectrum.data(), specCount, 0, NULL, 0.0f, 0.1f, ImVec2(0, 80));
            }
        } else {
            ImGui::Text("No audio data received yet. Play some music!");
        }
        
        ImGui::End();

        // Clear screen with a retro dark background color
        glClearColor(color[0], color[1], color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // TODO: Render retro graphics here

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Stop audio capture before exiting
    audioCapture.Stop();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
