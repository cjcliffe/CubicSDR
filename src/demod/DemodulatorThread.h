#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"
#include "Modem.h"

typedef ThreadQueue<AudioThreadInput *> DemodulatorThreadOutputQueue;

#define DEMOD_VIS_SIZE 2048

class DemodulatorThread : public IOThread {
public:

    DemodulatorThread();
    ~DemodulatorThread();

    void onBindOutput(std::string name, ThreadQueueBase *threadQueue);
    
    void run();
    void terminate();
    
    void setAGC(bool state);
    bool getAGC();

    void setMuted(bool state);
    bool isMuted();
    
    float getSignalLevel();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    void setDemodulatorType(std::string demod_type_in);
    std::string getDemodulatorType();
    
    void setDemodulatorLock(bool demod_lock_in);
    int getDemodulatorLock();
    
    void setDemodulatorCons(int demod_cons_in);
    int getDemodulatorCons();
//
//#ifdef __APPLE__
//    static void *pthread_helper(void *context) {
//        return ((DemodulatorThread *) context)->threadMain();
//    }
//#endif

protected:
    ReBuffer<AudioThreadInput> outputBuffers;

    std::vector<liquid_float_complex> agcData;
    std::vector<float> agcAMData;
//    std::vector<float> demodOutputData;
//    std::vector<float> demodStereoData;
//    std::vector<float> resampledOutputData;
//    std::vector<float> resampledStereoData;
	std::vector<unsigned int> demodOutputDataDigital;
	//std::vector<unsigned int> demodOutputDataDigitalTest;

	//std::vector<unsigned char> demodOutputSoftbits;
	//std::vector<unsigned char> demodOutputSoftbitsTest;

//    freqdem demodFM;
//    ampmodem demodAM;
//    ampmodem demodAM_DSB_CSP;
//    ampmodem demodAM_DSB;
//    ampmodem demodAM_LSB;
//    ampmodem demodAM_USB;
/*
    modem demodASK;    
    modem demodASK2;
    modem demodASK4;
    modem demodASK8;
    modem demodASK16;
    modem demodASK32;
    modem demodASK64;
    modem demodASK128;
    modem demodASK256;
                
    modem demodAPSK;
	modem demodAPSK4;
	modem demodAPSK8;
	modem demodAPSK16;
	modem demodAPSK32;
	modem demodAPSK64;
	modem demodAPSK128;
	modem demodAPSK256;

    modem demodBPSK;

    modem demodDPSK;
	modem demodDPSK2;
	modem demodDPSK4;
	modem demodDPSK8;
	modem demodDPSK16;
	modem demodDPSK32;
	modem demodDPSK64;
	modem demodDPSK128;
	modem demodDPSK256;

    modem demodPSK;
	modem demodPSK2;
	modem demodPSK4;
	modem demodPSK8;
	modem demodPSK16;
	modem demodPSK32;
	modem demodPSK64;
	modem demodPSK128;
	modem demodPSK256;

    modem demodOOK;

    modem demodSQAM;
	modem demodSQAM32;
	modem demodSQAM128;

    modem demodST;

    modem demodQAM;
	modem demodQAM4;
	modem demodQAM8;
	modem demodQAM16;
	modem demodQAM32;
	modem demodQAM64;
	modem demodQAM128;
	modem demodQAM256;

    modem demodQPSK;
*/
    agc_crcf iqAutoGain;

    float amOutputCeil;
    float amOutputCeilMA;
    float amOutputCeilMAA;

    std::atomic_bool muted;
    std::atomic_bool agcEnabled;
    std::string demodulatorType;
    std::atomic_int demodulatorCons;
    int audioSampleRate;

    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel;
    bool squelchEnabled;
    
    bool currentDemodLock;
	int currentDemodCons;

	void updateDemodulatorCons(int Cons);
    void updateDemodulatorLock(modem demod, float sensitivity);

    Modem *cModem;
    ModemKit *cModemKit;
    
    DemodulatorThreadPostInputQueue* iqInputQueue;
    AudioThreadInputQueue *audioOutputQueue;
    DemodulatorThreadOutputQueue* audioVisOutputQueue;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
    DemodulatorThreadCommandQueue* threadQueueNotify;
};
