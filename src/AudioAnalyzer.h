#pragma once
#include <vector>
#include "kiss_fft.h"

class AudioAnalyzer {
public:
    // nfft should be a power of 2, e.g., 512, 1024, 2048
    AudioAnalyzer(int nfft = 1024);
    ~AudioAnalyzer();

    // Takes raw audio samples (mono, float) and computes the frequency spectrum.
    // The returned vector contains the magnitude of each frequency bin.
    void ComputeSpectrum(const std::vector<float>& audioSamples, std::vector<float>& outSpectrum);

private:
    int m_nfft;
    kiss_fft_cfg m_cfg;
    std::vector<kiss_fft_cpx> m_fftIn;
    std::vector<kiss_fft_cpx> m_fftOut;
};
