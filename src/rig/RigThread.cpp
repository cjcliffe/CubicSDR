#include "RigThread.h"

std::vector<const struct rig_caps *> RigThread::rigCaps;

RigThread::RigThread() {
    terminated.store(true);
}

RigThread::~RigThread() {

}

void RigThread::enumerate() {
    rig_set_debug(RIG_DEBUG_ERR);
    rig_load_all_backends();
    RigThread::rigCaps.clear();
    rig_list_foreach(RigThread::add_hamlib_rig, 0);
    std::sort(RigThread::rigCaps.begin(), RigThread::rigCaps.end(), rigGreater());
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

    std::cout << "Rig thread starting." << std::endl;

    rig = rig_init(rigModel);
	strncpy(rig->state.rigport.pathname, rigFile.c_str(), FILPATHLEN - 1);
	rig->state.rigport.parm.serial.rate = serialRate;
	retcode = rig_open(rig);
	char *info_buf = (char *)rig_get_info(rig);
    std::cout << "Rig info: " << info_buf << std::endl;
    
    while (!terminated.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        if (freqChanged.load()) {
            status = rig_get_freq(rig, RIG_VFO_CURR, &freq);
            if (freq != newFreq) {
                freq = newFreq;
                rig_set_freq(rig, RIG_VFO_CURR, freq);
                std::cout << "Set Rig Freq: %f" <<  newFreq << std::endl;
            }
            
            freqChanged.store(false);
        } else {
            status = rig_get_freq(rig, RIG_VFO_CURR, &freq);
        }
        
        std::cout <<  "Rig Freq: " << freq << std::endl;
    }
    
    rig_close(rig);
    
    std::cout << "Rig thread exiting." << std::endl;
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

