#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "DemodulatorPreThread.h"

DemodulatorPreThread::DemodulatorPreThread(DemodulatorThreadInputQueue* pQueueIn, DemodulatorThreadPostInputQueue* pQueueOut,
        DemodulatorThreadControlCommandQueue *threadQueueControl, DemodulatorThreadCommandQueue* threadQueueNotify) :
        inputQueue(pQueueIn), postInputQueue(pQueueOut), terminated(false), initialized(false), audio_resampler(NULL), stereo_resampler(NULL), resample_ratio(
                1), audio_resample_ratio(1), resampler(NULL), commandQueue(NULL), audioInputQueue(NULL), threadQueueNotify(threadQueueNotify), threadQueueControl(
                threadQueueControl) {

    float kf = 0.5;         // modulation factor
    fdem = freqdem_create(kf);
//    freqdem_print(fdem);

    nco_shift = nco_crcf_create(LIQUID_VCO);
    shift_freq = 0;

    workerQueue = new DemodulatorThreadWorkerCommandQueue;
    workerResults = new DemodulatorThreadWorkerResultQueue;
    workerThread = new DemodulatorWorkerThread(workerQueue, workerResults);

    t_Worker = new std::thread(&DemodulatorWorkerThread::threadMain, workerThread);
}

void DemodulatorPreThread::initialize() {
    initialized = false;

    resample_ratio = (float) (params.bandwidth) / (float) params.inputRate;
    audio_resample_ratio = (float) (params.audioSampleRate) / (float) params.bandwidth;

    float As = 60.0f;         // stop-band attenuation [dB]

    // create multi-stage arbitrary resampler object
    if (resampler) {
        msresamp_crcf_destroy(resampler);
    }
    resampler = msresamp_crcf_create(resample_ratio, As);
//    msresamp_crcf_print(resampler);

    if (audio_resampler) {
        msresamp_rrrf_destroy(audio_resampler);
    }
    audio_resampler = msresamp_rrrf_create(audio_resample_ratio, As);
//    msresamp_crcf_print(audio_resampler);

    if (stereo_resampler) {
        msresamp_rrrf_destroy(stereo_resampler);
    }
    stereo_resampler = msresamp_rrrf_create(audio_resample_ratio, As);

    initialized = true;
//    std::cout << "inputResampleRate " << params.bandwidth << std::endl;

    last_params = params;
}

DemodulatorPreThread::~DemodulatorPreThread() {
    delete workerThread;
    delete workerQueue;
    delete workerResults;
}

