#include <GLFW/glfw3.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "AudioCapture.h"
#include "AudioAnalyzer.h"
#include "MediaColorExtractor.h"
#include <vector>
#include <algorithm>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void RenderPSPStyle(const std::vector<float>& frequencySpectrum, float time, float r, float g, float b, float speed, float intensity, float colorShift) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    int baseRibbons = 5; // Amount of strings per layer
    int historySteps = 10; // Number of "ghost" trails

    // 1. Extract raw audio energy
    float rawBass = 0.0f;
    float rawMid = 0.0f;
    float rawHigh = 0.0f;
    
    if (!frequencySpectrum.empty()) {
        int bassCount = std::min(10, (int)frequencySpectrum.size());
        for (int i = 0; i < bassCount; ++i) rawBass += frequencySpectrum[i];
        rawBass /= (float)std::max(1, bassCount);

        int midCount = std::min(30, (int)frequencySpectrum.size());
        for (int i = 10; i < midCount; ++i) rawMid += frequencySpectrum[i];
        rawMid /= (float)std::max(1, midCount - 10);
        
        int highCount = std::min(100, (int)frequencySpectrum.size());
        for (int i = 30; i < highCount; ++i) rawHigh += frequencySpectrum[i];
        rawHigh /= (float)std::max(1, highCount - 30);
    }
    
    // VERY IMPORTANT: Smooth out the energy across frames so it doesn't hurt the eyes
    static float bassPower = 0.0f;
    static float midPower = 0.0f;
    static float highPower = 0.0f;
    
    // 0.08f multiplier makes it blend smoothly between frames without jumping
    bassPower += (rawBass - bassPower) * 0.08f;
    midPower += (rawMid - midPower) * 0.08f;
    highPower += (rawHigh - highPower) * 0.08f;
    
    // 2. Automatic Gain Control (AGC) - Auto-adjusts to the song's volume
    static float sensitivity = 1.0f;
    float currentMax = std::max({bassPower, midPower, highPower});
    if (currentMax > 0.001f) {
        float targetSensitivity = 0.2f / currentMax; // Target an optimal visual amplitude
        targetSensitivity = std::clamp(targetSensitivity, 0.3f, 4.0f); // Less aggressive limits
        sensitivity += (targetSensitivity - sensitivity) * 0.01f; // Slower smooth transition
    } else {
        sensitivity += (1.0f - sensitivity) * 0.01f; // Return to normal if silent
    }
    
    float finalBass = bassPower * sensitivity;
    float finalMid = midPower * sensitivity;
    float finalHigh = highPower * sensitivity;

    finalBass = std::min(finalBass, 0.4f); // Softer cap
    finalMid = std::min(finalMid, 0.4f);
    
    // 3. Dynamic color shift based on frequency
    float dynR = r + finalBass * colorShift;
    float dynG = g + finalHigh * colorShift;
    float dynB = b + finalMid * colorShift;
    
    // Normalize color back to [0, 1] range
    float maxCol = std::max({dynR, dynG, dynB, 1.0f});
    dynR /= maxCol; dynG /= maxCol; dynB /= maxCol;

    // 4. Render 3 Depth Layers
    for (int layer = 0; layer < 3; ++layer) {
        float layerWidth = (layer == 0) ? 1.0f : (layer == 1 ? 2.5f : 1.5f);
        glLineWidth(layerWidth);
        
        int steps = historySteps - layer * 2; 
        
        for (int step = steps - 1; step >= 0; --step) {
            float layerTimeOffset = time * speed * (0.7f + layer * 0.3f); 
            float t = layerTimeOffset - step * 0.05f;
            
            float stepAlpha = std::pow(1.0f - (step / (float)steps), 1.5f);
            if (layer == 0) stepAlpha *= 0.35f; 
            if (layer == 2) stepAlpha *= 0.8f;  
            
            for (int i = 0; i < baseRibbons; ++i) {
                glBegin(GL_LINE_STRIP);
                for (float x = -1.15f; x <= 1.15f; x += 0.02f) {
                    float envelope = std::cos(x * 3.14159265f * 0.45f);
                    if (envelope < 0.0f) envelope = 0.0f;
                    envelope = std::pow(envelope, 1.5f);
                    
                    // Reduced multipliers for smoother movement. Increased base value (0.08f) so it looks good silent.
                    float audioIntensity = 0.08f * intensity + (finalBass * 2.0f * intensity) + (finalMid * 1.5f * intensity);
                    if (layer == 0) audioIntensity *= 0.6f;  
                    if (layer == 2) audioIntensity *= 1.2f;  
                    
                    float layerPhase = layer * 2.1f;
                    float pspWave = 0.0f;
                    
                    pspWave += std::sin(x * (2.5f + layer*0.5f) + t * 2.2f + i * 1.5f + layerPhase) * 1.0f;
                    pspWave += std::sin(x * (4.0f - layer*0.2f) - t * 1.7f + i * 2.1f + layerPhase) * 0.4f;
                    pspWave += std::cos(x * 1.8f + t * 2.8f + i * 0.8f + layerPhase) * 0.4f;
                    
                    // Reduced treble jitter
                    if (layer == 2 && finalHigh > 0.05f) {
                        pspWave += std::sin(x * 15.0f + t * 12.0f) * finalHigh * 0.8f * intensity;
                    }
                    
                    float y = pspWave * audioIntensity * envelope;
                    
                    float parallaxOffset = (layer - 1) * 0.15f;
                    y += parallaxOffset * envelope * 0.5f;
                    
                    float alpha = (envelope * 0.8f + 0.2f) * stepAlpha;
                    if (alpha > 1.0f) alpha = 1.0f;
                    
                    float colorVar = (i % 3) * 0.08f;
                    glColor4f(dynR - colorVar, dynG + colorVar * 0.5f, dynB + colorVar, alpha);
                    
                    glVertex2f(x, y);
                }
                glEnd();
            }
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    int windowWidth = mode->width * 0.8;
    int windowHeight = mode->height * 0.8;

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

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    AudioCapture audioCapture;
    if (!audioCapture.Start()) {
        std::cerr << "Failed to start audio capture" << std::endl;
    }

    AudioAnalyzer audioAnalyzer(1024);

    MediaColorExtractor mediaColorExtractor;
    mediaColorExtractor.Start();

    std::cout << "Successfully created visualizer window." << std::endl;

    std::vector<float> audioSamples;
    std::vector<float> frequencySpectrum;
    float time = 0.0f;

    bool showMenu = true;
    bool wasTabPressed = false;

    // Settings state
    static float bgColor[3] = {0.02f, 0.02f, 0.05f};
    static float ribbonColor[3] = {0.8f, 0.2f, 0.9f}; 
    static float targetRibbonColor[3] = {0.8f, 0.2f, 0.9f}; 
    static bool autoColorFromMedia = false;
    static float waveSpeed = 1.0f;
    static float visualIntensity = 1.0f;
    static float colorShift = 0.5f;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        time += 0.016f; // Approx 60 FPS time step

        // Handle menu toggle via Tab key
        bool isTabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (isTabPressed && !wasTabPressed) {
            showMenu = !showMenu;
        }
        wasTabPressed = isTabPressed;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        audioCapture.GetLatestSamples(audioSamples);
        audioAnalyzer.ComputeSpectrum(audioSamples, frequencySpectrum);

        if (autoColorFromMedia) {
            float mr, mg, mb;
            if (mediaColorExtractor.GetDominantColor(mr, mg, mb)) {
                targetRibbonColor[0] = mr;
                targetRibbonColor[1] = mg;
                targetRibbonColor[2] = mb;
            }
            ribbonColor[0] += (targetRibbonColor[0] - ribbonColor[0]) * 0.05f;
            ribbonColor[1] += (targetRibbonColor[1] - ribbonColor[1]) * 0.05f;
            ribbonColor[2] += (targetRibbonColor[2] - ribbonColor[2]) * 0.05f;
        } else {
            targetRibbonColor[0] = ribbonColor[0];
            targetRibbonColor[1] = ribbonColor[1];
            targetRibbonColor[2] = ribbonColor[2];
        }

        if (showMenu) {
            // ImGui Settings Window
            // We use AlwaysAutoResize to make it compact
            ImGui::Begin(" m0-visualiser setting", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("press tab to hide or show this menu");
            ImGui::Separator();
            
            ImGui::Text("appearance");
            ImGui::ColorEdit3("background ", bgColor);
            if (!autoColorFromMedia) {
                ImGui::ColorEdit3("primary", ribbonColor);
            } else {
                ImGui::Text("Primary Color: (Auto-syncing with media)");
            }
            ImGui::Checkbox("Auto-Color from Spotify/Media", &autoColorFromMedia);
            ImGui::SliderFloat("Color Reactivity", &colorShift, 0.0f, 2.0f, "%.2f");
            ImGui::Separator();
            
            ImGui::Text("Behavior");
            ImGui::SliderFloat("Wave Speed", &waveSpeed, 0.1f, 3.0f, "%.2f");
            ImGui::SliderFloat("Audio Intensity", &visualIntensity, 0.1f, 3.0f, "%.2f");
            ImGui::Separator();
            
            if (!audioSamples.empty()) {
                int displayCount = std::min((int)audioSamples.size(), 1024);
                ImGui::PlotLines("Waveform", audioSamples.data(), displayCount, 0, NULL, -1.0f, 1.0f, ImVec2(0, 50));

                if (!frequencySpectrum.empty()) {
                    int specCount = std::min((int)frequencySpectrum.size(), 256);
                    ImGui::PlotHistogram("Spectrum", frequencySpectrum.data(), specCount, 0, NULL, 0.0f, 0.1f, ImVec2(0, 50));
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Waiting for audio...");
            }
            
            ImGui::End();
        }

        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the beautiful flowing ribbons!
        RenderPSPStyle(frequencySpectrum, time, ribbonColor[0], ribbonColor[1], ribbonColor[2], waveSpeed, visualIntensity, colorShift);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    audioCapture.Stop();
    mediaColorExtractor.Stop();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
