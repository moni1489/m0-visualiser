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

void RenderPSPStyle(const std::vector<float>& frequencySpectrum, float time, float r, float g, float b) {
    glLineWidth(2.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    int baseRibbons = 4; // Main strings
    int historySteps = 12; // "Delay" ghost strings

    // Calculate global bass and mid power (smoothed)
    float bassPower = 0.0f;
    float midPower = 0.0f;
    if (!frequencySpectrum.empty()) {
        int bassCount = std::min(10, (int)frequencySpectrum.size());
        for (int i = 0; i < bassCount; ++i) bassPower += frequencySpectrum[i];
        bassPower /= (float)bassCount;

        int midCount = std::min(30, (int)frequencySpectrum.size());
        for (int i = 10; i < midCount; ++i) midPower += frequencySpectrum[i];
        midPower /= (float)std::max(1, midCount - 10);
    }
    
    // Soft cap power to ensure it never blows up off screen, but allow more energy
    if (bassPower > 0.4f) bassPower = 0.4f;
    if (midPower > 0.4f) midPower = 0.4f;

    // Draw from oldest (most transparent) to newest
    for (int step = historySteps - 1; step >= 0; --step) {
        float t = time - step * 0.05f; // Time delay
        float stepAlpha = std::pow(1.0f - (step / (float)historySteps), 1.5f); // Fade out history
        
        for (int i = 0; i < baseRibbons; ++i) {
            glBegin(GL_LINE_STRIP);
            // Increase step size slightly for smoother performance
            for (float x = -1.05f; x <= 1.05f; x += 0.02f) {
                // Envelope: pinched edges, natural smooth center
                float envelope = std::cos(x * 3.14159265f * 0.5f);
                if (envelope < 0.0f) envelope = 0.0f;
                envelope = std::pow(envelope, 1.5f); // Soft, natural shape without extreme spikes
                
                // Base idle intensity + global music reaction
                // DOUBLED the strength from the last iteration for a good middle ground
                float audioIntensity = 0.04f + (bassPower * 3.5f) + (midPower * 2.0f); 
                
                // Create strings (pure smooth sine waves)
                float pspWave = 0.0f;
                pspWave += std::sin(x * 3.5f + t * 2.5f + i * 1.5f) * 1.0f;
                pspWave += std::sin(x * 6.0f - t * 1.8f + i * 2.1f) * 0.4f;
                pspWave += std::cos(x * 2.0f + t * 3.2f + i * 0.8f) * 0.4f; // Slower deep wobble
                
                // Final Y position
                float y = pspWave * audioIntensity * envelope;
                
                // Color and alpha
                float alpha = (envelope * 0.6f + 0.4f) * stepAlpha; 
                if (alpha > 1.0f) alpha = 1.0f;
                
                float colorVar = (i % 3) * 0.05f;
                glColor4f(r - colorVar, g + colorVar, b, alpha);
                
                glVertex2f(x, y);
            }
            glEnd();
        }
    }
}

void RenderXboxStyle(const std::vector<float>& frequencySpectrum, float time, float r, float g, float b) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Glow effect
    glEnable(GL_LINE_SMOOTH);

    // 1. Draw glowing inner core
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, 0.7f); // Bright center
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= 60; ++i) {
        float angle = (i / 60.0f) * 3.14159265f * 2.0f;
        float pulse = 0.0f;
        if (!frequencySpectrum.empty()) {
            pulse = frequencySpectrum[0] * 0.3f; // Bass causes core to throb
        }
        float radius = 0.15f + pulse + std::sin(time * 3.0f) * 0.01f;
        glColor4f(r, g, b, 0.0f); // Fade out edge
        glVertex2f(radius * std::cos(angle), radius * std::sin(angle));
    }
    glEnd();

    // 2. Draw sharp oscilloscope rings around it
    int segments = 256;
    glLineWidth(2.5f);
    for (int ring = 0; ring < 2; ++ring) {
        float baseRadius = 0.25f + ring * 0.1f;
        
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float angle = (i / (float)segments) * 3.14159265f * 2.0f;
            
            float spike = 0.0f;
            if (!frequencySpectrum.empty()) {
                // Symmetrical spikes mirroring left/right
                int mirrorIndex = segments / 2 - std::abs(i - segments / 2);
                int freqIndex = (int)((mirrorIndex / (float)(segments/2)) * 64);
                if (freqIndex < frequencySpectrum.size()) {
                    // Make spikes very sharp
                    spike = std::pow(frequencySpectrum[freqIndex], 1.2f) * (3.0f + ring * 2.0f);
                }
            }
            
            float radius = baseRadius + spike;
            // Rotate the rings in opposite directions
            float rotAngle = angle + time * (ring == 0 ? 0.3f : -0.2f);
            
            glColor4f(r, g, b, 0.9f - ring * 0.3f);
            glVertex2f(radius * std::cos(rotAngle), radius * std::sin(rotAngle));
        }
        glEnd();
    }
}

