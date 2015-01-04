#include "SDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"

SDRThread::SDRThread(SDRThreadCommandQueue* pQueue) :
        commandQueue(pQueue), iqDataOutQueue(NULL), terminated(false) {
    dev = NULL;
    sampleRate = SRATE;
}

SDRThread::~SDRThread() {
    rtlsdr_close(dev);
}

int SDRThread::enumerate_rtl() {

    int first_available = -1;

    char manufact[256], product[256], serial[256];

    unsigned int rtl_count = rtlsdr_get_device_count();

    std::cout << "RTL Devices: " << rtl_count << std::endl;

    for (int i = 0; i < rtl_count; i++) {
        std::cout << "Device #" << i << ": " << rtlsdr_get_device_name(i) << std::endl;
        if (rtlsdr_get_device_usb_strings(i, manufact, product, serial) == 0) {
            std::cout << "\tManufacturer: " << manufact << ", Product Name: " << product << ", Serial: " << serial << std::endl;

            rtlsdr_open(&dev, i);

            std::cout << "\t Tuner type: ";
            switch (rtlsdr_get_tuner_type(dev)) {
            case RTLSDR_TUNER_UNKNOWN:
                std::cout << "Unknown";
                break;
            case RTLSDR_TUNER_E4000:
                std::cout << "Elonics E4000";
                break;
            case RTLSDR_TUNER_FC0012:
                std::cout << "Fitipower FC0012";
                break;
            case RTLSDR_TUNER_FC0013:
                std::cout << "Fitipower FC0013";
                break;
            case RTLSDR_TUNER_FC2580:
                std::cout << "Fitipower FC2580";
                break;
            case RTLSDR_TUNER_R820T:
                std::cout << "Rafael Micro R820T";
                break;
            case RTLSDR_TUNER_R828D:
                break;
            }

            std::cout << std::endl;
            /*
             int num_gains = rtlsdr_get_tuner_gains(dev, NULL);

             int *gains = (int *)malloc(sizeof(int) * num_gains);
             rtlsdr_get_tuner_gains(dev, gains);

             std::cout << "\t Valid gains: ";
             for (int g = 0; g < num_gains; g++) {
             if (g > 0) {
             std::cout << ", ";
             }
             std::cout << ((float)gains[g]/10.0f);
             }
             std::cout << std::endl;

             free(gains);
             */

            rtlsdr_close(dev);
            if (first_available == -1) {
                first_available = i;
            }

        } else {
            std::cout << "\tUnable to access device #" << i << " (in use?)" << std::endl;
        }

    }

    return first_available;

}

void SDRThread::threadMain() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO) - 1;
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

    std::cout << "SDR thread initializing.." << std::endl;

    int devCount = rtlsdr_get_device_count();
    int firstDevAvailable = enumerate_rtl();

    if (firstDevAvailable == -1) {
        std::cout << "No devices found.. SDR Thread exiting.." << std::endl;
        return;
    } else {
        std::cout << "Using first available RTL-SDR device #" << firstDevAvailable << std::endl;
    }

    signed char buf[BUF_SIZE];

    unsigned int frequency = DEFAULT_FREQ;
    unsigned int bandwidth = SRATE;

    rtlsdr_open(&dev, firstDevAvailable);
    rtlsdr_set_sample_rate(dev, bandwidth);
    rtlsdr_set_center_freq(dev, frequency);
    rtlsdr_set_agc_mode(dev, 1);
    rtlsdr_set_offset_tuning(dev, 0);
    rtlsdr_reset_buffer(dev);

    sampleRate = rtlsdr_get_sample_rate(dev);

    std::cout << "Sample Rate is: " << sampleRate << std::endl;

    int n_read;
    double seconds = 0.0;

    std::cout << "SDR thread started.." << std::endl;

    std::deque<SDRThreadIQData *> buffers;
    std::deque<SDRThreadIQData *>::iterator buffers_i;

    while (!terminated) {
        SDRThreadCommandQueue *cmdQueue = commandQueue.load();

        if (!cmdQueue->empty()) {
            bool freq_changed = false;
            float new_freq;

            while (!cmdQueue->empty()) {
                SDRThreadCommand command;
                cmdQueue->pop(command);

                switch (command.cmd) {
                case SDRThreadCommand::SDR_THREAD_CMD_TUNE:
                    freq_changed = true;
                    new_freq = command.int_value;
                    if (new_freq < SRATE / 2) {
                        new_freq = SRATE / 2;
                    }
                    std::cout << "Set frequency: " << new_freq << std::endl;
                    break;
                default:
                    break;
                }
            }

            if (freq_changed) {
                frequency = new_freq;
                rtlsdr_set_center_freq(dev, frequency);
            }
        }

        rtlsdr_read_sync(dev, buf, BUF_SIZE, &n_read);

        SDRThreadIQData *dataOut = NULL;

        for (buffers_i = buffers.begin(); buffers_i != buffers.end(); buffers_i++) {
            if ((*buffers_i)->getRefCount() <= 0) {
                dataOut = (*buffers_i);
                break;
            }
        }

        if (dataOut == NULL) {
            dataOut = new SDRThreadIQData;
            buffers.push_back(dataOut);
        }

//        std::lock_guard < std::mutex > lock(dataOut->m_mutex);
        dataOut->setRefCount(1);
        dataOut->frequency = frequency;
        dataOut->bandwidth = bandwidth;

        if (dataOut->data.capacity() < n_read) {
            dataOut->data.reserve(n_read);
        }

        if (dataOut->data.size() != n_read) {
            dataOut->data.resize(n_read);
        }

        for (int i = 0; i < n_read; i++) {
            dataOut->data[i] = buf[i] - 127;
        }

        double time_slice = (double) n_read / (double) sampleRate;
        seconds += time_slice;

        if (iqDataOutQueue != NULL) {
            iqDataOutQueue.load()->push(dataOut);
        }
    }

    while (!buffers.empty()) {
        SDRThreadIQData *iqDataDel = buffers.front();
        buffers.pop_front();
//        std::lock_guard < std::mutex > lock(iqDataDel->m_mutex);
//        delete iqDataDel;
    }

    std::cout << "SDR thread done." << std::endl;
}

void SDRThread::terminate() {
    terminated = true;
}
