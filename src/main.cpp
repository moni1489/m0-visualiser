#include <GLFW/glfw3.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "AudioCapture.h"
#include "AudioAnalyzer.h"
#include <vector>
#include <algorithm>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void RenderRetroRibbons(const std::vector<float>& frequencySpectrum, float time, float r, float g, float b) {
    if (frequencySpectrum.empty()) return;

    glLineWidth(2.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Enable line smoothing for better looking ribbons
    glEnable(GL_LINE_SMOOTH);

    int numRibbons = 5;
    for (int i = 0; i < numRibbons; ++i) {
        float yOffset = (i - (numRibbons - 1) / 2.0f) * 0.3f; // Spread out ribbons vertically
        
        glBegin(GL_LINE_STRIP);
        for (float x = -1.05f; x <= 1.05f; x += 0.01f) {
            // Map x from [-1, 1] to an index in the frequency spectrum (0 to spectrum size)
            float normX = (x + 1.0f) * 0.5f;
            if (normX < 0.0f) normX = 0.0f;
            if (normX > 1.0f) normX = 1.0f;
            
            // We use the lower frequencies mostly for these visualizers
            int freqIndex = (int)(normX * (frequencySpectrum.size() / 4)); 
            if (freqIndex >= frequencySpectrum.size()) freqIndex = frequencySpectrum.size() - 1;
            
            float freqValue = frequencySpectrum[freqIndex];
            
            // Create a wave effect combining sine waves and audio frequency
            float wave1 = std::sin(x * 5.0f + time * 3.0f + i * 1.5f) * 0.1f;
            float wave2 = std::cos(x * 8.0f - time * 2.0f + i) * 0.05f;
            
            float pulse = freqValue * 4.0f; // Amplify the audio response
            
            // Calculate final y position
            float y = yOffset + (wave1 + wave2) * (1.0f + pulse) + (pulse * std::sin(time * 5.0f + x * 10.0f));
            
            // Fade out edges smoothly
            float alpha = 1.0f - std::pow(std::abs(x), 2.0f);
            if (alpha < 0.0f) alpha = 0.0f;
            
            // Make the ribbons slightly different shades based on index
            float intensity = 1.0f - (i * 0.1f);
            glColor4f(r * intensity, g * intensity, b, alpha);
            glVertex2f(x, y);
        }
        glEnd();
    }
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
    float time = 0.0f;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        time += 0.016f; // Approx 60 FPS time step

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui Settings Window
        ImGui::Begin("Retro Visualizer Settings");
        ImGui::Text("Welcome to the Retro Visualizer!");
        static float bgColor[3] = {0.02f, 0.02f, 0.05f};
        static float ribbonColor[3] = {0.0f, 0.8f, 1.0f};
        ImGui::ColorEdit3("Background Color", bgColor);
        ImGui::ColorEdit3("Ribbon Color", ribbonColor);
        
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
        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the beautiful flowing ribbons!
        RenderRetroRibbons(frequencySpectrum, time, ribbonColor[0], ribbonColor[1], ribbonColor[2]);

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
