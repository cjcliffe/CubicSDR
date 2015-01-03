#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "DemodulatorPreThread.h"

DemodulatorPreThread::DemodulatorPreThread(DemodulatorThreadInputQueue* iqInputQueue, DemodulatorThreadPostInputQueue* iqOutputQueue,
        DemodulatorThreadControlCommandQueue *threadQueueControl, DemodulatorThreadCommandQueue* threadQueueNotify) :
        iqInputQueue(iqInputQueue), iqOutputQueue(iqOutputQueue), terminated(false), initialized(false), audioResampler(NULL), stereoResampler(NULL), iqResampleRatio(
                1), audioResampleRatio(1), iqResampler(NULL), commandQueue(NULL), threadQueueNotify(threadQueueNotify), threadQueueControl(
                threadQueueControl) {

    freqShifter = nco_crcf_create(LIQUID_VCO);
    shiftFrequency = 0;

    workerQueue = new DemodulatorThreadWorkerCommandQueue;
    workerResults = new DemodulatorThreadWorkerResultQueue;
    workerThread = new DemodulatorWorkerThread(workerQueue, workerResults);

    t_Worker = new std::thread(&DemodulatorWorkerThread::threadMain, workerThread);
}

void DemodulatorPreThread::initialize() {
    initialized = false;

    iqResampleRatio = (double) (params.bandwidth) / (double) params.inputRate;
    audioResampleRatio = (double) (params.audioSampleRate) / (double) params.bandwidth;

    float As = 120.0f;         // stop-band attenuation [dB]

    iqResampler = msresamp_crcf_create(iqResampleRatio, As);
    audioResampler = msresamp_rrrf_create(audioResampleRatio, As);
    stereoResampler = msresamp_rrrf_create(audioResampleRatio, As);

    initialized = true;
//    std::cout << "inputResampleRate " << params.bandwidth << std::endl;
    lastParams = params;
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
    sched_param prio = {priority}; // scheduling priority of thread
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
        iqInputQueue->pop(inp);

        bool bandwidthChanged = false;
        DemodulatorThreadParameters bandwidthParams = params;

        if (!commandQueue->empty()) {
            bool paramsChanged = false;
            while (!commandQueue->empty()) {
                DemodulatorThreadCommand command;
                commandQueue->pop(command);
                switch (command.cmd) {
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH:
                    if (command.int_value < 1500) {
                        command.int_value = 1500;
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
            if ((params.frequency - inp->frequency) != shiftFrequency) {
                shiftFrequency = params.frequency - inp->frequency;
                if (abs(shiftFrequency) <= (int) ((double) (SRATE / 2) * 1.5)) {
                    nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) SRATE)));
                }
            }
        }

        if (abs(shiftFrequency) > (int) ((double) (SRATE / 2) * 1.5)) {
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

            if (shiftFrequency != 0) {
                if (shiftFrequency < 0) {
                    nco_crcf_mix_block_up(freqShifter, in_buf, out_buf, bufSize);
                } else {
                    nco_crcf_mix_block_down(freqShifter, in_buf, out_buf, bufSize);
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

            resamp->audioResampleRatio = audioResampleRatio;
            resamp->audioResampler = audioResampler;
            resamp->stereoResampler = stereoResampler;
            resamp->resamplerRatio = iqResampleRatio;
            resamp->resampler = iqResampler;
            resamp->bandwidth = params.bandwidth;

            iqOutputQueue->push(resamp);
        }

        inp->decRefCount();

        if (!workerResults->empty()) {
            while (!workerResults->empty()) {
                DemodulatorWorkerThreadResult result;
                workerResults->pop(result);

                switch (result.cmd) {
                case DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS:
                    iqResampler = result.resampler;
                    audioResampler = result.audioResampler;
                    stereoResampler = result.stereoResampler;

                    iqResampleRatio = result.resamplerRatio;
                    audioResampleRatio = result.audioResamplerRatio;

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
    iqInputQueue->push(inp);
    workerThread->terminate();
}
