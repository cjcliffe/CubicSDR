#pragma once

#ifdef __APPLE__
#define BUF_SIZE (16384*2)
#define SRATE 2000000
#else
#define BUF_SIZE (16384*4)
#define SRATE 2500000
#endif
#define FFT_SIZE 2048

#define DEFAULT_FREQ 98900000
#define AUDIO_FREQUENCY 44100
