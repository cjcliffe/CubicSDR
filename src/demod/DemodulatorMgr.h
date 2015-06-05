#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorInstance.h"

class DemodulatorMgr {
public:
    DemodulatorMgr();
    ~DemodulatorMgr();

    DemodulatorInstance *newThread();
    std::vector<DemodulatorInstance *> &getDemodulators();
    std::vector<DemodulatorInstance *> *getDemodulatorsAt(long long freq, int bandwidth);
    void deleteThread(DemodulatorInstance *);

    void terminateAll();

    void setActiveDemodulator(DemodulatorInstance *demod, bool temporary = true);
    DemodulatorInstance *getActiveDemodulator();
    DemodulatorInstance *getLastActiveDemodulator();

    int getLastBandwidth() const;
    void setLastBandwidth(int lastBandwidth);

    int getLastDemodulatorType() const;
    void setLastDemodulatorType(int lastDemodType);
    
    bool getLastDemodulatorLock() const;
    void setLastDemodulatorLock(bool lastDemodLock);

    float getLastGain() const;
    void setLastGain(float lastGain);

    float getLastSquelchLevel() const;
    void setLastSquelchLevel(float lastSquelch);

    bool isLastSquelchEnabled() const;
    void setLastSquelchEnabled(bool lastSquelchEnabled);

    bool isLastStereo() const;
    void setLastStereo(bool lastStereo);

private:
    void garbageCollect();
    void updateLastState();

    std::vector<DemodulatorInstance *> demods;
    std::vector<DemodulatorInstance *> demods_deleted;
    DemodulatorInstance *activeDemodulator;
    DemodulatorInstance *lastActiveDemodulator;
    DemodulatorInstance *activeVisualDemodulator;

    int lastBandwidth;
    int lastDemodType;
    bool lastDemodLock;
    bool lastSquelchEnabled;
    float lastSquelch;
    float lastGain;
    bool lastStereo;
};
