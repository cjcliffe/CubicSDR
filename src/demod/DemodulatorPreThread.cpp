// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "DemodulatorPreThread.h"
#include "CubicSDR.h"
#include "DemodulatorInstance.h"

//50 ms
#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

DemodulatorPreThread::DemodulatorPreThread(DemodulatorInstance* parent) : IOThread(), iqResampler(nullptr), iqResampleRatio(1), cModem(nullptr), cModemKit(nullptr)
 {
	initialized.store(false);
    this->parent = parent;

    freqShifter = nco_crcf_create(LIQUID_VCO);
    shiftFrequency = 0;

    workerQueue = std::make_shared<DemodulatorThreadWorkerCommandQueue>();
    workerQueue->set_max_num_items(2);

    workerResults = std::make_shared<DemodulatorThreadWorkerResultQueue>();
    workerResults->set_max_num_items(100);
     
    workerThread = new DemodulatorWorkerThread();
    workerThread->setInputQueue("WorkerCommandQueue",workerQueue);
    workerThread->setOutputQueue("WorkerResultQueue",workerResults);
     
    newSampleRate = currentSampleRate = 0;
    newBandwidth = currentBandwidth = 0;
    newAudioSampleRate = currentAudioSampleRate = 0;
    newFrequency = currentFrequency = 0;

    sampleRateChanged.store(false);
    frequencyChanged.store(false);
    bandwidthChanged.store(false);
    audioSampleRateChanged.store(false);
    modemSettingsChanged.store(false);
    demodTypeChanged.store(false);
}

bool DemodulatorPreThread::isInitialized() {
    return initialized.load();
}

DemodulatorPreThread::~DemodulatorPreThread() = default;

void DemodulatorPreThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO) - 1;
    sched_param prio = {priority, {0}}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

//    std::cout << "Demodulator preprocessor thread started.." << std::endl;

    ReBuffer<DemodulatorThreadPostIQData> buffers("DemodulatorPreThreadBuffers");

    iqInputQueue = std::static_pointer_cast<DemodulatorThreadInputQueue>(getInputQueue("IQDataInput"));
    iqOutputQueue = std::static_pointer_cast<DemodulatorThreadPostInputQueue>(getOutputQueue("IQDataOutput"));
    
    std::vector<liquid_float_complex> in_buf_data;
    std::vector<liquid_float_complex> out_buf_data;

    t_Worker = new std::thread(&DemodulatorWorkerThread::threadMain, workerThread);
    
    while (!stopping) {
        DemodulatorThreadIQDataPtr inp;

        if (!iqInputQueue->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }
        
        if (frequencyChanged.load()) {
            currentFrequency.store(newFrequency);
            frequencyChanged.store(false);
        }
        
        if (inp->sampleRate != currentSampleRate) {
            newSampleRate = inp->sampleRate;
            if (newSampleRate) {
                sampleRateChanged.store(true);
            }
        }
        
        if (!newAudioSampleRate) {
            newAudioSampleRate = parent->getAudioSampleRate();
            if (newAudioSampleRate) {
                audioSampleRateChanged.store(true);
            }
        } else if (parent->getAudioSampleRate() != newAudioSampleRate) {
            if (parent->getAudioSampleRate()) {
                newAudioSampleRate = parent->getAudioSampleRate();
                audioSampleRateChanged.store(true);
            }
        }
        
        if (demodTypeChanged.load() && (newSampleRate && newAudioSampleRate && newBandwidth)) {
            DemodulatorWorkerThreadCommand command(DemodulatorWorkerThreadCommand::Type::DEMOD_WORKER_THREAD_CMD_MAKE_DEMOD);
            command.frequency = newFrequency;
            command.sampleRate = newSampleRate;
            command.demodType = newDemodType;
            command.bandwidth = newBandwidth;
            command.audioSampleRate = newAudioSampleRate;
            demodType = newDemodType;
            sampleRateChanged.store(false);
            audioSampleRateChanged.store(false);
            ModemSettings lastSettings = parent->getLastModemSettings(newDemodType);
            if (!lastSettings.empty()) {
                command.settings = lastSettings;
                if (!modemSettingsBuffered.empty()) {
                    for (ModemSettings::const_iterator msi = modemSettingsBuffered.begin(); msi != modemSettingsBuffered.end(); msi++) {
                        command.settings[msi->first] = msi->second;
                    }
                }
            } else {
                command.settings = modemSettingsBuffered;
            }
            modemSettingsBuffered.clear();
            modemSettingsChanged.store(false);
            //VSO: blocking push
            workerQueue->push(command);
            cModem = nullptr;
            cModemKit = nullptr;
            demodTypeChanged.store(false);
            initialized.store(false);
        }
        else if (
            cModemKit && cModem &&
            (bandwidthChanged.load() || sampleRateChanged.load() || audioSampleRateChanged.load() || cModem->shouldRebuildKit()) &&
            (newSampleRate && newAudioSampleRate && newBandwidth)
        ) {
            DemodulatorWorkerThreadCommand command(DemodulatorWorkerThreadCommand::Type::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS);
            command.frequency = newFrequency;
            command.sampleRate = newSampleRate;
            command.bandwidth = newBandwidth;
            command.audioSampleRate = newAudioSampleRate;
            bandwidthChanged.store(false);
            sampleRateChanged.store(false);
            audioSampleRateChanged.store(false);
            modemSettingsBuffered.clear();
            //VSO: blocking
            workerQueue->push(command);
        }
        
        // Requested frequency is not center, shift it into the center!
        if ((currentFrequency - inp->frequency) != shiftFrequency) {
            shiftFrequency = currentFrequency - inp->frequency;
            if (abs(shiftFrequency) <= (int) ((double) (inp->sampleRate / 2) * 1.5)) {
                nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) inp->sampleRate)));
            }
        }

        if (cModem && cModemKit && abs(shiftFrequency) > (int) ((double) (inp->sampleRate / 2) * 1.5)) {
          
            continue;
        }

