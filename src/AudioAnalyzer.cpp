#include "AudioAnalyzer.h"
#include <cmath>
#include <algorithm>

AudioAnalyzer::AudioAnalyzer(int nfft) : m_nfft(nfft) {
    m_cfg = kiss_fft_alloc(nfft, 0, NULL, NULL);
    m_fftIn.resize(nfft);
    m_fftOut.resize(nfft);
}

AudioAnalyzer::~AudioAnalyzer() {
    free(m_cfg);
}

void AudioAnalyzer::ComputeSpectrum(const std::vector<float>& audioSamples, std::vector<float>& outSpectrum) {
    if (audioSamples.empty()) {
        outSpectrum.clear();
        return;
    }

    // Prepare input buffer (apply a Hann window to reduce spectral leakage)
    int count = std::min((int)audioSamples.size(), m_nfft);
    for (int i = 0; i < m_nfft; ++i) {
        if (i < count) {
            // Hann window function: 0.5 * (1 - cos(2*PI*n/(N-1)))
            float window = 0.5f * (1.0f - cosf(2.0f * 3.1415926535f * i / (m_nfft - 1)));
            m_fftIn[i].r = audioSamples[audioSamples.size() - count + i] * window;
        } else {
            m_fftIn[i].r = 0.0f;
        }
        m_fftIn[i].i = 0.0f;
    }

    // Execute FFT
    kiss_fft(m_cfg, m_fftIn.data(), m_fftOut.data());

    // Calculate magnitudes
    // We only need the first half of the FFT output (up to Nyquist frequency)
    int halfNfft = m_nfft / 2;
    outSpectrum.resize(halfNfft);
    for (int i = 0; i < halfNfft; ++i) {
        float r = m_fftOut[i].r;
        float im = m_fftOut[i].i;
        // Calculate magnitude and normalize
        outSpectrum[i] = sqrtf(r * r + im * im) * 2.0f / m_nfft;
    }
}
