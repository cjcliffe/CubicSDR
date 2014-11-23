#include "SDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"

SDRThread::SDRThread(SDRThreadCommandQueue* pQueue) :
        m_pQueue(pQueue), iqDataOutQueue(NULL), iqVisualQueue(NULL) {
    dev = NULL;
    sample_rate = SRATE;
}

SDRThread::~SDRThread() {
    std::cout << std::endl << "SDR Thread Done." << std::endl << std::endl;
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

    int dev_count = rtlsdr_get_device_count();
    int first_available = enumerate_rtl();

    if (first_available == -1) {
        std::cout << "No devices found.. SDR Thread exiting.." << std::endl;
        return;
    } else {
        std::cout << "Using first available RTL-SDR device #" << first_available << std::endl;
    }

    signed char buf[BUF_SIZE];

    unsigned int frequency = DEFAULT_FREQ;
    unsigned int bandwidth = SRATE;

    rtlsdr_open(&dev, first_available);
    rtlsdr_set_sample_rate(dev, bandwidth);
    rtlsdr_set_center_freq(dev, frequency);
    rtlsdr_set_agc_mode(dev, 1);
    rtlsdr_set_offset_tuning(dev, 1);
    rtlsdr_reset_buffer(dev);

    sample_rate = rtlsdr_get_sample_rate(dev);

    std::cout << "Sample Rate is: " << sample_rate << std::endl;

    int n_read;
    double seconds = 0.0;

    std::cout << "Sampling..";
    while (1) {
        if (!m_pQueue->empty()) {
            bool freq_changed = false;
            float new_freq;

            while (!m_pQueue->empty()) {
                SDRThreadCommand command;
                m_pQueue->pop(command);

                switch (command.cmd) {
                case SDRThreadCommand::SDR_THREAD_CMD_TUNE:
                    std::cout << "Set frequency: " << command.int_value << std::endl;
                    freq_changed = true;
                    new_freq = command.int_value;
                    break;
                }
            }

            if (freq_changed) {
                frequency = new_freq;
                rtlsdr_set_center_freq(dev, frequency);
            }
        }

        rtlsdr_read_sync(dev, buf, BUF_SIZE, &n_read);

        std::vector<signed char> new_buffer;

        for (int i = 0; i < n_read; i++) {
            new_buffer.push_back(buf[i] - 127);
        }

        double time_slice = (double) n_read / (double) sample_rate;
        seconds += time_slice;

        SDRThreadIQData dataOut;
        dataOut.frequency = frequency;
        dataOut.bandwidth = bandwidth;
        dataOut.data = new_buffer;

        if (iqDataOutQueue != NULL) {
            iqDataOutQueue->push(dataOut);
        }

        if (iqVisualQueue != NULL) {
            iqVisualQueue->push(dataOut);
        }

        if (demodulators.size()) {
            for (int i = 0, iMax = demodulators.size(); i < iMax; i++) {
                DemodulatorThreadQueue *demodQueue = demodulators[i];
                DemodulatorThreadTask demod_task = DemodulatorThreadTask(DemodulatorThreadTask::DEMOD_THREAD_DATA);
                demod_task.data = new DemodulatorThreadIQData(bandwidth, frequency, new_buffer);
                demodQueue->addTask(demod_task, DemodulatorThreadQueue::DEMOD_PRIORITY_HIGHEST);
            }
        }

    }

}

