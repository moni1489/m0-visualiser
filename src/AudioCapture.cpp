#include "AudioCapture.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <iostream>

// Releaser helper for COM objects
template <class T>
void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

// Reference constants
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);

AudioCapture::AudioCapture() : m_running(false), m_maxBufferSize(4096) {
    m_sampleBuffer.reserve(m_maxBufferSize);
}

AudioCapture::~AudioCapture() {
    Stop();
}

bool AudioCapture::Start() {
    if (m_running) return true;
    m_running = true;
    m_captureThread = std::thread(&AudioCapture::CaptureThread, this);
    return true;
}

void AudioCapture::Stop() {
    m_running = false;
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
}

int AudioCapture::GetLatestSamples(std::vector<float>& outSamples) {
    std::lock_guard<std::mutex> lock(m_mutex);
    outSamples = m_sampleBuffer;
    return outSamples.size();
}

void AudioCapture::CaptureThread() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return;

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;
    WAVEFORMATEX* pwfx = nullptr;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    if (FAILED(hr)) goto Exit;

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) goto Exit;

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) goto Exit;

    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) goto Exit;

    // We specifically want to capture system output (Loopback)
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, pwfx, NULL);
    if (FAILED(hr)) goto Exit;

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    if (FAILED(hr)) goto Exit;

    hr = pAudioClient->Start();
    if (FAILED(hr)) goto Exit;

    while (m_running) {
        Sleep(10); // Capture chunk every 10ms

        UINT32 packetLength = 0;
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        
        while (packetLength != 0) {
            BYTE* pData;
            UINT32 numFramesAvailable;
            DWORD flags;
            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
            if (FAILED(hr)) break;

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                pData = NULL; 
            }

            // Downmix and convert to float buffer
            if (pData != NULL) {
                int channels = pwfx->nChannels;
                int bytesPerSample = pwfx->wBitsPerSample / 8;
                int frames = numFramesAvailable;

                std::vector<float> tempBuffer;
                tempBuffer.reserve(frames);

                for (int i = 0; i < frames; ++i) {
                    float sum = 0;
                    for (int c = 0; c < channels; ++c) {
                        float sample = 0;
                        if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
                           (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && 
                            reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
                            sample = reinterpret_cast<float*>(pData)[i * channels + c];
                        } else if (bytesPerSample == 2) {
                            short s = reinterpret_cast<short*>(pData)[i * channels + c];
                            sample = s / 32768.0f;
                        }
                        sum += sample;
                    }
                    tempBuffer.push_back(sum / channels);
                }

                // Push to our shared buffer safely
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_sampleBuffer.insert(m_sampleBuffer.end(), tempBuffer.begin(), tempBuffer.end());
                    
                    // Keep buffer size bounded to m_maxBufferSize
                    if (m_sampleBuffer.size() > m_maxBufferSize) {
                        size_t excess = m_sampleBuffer.size() - m_maxBufferSize;
                        m_sampleBuffer.erase(m_sampleBuffer.begin(), m_sampleBuffer.begin() + excess);
                    }
                }
            }

            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            hr = pCaptureClient->GetNextPacketSize(&packetLength);
        }
    }

    pAudioClient->Stop();

Exit:
    if (pwfx) CoTaskMemFree(pwfx);
    SafeRelease(&pCaptureClient);
    SafeRelease(&pAudioClient);
    SafeRelease(&pDevice);
    SafeRelease(&pEnumerator);
    CoUninitialize();
}
