#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

class MediaColorExtractor {
public:
    MediaColorExtractor();
    ~MediaColorExtractor();

    void Start();
    void Stop();

    // Returns true if a new color was extracted
    bool GetDominantColor(float& r, float& g, float& b);

private:
    void WorkerThread();
    void FetchMediaColor();

    std::atomic<bool> m_running;
    std::thread m_worker;
    
    std::mutex m_colorMutex;
    float m_r, m_g, m_b;
    bool m_hasNewColor;
};
