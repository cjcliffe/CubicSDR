#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "DemodulatorPreThread.h"
#include "CubicSDR.h"

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

    iqResampleRatio = (double) (params.bandwidth) / (double) params.sampleRate;
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
    liquid_float_complex carrySample;   // Keep the stream count even to simplify some demod operations
    bool carrySampleFlag = false;

    terminated = false;

    while (!terminated) {
        DemodulatorThreadIQData *inp;
        iqInputQueue->pop(inp);

        bool bandwidthChanged = false;
        bool rateChanged = false;
        DemodulatorThreadParameters tempParams = params;

        if (!commandQueue->empty()) {
            while (!commandQueue->empty()) {
                DemodulatorThreadCommand command;
                commandQueue->pop(command);
                switch (command.cmd) {
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH:
                    if (command.llong_value < 1500) {
                        command.llong_value = 1500;
                    }
                    if (command.llong_value > params.sampleRate) {
                        tempParams.bandwidth = params.sampleRate;
                    } else {
                        tempParams.bandwidth = command.llong_value;
                    }
                    bandwidthChanged = true;
                    break;
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY:
                    params.frequency = command.llong_value;
                    break;
                default:
                    break;
                }
            }
        }

        if (inp->sampleRate != tempParams.sampleRate && inp->sampleRate) {
            tempParams.sampleRate = inp->sampleRate;
            rateChanged = true;
        }

        if (bandwidthChanged || rateChanged) {
            DemodulatorWorkerThreadCommand command(DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS);
            command.sampleRate = tempParams.sampleRate;
            command.audioSampleRate = tempParams.audioSampleRate;
            command.bandwidth = tempParams.bandwidth;
            command.frequency = tempParams.frequency;

            workerQueue->push(command);
        }

        if (!initialized) {
            continue;
        }

        // Requested frequency is not center, shift it into the center!
        if (inp->frequency != params.frequency) {
            if ((params.frequency - inp->frequency) != shiftFrequency || rateChanged) {
                shiftFrequency = params.frequency - inp->frequency;
                if (abs(shiftFrequency) <= (int) ((double) (wxGetApp().getSampleRate() / 2) * 1.5)) {
                    nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) wxGetApp().getSampleRate())));
                }
            }
        }

        if (abs(shiftFrequency) > (int) ((double) (wxGetApp().getSampleRate() / 2) * 1.5)) {
            continue;
        }

//        std::lock_guard < std::mutex > lock(inp->m_mutex);
        std::vector<liquid_float_complex> *data = &inp->data;
        if (data->size() && (inp->sampleRate == params.sampleRate)) {
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

            int out_size = ceil((double) (bufSize) * iqResampleRatio) + 512;

            if (resampledData.size() != out_size) {
                if (resampledData.capacity() < out_size) {
                    resampledData.reserve(out_size);
                }
                resampledData.resize(out_size);
            }

            unsigned int numWritten;
            msresamp_crcf_execute(iqResampler, in_buf, bufSize, &resampledData[0], &numWritten);

            resamp->setRefCount(1);

            bool uneven = (numWritten % 2 != 0);

            if (!carrySampleFlag && !uneven) {
                resamp->data.assign(resampledData.begin(), resampledData.begin() + numWritten);
                carrySampleFlag = false;
            } else if (!carrySampleFlag && uneven) {
                resamp->data.assign(resampledData.begin(), resampledData.begin() + (numWritten-1));
                carrySample = resampledData.back();
                carrySampleFlag = true;
            } else if (carrySampleFlag && uneven) {
                resamp->data.resize(numWritten+1);
                resamp->data[0] = carrySample;
                memcpy(&resamp->data[1],&resampledData[0],sizeof(liquid_float_complex)*numWritten);
                carrySampleFlag = false;
            } else if (carrySampleFlag && !uneven) {
                resamp->data.resize(numWritten);
                resamp->data[0] = carrySample;
                memcpy(&resamp->data[1],&resampledData[0],sizeof(liquid_float_complex)*(numWritten-1));
                carrySample = resampledData.back();
                carrySampleFlag = true;
            }



            resamp->audioResampleRatio = audioResampleRatio;
            resamp->audioResampler = audioResampler;
            resamp->stereoResampler = stereoResampler;
            resamp->sampleRate = params.bandwidth;

            iqOutputQueue->push(resamp);
        }

        inp->decRefCount();

        if (!workerResults->empty()) {
            while (!workerResults->empty()) {
                DemodulatorWorkerThreadResult result;
                workerResults->pop(result);

                switch (result.cmd) {
                case DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS:
                    msresamp_crcf_destroy(iqResampler);


                    if (result.iqResampler) {
                        iqResampler = result.iqResampler;
                        iqResampleRatio = result.iqResampleRatio;
                    }

                    if (result.audioResampler) {
                        audioResampler = result.audioResampler;
                        audioResampleRatio = result.audioResamplerRatio;
                        stereoResampler = result.stereoResampler;
                        params.audioSampleRate = result.audioSampleRate;
                    }

                    if (params.bandwidth) {
                        params.bandwidth = result.bandwidth;
                    }
                    if (params.sampleRate) {
                        params.sampleRate = result.sampleRate;
                    }
                    break;
                default:
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

    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
    std::cout << "Demodulator preprocessor thread done." << std::endl;

#ifdef __APPLE__
    return this;
#endif
}

void DemodulatorPreThread::terminate() {
    terminated = true;
    DemodulatorThreadIQData *inp = new DemodulatorThreadIQData;    // push dummy to nudge queue
    iqInputQueue->push(inp);
    workerThread->terminate();
    t_Worker->detach();
    delete t_Worker;
}