void RenderClassicBars(const std::vector<float>& frequencySpectrum, float time, float r, float g, float b) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if (frequencySpectrum.empty()) return;

    int numBars = 64;
    float barWidth = 2.0f / numBars * 0.8f;
    float spacing = 2.0f / numBars * 0.2f;

    glBegin(GL_QUADS);
    for (int i = 0; i < numBars; ++i) {
        float x = -0.9f + i * (barWidth + spacing); // Centered a bit better
        
        int freqIndex = (int)((i / (float)numBars) * (frequencySpectrum.size() / 3.0f));
        float height = frequencySpectrum[freqIndex] * 6.0f; // Scale height
        if (height < 0.01f) height = 0.01f; // Minimum height

        float alpha = 0.9f;
        
        // Bottom left
        glColor4f(r * 0.1f, g * 0.1f, b * 0.1f, alpha);
        glVertex2f(x, -0.6f);
        
        // Bottom right
        glVertex2f(x + barWidth, -0.6f);
        
        // Top right
        glColor4f(r, g, b, alpha);
        glVertex2f(x + barWidth, -0.6f + height);
        
        // Top left
        glVertex2f(x, -0.6f + height);
    }
    glEnd();
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

    // Visualizer settings state
    int currentMode = 0;
    const char* modes[] = { "PSP Strings (Breakcore)", "Xbox Classic (Orb)", "Retro Spectrum Bars" };

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
        
        ImGui::Combo("Visualizer Mode", &currentMode, modes, IM_ARRAYSIZE(modes));
        
        static float bgColor[3] = {0.02f, 0.02f, 0.05f};
        static float ribbonColor[3] = {0.8f, 0.2f, 0.9f}; // Default pink/purple
        
        // Auto-change color to green if switching to Xbox mode for the first time
        static int lastMode = 0;
        if (currentMode == 1 && lastMode != 1) {
            ribbonColor[0] = 0.1f; ribbonColor[1] = 0.9f; ribbonColor[2] = 0.2f; // Xbox Green
        }
        lastMode = currentMode;

        ImGui::ColorEdit3("Background Color", bgColor);
        ImGui::ColorEdit3("Primary Color", ribbonColor);
        
        // Fetch latest audio samples
        audioCapture.GetLatestSamples(audioSamples);
        
        // Always compute spectrum to clear or update it
        audioAnalyzer.ComputeSpectrum(audioSamples, frequencySpectrum);
        
        // Plot the raw waveform and spectrum in ImGui
        if (!audioSamples.empty()) {
            // Raw Waveform
            int displayCount = std::min((int)audioSamples.size(), 1024);
            ImGui::PlotLines("Waveform", audioSamples.data(), displayCount, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));

            // Frequency Spectrum (FFT)
            if (!frequencySpectrum.empty()) {
                // We typically only want to show the lower/mid frequencies
                int specCount = std::min((int)frequencySpectrum.size(), 256);
                ImGui::PlotHistogram("Spectrum", frequencySpectrum.data(), specCount, 0, NULL, 0.0f, 0.1f, ImVec2(0, 80));
            }
        } else {
            ImGui::Text("Waiting for audio... (idle waves playing)");
        }
        
        ImGui::End();

        // Clear screen with a retro dark background color
        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the beautiful flowing ribbons based on mode!
        if (currentMode == 0) {
            RenderPSPStyle(frequencySpectrum, time, ribbonColor[0], ribbonColor[1], ribbonColor[2]);
        } else if (currentMode == 1) {
            RenderXboxStyle(frequencySpectrum, time, ribbonColor[0], ribbonColor[1], ribbonColor[2]);
        } else if (currentMode == 2) {
            RenderClassicBars(frequencySpectrum, time, ribbonColor[0], ribbonColor[1], ribbonColor[2]);
        }

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
