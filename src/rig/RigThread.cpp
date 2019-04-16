// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "RigThread.h"

std::vector<const struct rig_caps *> RigThread::rigCaps;

RigThread::RigThread() {
    freq = wxGetApp().getFrequency();
    newFreq = freq;
    freqChanged.store(true);
    termStatus = 0;
    controlMode.store(true);
    followMode.store(true);
    centerLock.store(false);
    followModem.store(false);
}

RigThread::~RigThread() {

}

RigList &RigThread::enumerate() {
    if (RigThread::rigCaps.empty()) {
        rig_set_debug(RIG_DEBUG_ERR);
        rig_load_all_backends();
        
        rig_list_foreach(RigThread::add_hamlib_rig, 0);
        std::sort(RigThread::rigCaps.begin(), RigThread::rigCaps.end(), rigGreater());
        std::cout << "Loaded " << RigThread::rigCaps.size() << " rig models via hamlib." << std::endl;
    }
    return RigThread::rigCaps;
}

int RigThread::add_hamlib_rig(const struct rig_caps *rc, void* /* f */)
{
    rigCaps.push_back(rc);
    return 1;
}

void RigThread::initRig(rig_model_t rig_model, std::string rig_file, int serial_rate) {
    rigModel = rig_model;
    rigFile = rig_file;
    serialRate = serial_rate;
};

void RigThread::run() {
    int retcode, status;

    termStatus = 0;
    
    std::cout << "Rig thread starting." << std::endl;

    rig = rig_init(rigModel);
	strncpy(rig->state.rigport.pathname, rigFile.c_str(), FILPATHLEN - 1);
	rig->state.rigport.parm.serial.rate = serialRate;
	retcode = rig_open(rig);
    
    if (retcode != 0) {
        std::cout << "Rig failed to init. " << std::endl;
        IOThread::terminate();
        return;
    }
    
	char *info_buf = (char *)rig_get_info(rig);

    if (info_buf) {
        std::cout << "Rig info: " << info_buf << std::endl;
    } else {
        std::cout << "Rig info was NULL." << std::endl;
    }
    
    while (!stopping) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        auto activeDemod = wxGetApp().getDemodMgr().getActiveContextModem();
        auto lastDemod = wxGetApp().getDemodMgr().getCurrentModem();

        if (freqChanged.load() && (controlMode.load() || setOneShot.load())) {
            status = rig_get_freq(rig, RIG_VFO_CURR, &freq);
            if (status == 0 && !stopping) {
                
                if (freq != newFreq && setOneShot.load()) {
                    freq = newFreq;
                    rig_set_freq(rig, RIG_VFO_CURR, freq);
    //                std::cout << "Set Rig Freq: %f" <<  newFreq << std::endl;
                }
                
                freqChanged.store(false);
                setOneShot.store(false);
            } else {
                termStatus = 0;
                break;
            }
        } else {
            freq_t checkFreq;
            
            status = rig_get_freq(rig, RIG_VFO_CURR, &checkFreq);

            if (status == 0 && !stopping) {
                if (checkFreq != freq && followMode.load()) {
                    freq = checkFreq;
                    if (followModem.load()) {
                        if (lastDemod) {
                            lastDemod->setFrequency(freq);
                            lastDemod->updateLabel(freq);
                            lastDemod->setFollow(true);
                        }
                    } else {
                        wxGetApp().setFrequency((long long)checkFreq);
                    }
                } else if (wxGetApp().getFrequency() != freq && controlMode.load() && !centerLock.load() && !followModem.load()) {
                    freq = wxGetApp().getFrequency();
                    rig_set_freq(rig, RIG_VFO_CURR, freq);
                } else if (followModem.load()) {
                    if (lastDemod) {
                        if (lastDemod->getFrequency() != freq) {
                            lastDemod->setFrequency(freq);
                            lastDemod->updateLabel(freq);
                            lastDemod->setFollow(true);
                        }
                    }
                }
            } else {
                termStatus = 0;
                break;
            }
        }
        
        if (!centerLock.load() && followModem.load() && wxGetApp().getFrequency() != freq && (lastDemod && lastDemod != activeDemod)) {
            wxGetApp().setFrequency((long long)freq);
        }
        
//        std::cout <<  "Rig Freq: " << freq << std::endl;
    }
    
    rig_close(rig);
    rig_cleanup(rig);
    
    std::cout << "Rig thread exiting status " << termStatus << "." << std::endl;
};

freq_t RigThread::getFrequency() {
    if (freqChanged.load() && (setOneShot.load() || controlMode.load())) {
        return newFreq;
    } else {
        return freq;
    }
}

void RigThread::setFrequency(freq_t new_freq, bool oneShot) {
    newFreq = new_freq;
    freqChanged.store(true);
    setOneShot.store(oneShot);
}

void RigThread::setControlMode(bool cMode) {
    controlMode.store(cMode);
}

bool RigThread::getControlMode() {
    return controlMode.load();
}

void RigThread::setFollowMode(bool fMode) {
    followMode.store(fMode);
}

bool RigThread::getFollowMode() {
    return followMode.load();
}

void RigThread::setCenterLock(bool cLock) {
    centerLock.store(cLock);
}

bool RigThread::getCenterLock() {
    return centerLock.load();
}

void RigThread::setFollowModem(bool mFollow) {
    followModem.store(mFollow);
}

bool RigThread::getFollowModem() {
    return followModem.load();
}
