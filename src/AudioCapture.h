#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool Start();
    void Stop();

    // Retrieves the latest audio samples (downmixed to mono)
    // Returns the number of samples copied
    int GetLatestSamples(std::vector<float>& outSamples);

private:
    void CaptureThread();

    std::atomic<bool> m_running;
    std::thread m_captureThread;

    std::mutex m_mutex;
    std::vector<float> m_sampleBuffer;
    size_t m_maxBufferSize;
};
