#include "DemodulatorWorkerThread.h"
#include "CubicSDRDefs.h"
#include "CubicSDR.h"
#include <vector>

DemodulatorWorkerThread::DemodulatorWorkerThread() : IOThread(),
        commandQueue(NULL), resultQueue(NULL), cModem(nullptr), cModemKit(nullptr) {
}

DemodulatorWorkerThread::~DemodulatorWorkerThread() {
}

void DemodulatorWorkerThread::run() {

//    std::cout << "Demodulator worker thread started.." << std::endl;
    
    commandQueue = static_cast<DemodulatorThreadWorkerCommandQueue *>(getInputQueue("WorkerCommandQueue"));
    resultQueue = static_cast<DemodulatorThreadWorkerResultQueue *>(getOutputQueue("WorkerResultQueue"));
    
    while (!terminated) {
        bool filterChanged = false;
        bool makeDemod = false;
        DemodulatorWorkerThreadCommand filterCommand, demodCommand;
        DemodulatorWorkerThreadCommand command;

        bool done = false;
        while (!done) {
            commandQueue->pop(command);
            switch (command.cmd) {
                case DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS:
                    filterChanged = true;
                    filterCommand = command;
                    break;
                case DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_MAKE_DEMOD:
                    makeDemod = true;
                    demodCommand = command;
                    break;
                default:
                    break;
            }
            done = commandQueue->empty();
        }

        if ((makeDemod || filterChanged) && !terminated) {
            DemodulatorWorkerThreadResult result(DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS);
            
            
            if (filterCommand.sampleRate) {
                result.sampleRate = filterCommand.sampleRate;
            }
            
            if (makeDemod) {
                cModem = Modem::makeModem(demodCommand.demodType);
                cModemName = cModem->getName();
                cModemType = cModem->getType();
                if (demodCommand.settings.size()) {
                    cModem->writeSettings(demodCommand.settings);
                }
                result.sampleRate = demodCommand.sampleRate;
                wxGetApp().getAppFrame()->updateModemProperties(cModem->getSettings());
            }
            result.modem = cModem;

            if (makeDemod && demodCommand.bandwidth && demodCommand.audioSampleRate) {
                if (cModem != nullptr) {
                    result.bandwidth = cModem->checkSampleRate(demodCommand.bandwidth, demodCommand.audioSampleRate);
                    cModemKit = cModem->buildKit(result.bandwidth, demodCommand.audioSampleRate);
                } else {
                    cModemKit = nullptr;
                }
            } else if (filterChanged && filterCommand.bandwidth && filterCommand.audioSampleRate) {
                if (cModem != nullptr) {
                    result.bandwidth = cModem->checkSampleRate(filterCommand.bandwidth, filterCommand.audioSampleRate);
                    cModemKit = cModem->buildKit(result.bandwidth, filterCommand.audioSampleRate);
                } else {
                    cModemKit = nullptr;
                }
            } else if (makeDemod) {
                cModemKit = nullptr;
            }
            if (cModem != nullptr) {
                cModem->clearRebuildKit();
            }
            
            float As = 60.0f;         // stop-band attenuation [dB]
            
            if (cModem && result.sampleRate && result.bandwidth) {
                result.bandwidth = cModem->checkSampleRate(result.bandwidth, makeDemod?demodCommand.audioSampleRate:filterCommand.audioSampleRate);
                result.iqResampleRatio = (double) (result.bandwidth) / (double) result.sampleRate;
                result.iqResampler = msresamp_crcf_create(result.iqResampleRatio, As);
            }

            result.modemKit = cModemKit;
            result.modemType = cModemType;
            result.modemName = cModemName;
            
            resultQueue->push(result);
        }

    }

//    std::cout << "Demodulator worker thread done." << std::endl;
}

void DemodulatorWorkerThread::terminate() {
    terminated = true;
    DemodulatorWorkerThreadCommand inp;    // push dummy to nudge queue
    commandQueue->push(inp);
}
