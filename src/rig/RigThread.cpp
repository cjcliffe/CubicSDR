#include "RigThread.h"

std::vector<const struct rig_caps *> RigThread::rigCaps;

RigThread::RigThread() {
    terminated.store(true);
    freq = wxGetApp().getFrequency();
    newFreq = freq;
    freqChanged.store(true);
    termStatus = 0;
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

int RigThread::add_hamlib_rig(const struct rig_caps *rc, void* f)
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
        terminated.store(true);
        return;
    }
    
	char *info_buf = (char *)rig_get_info(rig);

    if (info_buf) {
        std::cout << "Rig info: " << info_buf << std::endl;
    } else {
        std::cout << "Rig info was NULL." << std::endl;
    }
    
    while (!terminated.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        if (freqChanged.load()) {
            status = rig_get_freq(rig, RIG_VFO_CURR, &freq);
            if (status == 0) {
                if (freq != newFreq) {
                    freq = newFreq;
                    rig_set_freq(rig, RIG_VFO_CURR, freq);
    //                std::cout << "Set Rig Freq: %f" <<  newFreq << std::endl;
                }
                
                freqChanged.store(false);
            } else {
                termStatus = 0;
                terminate();
            }
        } else {
            freq_t checkFreq;

            status = rig_get_freq(rig, RIG_VFO_CURR, &checkFreq);
            
            if (status == 0) {
                if (checkFreq != freq) {
                    freq = checkFreq;
                    wxGetApp().setFrequency((long long)checkFreq);
                } else if (wxGetApp().getFrequency() != freq) {
                    freq = wxGetApp().getFrequency();
                    rig_set_freq(rig, RIG_VFO_CURR, freq);
                }
            } else {
                termStatus = 0;
                terminate();
            }
        }
        
//        std::cout <<  "Rig Freq: " << freq << std::endl;
    }
    
    rig_close(rig);
    rig_cleanup(rig);
    
    std::cout << "Rig thread exiting status " << termStatus << "." << std::endl;
};

freq_t RigThread::getFrequency() {
    if (freqChanged.load()) {
        return newFreq;
    } else {
        return freq;
    }
}

void RigThread::setFrequency(freq_t new_freq) {
    newFreq = new_freq;
    freqChanged.store(true);
}

