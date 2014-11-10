#include "SDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>

//wxDEFINE_EVENT(wxEVT_COMMAND_SDRThread_INPUT, wxThreadEvent);

SDRThread::SDRThread(SDRThreadQueue* pQueue, int id) :
        wxThread(wxTHREAD_DETACHED), m_pQueue(pQueue), m_ID(id) {
    dev = NULL;
    sample_rate = SRATE;
}
SDRThread::~SDRThread() {

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

wxThread::ExitCode SDRThread::Entry() {

    int dev_count = rtlsdr_get_device_count();
    int first_available = enumerate_rtl();

    if (first_available == -1) {
        std::cout << "No devices found.. SDR Thread exiting.." << std::endl;
        return (wxThread::ExitCode) 0;
    } else {
        std::cout << "Using first available RTL-SDR device #" << first_available << std::endl;
    }

    signed char *buf = (signed char *) malloc(BUF_SIZE);

    rtlsdr_open(&dev, first_available);
    rtlsdr_set_sample_rate(dev, SRATE);
    rtlsdr_set_center_freq(dev, DEFAULT_FREQ);
    rtlsdr_set_agc_mode(dev, 1);
    rtlsdr_set_offset_tuning(dev, 1);
    rtlsdr_reset_buffer(dev);

    sample_rate = rtlsdr_get_sample_rate(dev);

    std::cout << "Sample Rate is: " << sample_rate << std::endl;

    int n_read;
    double seconds = 0.0;

    std::cout << "Sampling..";
    while (!TestDestroy()) {

        if (m_pQueue->stackSize()) {
            bool freq_changed = false;
            float new_freq;

            while (m_pQueue->stackSize()) {
                SDRThreadTask task = m_pQueue->pop(); // pop a task from the queue. this will block the worker thread if queue is empty
                switch (task.m_cmd) {
                case SDRThreadTask::SDR_THREAD_TUNING:
                    std::cout << "Set frequency: " << task.getUInt() << std::endl;
                    freq_changed = true;
                    new_freq = task.getUInt();
                    break;
                }
            }

            if (freq_changed) {
                rtlsdr_set_center_freq(dev, new_freq);
            }
        }

        rtlsdr_read_sync(dev, buf, BUF_SIZE, &n_read);

        if (!TestDestroy()) {
            std::vector<signed char> *new_buffer = new std::vector<signed char>();

            for (int i = 0; i < n_read; i++) {
                new_buffer->push_back(buf[i] - 127);
            }

            double time_slice = (double) n_read / (double) sample_rate;
            seconds += time_slice;

            // std::cout << "Time Slice: " << time_slice << std::endl;
            if (!TestDestroy()) {
                wxThreadEvent event(wxEVT_THREAD, EVENT_SDR_INPUT);
                event.SetPayload(new_buffer);
                wxQueueEvent(m_pQueue->getHandler(), event.Clone());
            } else {
                delete new_buffer;
            }
        }
    }
    std::cout << std::endl << "Done." << std::endl << std::endl;

    rtlsdr_close(dev);
    free(buf);

    return (wxThread::ExitCode) 0;
}

