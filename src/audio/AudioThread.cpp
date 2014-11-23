#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>

static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData) {

    AudioThread *src = (AudioThread *) userData;

    float *out = (float*) outputBuffer;

    if (statusFlags & paOutputOverflow) {
        std::cout << "Audio buffer overflow.." << std::endl;
    }

    if (statusFlags & paOutputUnderflow) {
        std::cout << "Audio buffer underflow.." << std::endl;
    }

    if ((statusFlags & paPrimingOutput) || (statusFlags & paOutputOverflow) || !src->audio_queue.size()) {
        for (int i = 0; i < framesPerBuffer * 2; i++) {
            out[i] = 0;
        }
        return paContinue;
    }

    std::vector<float> nextBuffer = src->audio_queue.front();

    for (int i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = nextBuffer[src->audio_queue_ptr];

        src->audio_queue_ptr++;

        if (src->audio_queue_ptr == nextBuffer.size()) {
            src->audio_queue.pop();
            src->audio_queue_ptr = 0;
            if (!src->audio_queue.size()) {
                break;
            }
            nextBuffer = src->audio_queue.front();
        }
    }

    return paContinue;
}

void AudioThread::threadMain() {
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "Error starting portaudio :(\n";
        return;
    }

    int preferred_device = -1;

    outputParameters.device = (preferred_device != -1) ? preferred_device : Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        std::cout << "Error: No default output device.\n";
    }

    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    stream = NULL;

    err = Pa_OpenStream(&stream, NULL, &outputParameters, AUDIO_FREQUENCY, paFramesPerBufferUnspecified,
            paPrimeOutputBuffersUsingStreamCallback | paClipOff, &audioCallback, this);

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cout << "Error starting stream: " << Pa_GetErrorText(err) << std::endl;
        std::cout << "\tPortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    while (1) {
        AudioThreadInput inp;
        inputQueue->pop(inp);
        audio_queue.push(inp.data);
    }
}

/*
 #ifdef WIN32
 #include <algorithm>
 #include <functional>
 #include <cctype>
 #include <locale>
 #include <sstream>

 // trim from start
 static inline std::wstring &wltrim(std::wstring &s) {
 s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
 return s;
 }

 // trim from end
 static inline std::wstring &wrtrim(std::wstring &s) {
 s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
 return s;
 }

 // trim from both ends
 static inline std::wstring &wtrim(std::wstring &s) {
 return wltrim(wrtrim(s));
 }
 #endif

 //wxDEFINE_EVENT(wxEVT_COMMAND_AudioThread_INPUT, wxThreadEvent);

 AudioThread::AudioThread(AudioThreadQueue* pQueue, int id) :
 wxThread(wxTHREAD_DETACHED), m_pQueue(pQueue), m_ID(id), audio_queue_ptr(0), stream(NULL) {

 }
 AudioThread::~AudioThread() {
 }



 wxThread::ExitCode AudioThread::Entry() {



 //#ifdef WIN32
 //    wchar_t dev_str[255];
 //    memset(dev_str, 0, sizeof(wchar_t) * 255);
 //    std::wstring env_name(L"PA_RECOMMENDED_OUTPUT_DEVICE");
 //    GetEnvironmentVariable(wtrim(env_name).c_str(), dev_str, 255);
 //    std::wistringstream env_result(dev_str);
 //
 //    if (!env_result.eof()) {
 //        int env_dev = -1;
 //        env_result >> env_dev;
 //
 //        if (env_result.eof()) { // read everything, was all a number
 //            if (env_dev >= 0) {
 //                std::cout << "Using preferred PortAudio device PA_RECOMMENDED_OUTPUT_DEVICE=" << env_dev << std::endl;
 //                preferred_device = env_dev;
 //            } else {
 //                std::cout << "Environment variable PA_RECOMMENDED_OUTPUT_DEVICE not set, using PortAudio defaults." << std::endl;
 //            }
 //        } else {
 //            std::cout << "Environment variable PA_RECOMMENDED_OUTPUT_DEVICE didn't evaluate to a number, using PortAudio defaults." << std::endl;
 //        }
 //    }
 //#endif

 while (!TestDestroy()) {

 if (m_pQueue->stackSize()) {

 while (m_pQueue->stackSize()) {
 AudioThreadTask task = m_pQueue->pop(); // pop a task from the queue. this will block the worker thread if queue is empty
 switch (task.m_cmd) {
 case AudioThreadTask::AUDIO_THREAD_DATA:
 if (!TestDestroy()) {
 audio_queue.push(task.data->data);
 }
 delete task.data;
 break;
 }
 }
 } else {
 this->Yield();
 this->Sleep(1);
 }
 }
 std::cout << std::endl << "Audio Thread Done." << std::endl << std::endl;

 return (wxThread::ExitCode) 0;
 }

 */
