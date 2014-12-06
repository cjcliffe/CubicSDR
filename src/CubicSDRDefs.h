#pragma once

#ifdef __APPLE__
#define BUF_SIZE (16384*3)
#else
#define BUF_SIZE (16384*4)
#endif
#define SRATE 2000000
#define FFT_SIZE 2048

#define DEFAULT_FREQ 98900000
#define AUDIO_FREQUENCY 48000
