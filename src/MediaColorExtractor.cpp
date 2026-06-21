#include "MediaColorExtractor.h"
#include <iostream>
#include <algorithm>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <robuffer.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

inline uint8_t* GetBufferData(const winrt::Windows::Storage::Streams::IBuffer& buffer) {
    winrt::com_ptr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccess =
        buffer.as<Windows::Storage::Streams::IBufferByteAccess>();
    uint8_t* pixels = nullptr;
    bufferByteAccess->Buffer(&pixels);
    return pixels;
}

MediaColorExtractor::MediaColorExtractor() 
    : m_running(false), m_r(0.8f), m_g(0.2f), m_b(0.9f), m_hasNewColor(false) {
}

MediaColorExtractor::~MediaColorExtractor() {
    Stop();
}

void MediaColorExtractor::Start() {
    if (m_running) return;
    m_running = true;
    m_worker = std::thread(&MediaColorExtractor::WorkerThread, this);
}

void MediaColorExtractor::Stop() {
    m_running = false;
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

bool MediaColorExtractor::GetDominantColor(float& r, float& g, float& b) {
    std::lock_guard<std::mutex> lock(m_colorMutex);
    if (m_hasNewColor) {
        r = m_r;
        g = m_g;
        b = m_b;
        m_hasNewColor = false;
        return true;
    }
    return false;
}

void MediaColorExtractor::WorkerThread() {
    winrt::init_apartment();

    while (m_running) {
        try {
            FetchMediaColor();
        } catch (...) {
            // Ignore temporary WinRT errors
        }
        
        for (int i = 0; i < 20 && m_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void MediaColorExtractor::FetchMediaColor() {
    auto manager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    if (!manager) return;

    auto session = manager.GetCurrentSession();
    if (!session) return;

    auto properties = session.TryGetMediaPropertiesAsync().get();
    if (!properties) return;

    auto thumbnailRef = properties.Thumbnail();
    if (!thumbnailRef) return;

    auto stream = thumbnailRef.OpenReadAsync().get();
    if (!stream) return;

    uint32_t size = static_cast<uint32_t>(stream.Size());
    if (size == 0 || size > 10 * 1024 * 1024) return;

    winrt::Windows::Storage::Streams::Buffer buffer(size);
    auto readResult = stream.ReadAsync(buffer, size, winrt::Windows::Storage::Streams::InputStreamOptions::None).get();

    uint8_t* rawBytes = GetBufferData(readResult);
    if (!rawBytes) return;

    int width, height, channels;
    unsigned char* img = stbi_load_from_memory(rawBytes, readResult.Length(), &width, &height, &channels, 3);
    
    if (img) {
        long long totalR = 0, totalG = 0, totalB = 0;
        int count = 0;
        
        for (int y = 0; y < height; y += 4) {
            for (int x = 0; x < width; x += 4) {
                int index = (y * width + x) * 3;
                unsigned char r = img[index];
                unsigned char g = img[index + 1];
                unsigned char b = img[index + 2];
                
                // Ignore black/white borders or pure dark regions
                if ((r > 30 || g > 30 || b > 30) && (r < 240 || g < 240 || b < 240)) {
                    totalR += r;
                    totalG += g;
                    totalB += b;
                    count++;
                }
            }
        }
        
        stbi_image_free(img);
        
        if (count > 0) {
            float avgR = (totalR / count) / 255.0f;
            float avgG = (totalG / count) / 255.0f;
            float avgB = (totalB / count) / 255.0f;
            
            // Normalize & Boost Saturation for visualizer
            float maxVal = std::max({avgR, avgG, avgB});
            if (maxVal > 0.1f) {
                avgR /= maxVal;
                avgG /= maxVal;
                avgB /= maxVal;
            }
            
            std::lock_guard<std::mutex> lock(m_colorMutex);
            // Only flag as new color if it significantly differs
            if (std::abs(m_r - avgR) > 0.05f || std::abs(m_g - avgG) > 0.05f || std::abs(m_b - avgB) > 0.05f) {
                m_r = avgR;
                m_g = avgG;
                m_b = avgB;
                m_hasNewColor = true;
            }
        }
    }
}
