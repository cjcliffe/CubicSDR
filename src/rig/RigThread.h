// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "IOThread.h"
#include "CubicSDR.h"
#include <hamlib/rig.h>
#include <hamlib/riglist.h>

#ifndef FILPATHLEN
	#define FILPATHLEN HAMLIB_FILPATHLEN
#endif

struct rigGreater
{
    bool operator()( const struct rig_caps *lx, const struct rig_caps *rx ) const {
        std::string ln(std::string(std::string(lx->mfg_name) + " " + std::string(lx->model_name)));
        std::string rn(std::string(std::string(rx->mfg_name) + " " + std::string(rx->model_name)));
    	return ln.compare(rn)<0;
    }
};

typedef std::vector<const struct rig_caps *> RigList;

class RigThread : public IOThread {
public:
    RigThread();
    ~RigThread() override;

    void initRig(rig_model_t rig_model, std::string rig_file, int serial_rate);
    void run() override;

    freq_t getFrequency();
    void setFrequency(freq_t new_freq, bool oneShot);
    
    void setControlMode(bool cMode);
    bool getControlMode();

    void setFollowMode(bool fMode);
    bool getFollowMode();

    void setCenterLock(bool cLock);
    bool getCenterLock();
    
    void setFollowModem(bool mFollow);
    bool getFollowModem();

    bool getErrorState();
    std::string getErrorMessage();

    static RigList &enumerate();
    static int add_hamlib_rig(const struct rig_caps *rc, void* f);

protected:
    void setErrorStateFromHamlibCode(int errcode);

private:
	RIG *rig;
    rig_model_t rigModel;
    std::string rigFile;
    int serialRate;
    int termStatus;
    freq_t freq;
    freq_t newFreq;
    std::atomic_bool freqChanged, setOneShot;
    std::atomic_bool controlMode, followMode, centerLock, followModem;
    std::atomic_bool errorState;
    std::string errorMessage;
    static RigList rigCaps;
};