#ifdef __APPLE__
void *DemodulatorPreThread::threadMain() {
#else
    void DemodulatorPreThread::threadMain() {
#endif
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO) - 1;
    sched_param prio = { priority }; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

    if (!initialized) {
        initialize();
    }

    std::cout << "Demodulator preprocessor thread started.." << std::endl;

    std::deque<DemodulatorThreadPostIQData *> buffers;
    std::deque<DemodulatorThreadPostIQData *>::iterator buffers_i;

    std::vector<liquid_float_complex> in_buf_data;
    std::vector<liquid_float_complex> out_buf_data;

    while (!terminated) {
        DemodulatorThreadIQData *inp;
        inputQueue->pop(inp);

        bool bandwidthChanged = false;
        DemodulatorThreadParameters bandwidthParams = params;

        if (!commandQueue->empty()) {
            bool paramsChanged = false;
            while (!commandQueue->empty()) {
                DemodulatorThreadCommand command;
                commandQueue->pop(command);
                switch (command.cmd) {
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH:
                    if (command.int_value < 3000) {
                        command.int_value = 3000;
                    }
                    if (command.int_value > SRATE) {
                        command.int_value = SRATE;
                    }
                    bandwidthParams.bandwidth = command.int_value;
                    bandwidthChanged = true;
                    break;
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY:
                    params.frequency = command.int_value;
                    break;
                }
            }

            if (bandwidthChanged) {
                DemodulatorWorkerThreadCommand command(DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS);
                command.audioSampleRate = bandwidthParams.audioSampleRate;
                command.bandwidth = bandwidthParams.bandwidth;
                command.frequency = bandwidthParams.frequency;
                command.inputRate = bandwidthParams.inputRate;

                workerQueue->push(command);
            }
        }

        if (!initialized) {
            continue;
        }

        // Requested frequency is not center, shift it into the center!
        if (inp->frequency != params.frequency) {
            if ((params.frequency - inp->frequency) != shift_freq) {
                shift_freq = params.frequency - inp->frequency;
                if (abs(shift_freq) <= (int) ((float) (SRATE / 2) * 1.5)) {
                    nco_crcf_set_frequency(nco_shift, (2.0 * M_PI) * (((float) abs(shift_freq)) / ((float) SRATE)));
                }
            }
        }

        if (abs(shift_freq) > (int) ((float) (SRATE / 2) * 1.5)) {
            continue;
        }

//        std::lock_guard < std::mutex > lock(inp->m_mutex);
        std::vector<liquid_float_complex> *data = &inp->data;
        if (data->size()) {
            int bufSize = data->size();

            if (in_buf_data.size() != bufSize) {
                if (in_buf_data.capacity() < bufSize) {
                    in_buf_data.reserve(bufSize);
                    out_buf_data.reserve(bufSize);
                }
                in_buf_data.resize(bufSize);
                out_buf_data.resize(bufSize);
            }

            in_buf_data.assign(inp->data.begin(), inp->data.end());

            liquid_float_complex *in_buf = &in_buf_data[0];
            liquid_float_complex *out_buf = &out_buf_data[0];
            liquid_float_complex *temp_buf = NULL;

            if (shift_freq != 0) {
                if (shift_freq < 0) {
                    nco_crcf_mix_block_up(nco_shift, in_buf, out_buf, bufSize);
                } else {
                    nco_crcf_mix_block_down(nco_shift, in_buf, out_buf, bufSize);
                }
                temp_buf = in_buf;
                in_buf = out_buf;
                out_buf = temp_buf;
            }

            DemodulatorThreadPostIQData *resamp = NULL;

            for (buffers_i = buffers.begin(); buffers_i != buffers.end(); buffers_i++) {
                if ((*buffers_i)->getRefCount() <= 0) {
                    resamp = (*buffers_i);
                    break;
                }
            }

            if (resamp == NULL) {
                resamp = new DemodulatorThreadPostIQData;
                buffers.push_back(resamp);
            }

            resamp->setRefCount(1);
            resamp->data.assign(in_buf, in_buf + bufSize);

//            firfilt_crcf_execute_block(fir_filter, in_buf, bufSize, &((*resamp.data)[0]));

            resamp->audio_resample_ratio = audio_resample_ratio;
            resamp->audio_resampler = audio_resampler;
            resamp->stereo_resampler = stereo_resampler;
            resamp->resample_ratio = resample_ratio;
            resamp->resampler = resampler;
            resamp->bandwidth = params.bandwidth;

            postInputQueue->push(resamp);
        }

        inp->decRefCount();

        if (!workerResults->empty()) {
            while (!workerResults->empty()) {
                DemodulatorWorkerThreadResult result;
                workerResults->pop(result);

                switch (result.cmd) {
                case DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS:
                    resampler = result.resampler;
                    audio_resampler = result.audio_resampler;
                    stereo_resampler = result.stereo_resampler;

                    resample_ratio = result.resample_ratio;
                    audio_resample_ratio = result.audio_resample_ratio;

                    params.audioSampleRate = result.audioSampleRate;
                    params.bandwidth = result.bandwidth;
                    params.inputRate = result.inputRate;

                    break;
                }
            }
        }
    }

    while (!buffers.empty()) {
        DemodulatorThreadPostIQData *iqDataDel = buffers.front();
        buffers.pop_front();
        delete iqDataDel;
    }

    std::cout << "Demodulator preprocessor thread done." << std::endl;
    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
}

void DemodulatorPreThread::terminate() {
    terminated = true;
    DemodulatorThreadIQData *inp = new DemodulatorThreadIQData;    // push dummy to nudge queue
    inputQueue->push(inp);
    workerThread->terminate();
}
