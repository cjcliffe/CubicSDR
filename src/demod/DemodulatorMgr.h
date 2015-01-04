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

private:
    void garbageCollect();

    std::vector<DemodulatorInstance *> demods;
    std::vector<DemodulatorInstance *> demods_deleted;
    DemodulatorInstance *activeDemodulator;
    DemodulatorInstance *lastActiveDemodulator;
    DemodulatorInstance *activeVisualDemodulator;

};
