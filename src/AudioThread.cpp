#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>

//wxDEFINE_EVENT(wxEVT_COMMAND_AudioThread_INPUT, wxThreadEvent);

AudioThread::AudioThread(AudioThreadQueue* pQueue, int id) :
        wxThread(wxTHREAD_DETACHED), m_pQueue(pQueue), m_ID(id) {

}
AudioThread::~AudioThread() {
    PaError err;
    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);
    Pa_Terminate();
}


static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData) {

    AudioThread *src = (AudioThread *) userData;

    float *out = (float*) outputBuffer;

    if (!src->audio_queue.size()) {
        for (int i = 0; i < framesPerBuffer * 2; i++) {
            out[i] = 0;
        }
        return paContinue;
    }

    std::vector<float> *nextBuffer = src->audio_queue.front();

    for (int i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = (*nextBuffer)[src->audio_queue_ptr];

        src->audio_queue_ptr++;

        if (src->audio_queue_ptr == nextBuffer->size()) {
            src->audio_queue.pop();
            delete nextBuffer;
            src->audio_queue_ptr = 0;
            if (!src->audio_queue.size()) {
                break;
            }
            nextBuffer = src->audio_queue.front();
        }
    }

    return paContinue;
}


wxThread::ExitCode AudioThread::Entry() {

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "Error starting :(\n";
        return (wxThread::ExitCode) 1;
    }

    int preferred_device = -1;

#ifdef WIN32
    wchar_t dev_str[255];
    memset(dev_str, 0, sizeof(wchar_t) * 255);
    std::wstring env_name(L"PA_RECOMMENDED_OUTPUT_DEVICE");
    GetEnvironmentVariable(env_name.c_str(), dev_str, 255);
    std::wstring env_result(dev_str);

    int env_dev = _wtoi(env_result.c_str());

    if (env_dev || env_result.length()) {
        std::cout << "Using preferred PortAudio device PA_RECOMMENDED_OUTPUT_DEVICE=" << env_result.c_str() << std::endl;
        preferred_device = env_dev;
    } else {
        std::cout << "Environment variable PA_RECOMMENDED_OUTPUT_DEVICE not set, using PortAudio defaults." << std::endl;
    }
#endif

    outputParameters.device = (preferred_device != -1) ? preferred_device : Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        std::cout << "Error: No default output device.\n";
    }

    outputParameters.channelCount = 2; /* Stereo output, most likely supported. */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output. */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    stream = NULL;

    err = Pa_OpenStream(&stream, NULL, &outputParameters, AUDIO_FREQUENCY, 1024, paClipOff, &patestCallback, this);

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cout << "Error starting stream: " << Pa_GetErrorText(err) << std::endl;
        std::cout << "\tPortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    while (!TestDestroy()) {

        if (m_pQueue->stackSize()) {

            while (m_pQueue->stackSize()) {
                AudioThreadTask task = m_pQueue->pop(); // pop a task from the queue. this will block the worker thread if queue is empty
                switch (task.m_cmd) {
                // case AudioThreadTask::AUDIO_THREAD_TUNING:
                //
                // audio_queue.push(newBuffer);

                }
            }
        }

        Sleep(1000);
    }
    std::cout << std::endl << "Audio Thread Done." << std::endl << std::endl;

    return (wxThread::ExitCode) 0;
}

