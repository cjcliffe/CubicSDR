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
    errorState.store(false);
    errorMessage = "";
}

RigThread::~RigThread() = default;

RigList &RigThread::enumerate() {
    if (RigThread::rigCaps.empty()) {
        rig_set_debug(RIG_DEBUG_ERR);
        rig_load_all_backends();
        
        rig_list_foreach(RigThread::add_hamlib_rig, nullptr);
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
}

void RigThread::setErrorStateFromHamlibCode(int errcode) {
    errorState.store(true);
    switch (abs(errcode)) {
        case RIG_EINVAL:
            errorMessage = "Invalid parameter specified.";
            break; /*!< invalid parameter */
        case RIG_ECONF:
            errorMessage = "Invalid configuration i.e. serial, etc.";
            break; /*!< invalid configuration (serial,..) */
        case RIG_ENOMEM:
            errorMessage = "Out of memory.";
            break; /*!< memory shortage */
        case RIG_ENIMPL:
            errorMessage = "Function Not Yet Implemented.";
            break; /*!< function not implemented, but will be */
        case RIG_ETIMEOUT:
            errorMessage = "Communication timed out.";
            break; /*!< communication timed out */
        case RIG_EIO:
            errorMessage = "I/O error (Open failed?)";
            break; /*!< IO error, including open failed */
        case RIG_EINTERNAL:
            errorMessage = "Internal hamlib error :(";
            break; /*!< Internal Hamlib error, huh! */
        case RIG_EPROTO:
            errorMessage = "Protocol Error.";
            break; /*!< Protocol error */
        case RIG_ERJCTED:
            errorMessage = "Command rejected by rig.";
            break; /*!< Command rejected by the rig */
        case RIG_ETRUNC:
            errorMessage = "Argument truncated.";
            break; /*!< Command performed, but arg truncated */
        case RIG_ENAVAIL:
            errorMessage = "Function not available.";
            break; /*!< function not available */
        case RIG_ENTARGET:
            errorMessage = "VFO not targetable.";
            break; /*!< VFO not targetable */
        case RIG_BUSERROR:
            errorMessage = "Error talking on the bus.";
            break; /*!< Error talking on the bus */
        case RIG_BUSBUSY:
            errorMessage = "Collision on the bus.";
            break; /*!< Collision on the bus */
        case RIG_EARG:
            errorMessage = "Invalid rig handle.";
            break; /*!< NULL RIG handle or any invalid pointer parameter in get arg */
        case RIG_EVFO:
            errorMessage = "Invalid VFO.";
            break; /*!< Invalid VFO */
        case RIG_EDOM:
            errorMessage = "Argument out of domain of function.";
            break; /*!< Argument out of domain of func */
    }

    std::cout << "Rig error: " << errorMessage << std::endl;
}

void RigThread::run() {
    int retcode, status;

    termStatus = 0;
    errorState.store(false);
    
    std::cout << "Rig thread starting." << std::endl;

    rig = rig_init(rigModel);
	strncpy(rig->state.rigport.pathname, rigFile.c_str(), FILPATHLEN - 1);
	rig->state.rigport.parm.serial.rate = serialRate;
	retcode = rig_open(rig);
    
    if (retcode != 0) {
        setErrorStateFromHamlibCode(retcode);
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
                if (status == -RIG_ENIMPL) {
                    std::cout << "Rig does not support rig_get_freq?" << std::endl;
                } else {
                    termStatus = status;
                    break;
                }
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
                    status = rig_set_freq(rig, RIG_VFO_CURR, freq);
                    if (status == -RIG_ENIMPL) {
                        std::cout << "Rig does not support rig_set_freq?" << std::endl;
                    } else if (status != 0) {
                        termStatus = status;
                        break;
                    }
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
                if (status == -RIG_ENIMPL) {
                    std::cout << "Rig does not support rig_get_freq?" << std::endl;
                } else {
                    termStatus = status;
                    break;
                }
            }
        }
        
        if (!centerLock.load() && followModem.load() && wxGetApp().getFrequency() != freq && (lastDemod && lastDemod != activeDemod)) {
            wxGetApp().setFrequency((long long)freq);
        }
        
//        std::cout <<  "Rig Freq: " << freq << std::endl;
    }
    
    rig_close(rig);
    rig_cleanup(rig);

    if (termStatus != 0) {
        setErrorStateFromHamlibCode(termStatus);
    }

    std::cout << "Rig thread exiting status " << termStatus << "." << std::endl;
}

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


bool RigThread::getErrorState() {
    return errorState.load();
}

std::string RigThread::getErrorMessage() {
    return errorMessage;
}