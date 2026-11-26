// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <atomic>
#include <mutex>

class CoreState {
public:
    virtual ~CoreState() = default;

    virtual long long getFrequency() = 0;
    virtual void setFrequency(long long freq) = 0;
    virtual long long getSampleRate() = 0;
    virtual void setSampleRate(long long rate) = 0;
    virtual int getPPM() = 0;
    virtual void setPPM(int ppm) = 0;
    virtual long long getOffset() = 0;
    virtual void setOffset(long long ofs) = 0;
    virtual int getFrequencySnap() = 0;
    virtual void setFrequencySnap(int snap) = 0;

    virtual bool getMainViewWindow(long long &centerHz, long long &spanHz) = 0;
    virtual bool getMainZoomedView(long long &centerFreq, long long &bandwidth) = 0;
    virtual void setMainZoomedView(long long centerFreq, long long bandwidth) = 0;
    virtual void resetMainZoomedView() = 0;
};

class CoreStateImpl : public CoreState {
public:
    explicit CoreStateImpl(long long rate);

    long long getFrequency() override;
    void setFrequency(long long freq) override;
    long long getSampleRate() override;
    void setSampleRate(long long rate) override;
    int getPPM() override;
    void setPPM(int ppm) override;
    long long getOffset() override;
    void setOffset(long long ofs) override;
    int getFrequencySnap() override;
    void setFrequencySnap(int snap) override;

    bool getMainViewWindow(long long &centerHz, long long &spanHz) override;
    bool getMainZoomedView(long long &centerFreq, long long &bandwidth) override;
    void setMainZoomedView(long long centerFreq, long long bandwidth) override;
    void resetMainZoomedView() override;

private:
    std::atomic_llong frequency;
    std::atomic_llong sampleRate;
    std::atomic_int ppm;
    std::atomic_llong offset;
    std::atomic_int snap;
    std::mutex viewBusy;
    bool zoomed;
    long long viewCenter;
    long long viewBandwidth;
};

inline CoreStateImpl::CoreStateImpl(long long rate)
    : frequency(0), sampleRate(rate), ppm(0), offset(0), snap(1), zoomed(false), viewCenter(0),
      viewBandwidth(0) {
}

inline long long CoreStateImpl::getFrequency() {
    return frequency.load();
}

inline void CoreStateImpl::setFrequency(long long freq) {
    frequency.store(freq);
}

inline long long CoreStateImpl::getSampleRate() {
    return sampleRate.load();
}

inline void CoreStateImpl::setSampleRate(long long rate) {
    sampleRate.store(rate);
}

inline int CoreStateImpl::getPPM() {
    return ppm.load();
}

inline void CoreStateImpl::setPPM(int ppmIn) {
    ppm.store(ppmIn);
}

inline long long CoreStateImpl::getOffset() {
    return offset.load();
}

inline void CoreStateImpl::setOffset(long long ofs) {
    offset.store(ofs);
}

inline int CoreStateImpl::getFrequencySnap() {
    return snap.load();
}

inline void CoreStateImpl::setFrequencySnap(int snapIn) {
    if (snapIn > 1000000) {
        snapIn = 1000000;
    }
    snap.store(snapIn);
}

inline bool CoreStateImpl::getMainViewWindow(long long &centerHz, long long &spanHz) {
    std::lock_guard<std::mutex> lock(viewBusy);
    if (zoomed) {
        centerHz = viewCenter;
        spanHz = viewBandwidth;
    } else {
        centerHz = frequency.load();
        spanHz = sampleRate.load();
    }
    return true;
}

inline bool CoreStateImpl::getMainZoomedView(long long &centerFreq, long long &bandwidth) {
    std::lock_guard<std::mutex> lock(viewBusy);
    if (zoomed) {
        centerFreq = viewCenter;
        bandwidth = viewBandwidth;
        return true;
    }
    return false;
}

inline void CoreStateImpl::setMainZoomedView(long long centerFreq, long long bandwidth) {
    std::lock_guard<std::mutex> lock(viewBusy);
    zoomed = true;
    viewCenter = centerFreq;
    viewBandwidth = bandwidth;
}

inline void CoreStateImpl::resetMainZoomedView() {
    std::lock_guard<std::mutex> lock(viewBusy);
    zoomed = false;
}