//        std::lock_guard < std::mutex > lock(inp->m_mutex);
        std::vector<liquid_float_complex> *data = &inp->data;
        if (!data->empty() && (inp->sampleRate == currentSampleRate) && cModem && cModemKit) {
            size_t bufSize = data->size();

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
            liquid_float_complex *temp_buf;

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

            DemodulatorThreadPostIQDataPtr resamp = buffers.getBuffer();

            size_t out_size = ceil((double) (bufSize) * iqResampleRatio) + 512;

            if (resampledData.size() != out_size) {
                if (resampledData.capacity() < out_size) {
                    resampledData.reserve(out_size);
                }
                resampledData.resize(out_size);
            }

            unsigned int numWritten;
            msresamp_crcf_execute(iqResampler, in_buf, bufSize, &resampledData[0], &numWritten);

            resamp->data.assign(resampledData.begin(), resampledData.begin() + numWritten);

            resamp->modemType = cModem->getType();
            resamp->modemName = cModem->getName();
            resamp->modem = cModem;
            resamp->modemKit = cModemKit;
            resamp->sampleRate = currentBandwidth;

            //VSO: blocking push
            iqOutputQueue->push(resamp);   
        }

        DemodulatorWorkerThreadResult result;
        //process all worker results until 
        while (!stopping && workerResults->try_pop(result)) {
              
            switch (result.cmd) {
                case DemodulatorWorkerThreadResult::Type::DEMOD_WORKER_THREAD_RESULT_FILTERS:
                    if (result.iqResampler) {
                        if (iqResampler) {
                            msresamp_crcf_destroy(iqResampler);
                        }
                        iqResampler = result.iqResampler;
                        iqResampleRatio = result.iqResampleRatio;
                    }

                    if (result.modem != nullptr) {
                        cModem = result.modem;
#if ENABLE_DIGITAL_LAB
                        if (cModem->getType() == "digital") {
                            auto *mDigi = (ModemDigital *)cModem;
                            mDigi->setOutput(parent->getOutput());
                        }
#endif
                    }
                    
                    if (result.modemKit != nullptr) {
                        cModemKit = result.modemKit;
                        currentAudioSampleRate = cModemKit->audioSampleRate;
                    }
                        
                    if (result.bandwidth) {
                        currentBandwidth = result.bandwidth;
                    }

                    if (result.sampleRate) {
                        currentSampleRate = result.sampleRate;
                    }
                        
                    if (!result.modemName.empty()) {
                        demodType = result.modemName;
                        demodTypeChanged.store(false);
                    }
                        
                    shiftFrequency = inp->frequency-1;
                    initialized.store(cModem != nullptr);
                    break;
                default:
                    break;
            }
        } //end while
        
        if ((cModem != nullptr) && modemSettingsChanged.load()) {
            cModem->writeSettings(modemSettingsBuffered);
            modemSettingsBuffered.clear();
            modemSettingsChanged.store(false);
        }
    } //end while stopping

   
    iqOutputQueue->flush();
    iqInputQueue->flush();
}

