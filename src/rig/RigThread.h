#pragma once

#include "IOThread.h"
#include "CubicSDR.h"
#include <hamlib/rig.h>
#include <hamlib/riglist.h>

struct rigGreater
{
    bool operator()( const struct rig_caps *lx, const struct rig_caps *rx ) const {
        std::string ln(std::string(std::string(lx->mfg_name) + " " + std::string(lx->model_name)));
        std::string rn(std::string(std::string(rx->mfg_name) + " " + std::string(rx->model_name)));
    	return ln.compare(rn)<0;
    }
};

class RigThread : public IOThread {
public:
    RigThread();
    ~RigThread();

    void initRig(rig_model_t rig_model, std::string rig_file, int serial_rate);
    void run();
    
    freq_t getFrequency();
    void setFrequency(freq_t new_freq);
    
    static void enumerate();
    static int add_hamlib_rig(const struct rig_caps *rc, void* f);
    
private:
	RIG *rig;
    rig_model_t rigModel;
    std::string rigFile;
    int serialRate;
    
    freq_t freq;
    freq_t newFreq;
    std::atomic_bool freqChanged;
    static std::vector<const struct rig_caps *> rigCaps;
};