void DemodulatorPreThread::setDemodType(std::string demodType_in) {
    newDemodType = demodType_in;
    demodTypeChanged.store(true);
}

std::string DemodulatorPreThread::getDemodType() {
    if (demodTypeChanged.load()) {
        return newDemodType;
    }
    return demodType;
}

void DemodulatorPreThread::setFrequency(long long freq) {
    frequencyChanged.store(true);
    newFrequency = freq;
}

long long DemodulatorPreThread::getFrequency() {
    if (frequencyChanged.load()) {
        return newFrequency;
    }
    return currentFrequency;
}

void DemodulatorPreThread::setSampleRate(long long sampleRate) {
    sampleRateChanged.store(true);
    newSampleRate = sampleRate;
}

long long DemodulatorPreThread::getSampleRate() {
    if (sampleRateChanged.load()) {
        return newSampleRate;
    }
    return currentSampleRate;
}

void DemodulatorPreThread::setBandwidth(int bandwidth) {
    bandwidthChanged.store(true);
    newBandwidth = bandwidth;
}

int DemodulatorPreThread::getBandwidth() {
//    if (bandwidthChanged.load()) {
//        return newBandwidth;
//    }

    return currentBandwidth;
}

void DemodulatorPreThread::setAudioSampleRate(int rate) {
    audioSampleRateChanged.store(true);
    newAudioSampleRate = rate;
}

int DemodulatorPreThread::getAudioSampleRate() {
    if (audioSampleRateChanged.load()) {
        return newAudioSampleRate;
    }
    return currentAudioSampleRate;
}

void DemodulatorPreThread::terminate() {

    //make non-blocking calls to be sure threads are flagged for termination.
    IOThread::terminate();
    workerThread->terminate();

    //unblock the push()
    iqOutputQueue->flush();
    iqInputQueue->flush();

    //wait blocking for termination here, it could be long with lots of modems and we MUST terminate properly,
    //else better kill the whole application...
    workerThread->isTerminated(5000);

    t_Worker->join();
    delete t_Worker;
    t_Worker = nullptr;

    delete workerThread;
    workerThread = nullptr;
}

Modem *DemodulatorPreThread::getModem() {
    return cModem;
}

ModemKit *DemodulatorPreThread::getModemKit() {
    return cModemKit;
}


std::string DemodulatorPreThread::readModemSetting(const std::string& setting) {
    if (cModem) {
        return cModem->readSetting(setting);
    } else if (modemSettingsBuffered.find(setting) != modemSettingsBuffered.end()) {
        return modemSettingsBuffered[setting];
    }
    return "";
}

void DemodulatorPreThread::writeModemSetting(const std::string& setting, std::string value) {
    modemSettingsBuffered[setting] = value;
    modemSettingsChanged.store(true);
}

ModemSettings DemodulatorPreThread::readModemSettings() {
    if (cModem) {
        return cModem->readSettings();
    } else {
        return modemSettingsBuffered;
    }
}

void DemodulatorPreThread::writeModemSettings(ModemSettings settings) {
    modemSettingsBuffered = settings;
    modemSettingsChanged.store(true);
}
