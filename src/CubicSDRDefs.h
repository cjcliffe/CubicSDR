// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#if defined(__linux__) || defined(__FreeBSD__)
#include <sys/param.h>
#endif

#define CUBICSDR_TITLE "" CUBICSDR_BUILD_TITLE

#ifndef __BYTE_ORDER
    #ifdef _WIN32
        #define ATTRIBUTE
        #define __LITTLE_ENDIAN 1234
        #define __BIG_ENDIAN    4321
        #define __PDP_ENDIAN    3412
        #define __BYTE_ORDER __LITTLE_ENDIAN
    #else
        #ifdef __APPLE__
            #include <machine/endian.h>
        #else
	    #ifdef __FreeBSD__
		#include <sys/endian.h>
	    #else
		 #include <endian.h>
	    #endif
        #endif
    #endif
#endif

const char filePathSeparator =
#ifdef _WIN32
                            '\\';
#else
                            '/';
#endif

#define BUF_SIZE (16384*6)

#define DEFAULT_SAMPLE_RATE 2500000

//
#define DEFAULT_FFT_SIZE 2048
#define DEFAULT_DMOD_FFT_SIZE (DEFAULT_FFT_SIZE / 2)
#define DEFAULT_SCOPE_FFT_SIZE (DEFAULT_FFT_SIZE / 2)

//Both must be a power of 2 to prevent terrible OpenGL performance.
//TODO: Make the waterfall resolutions an option.
#define DEFAULT_MAIN_WATERFALL_LINES_NB 512 // 1024
#define DEFAULT_DEMOD_WATERFALL_LINES_NB 256

#define DEFAULT_DEMOD_TYPE "FM"
#define DEFAULT_DEMOD_BW 200000

#define DEFAULT_WATERFALL_LPS 30

//Dmod waterfall lines per second is adjusted 
//so that the whole demod waterfall show DEMOD_WATERFALL_DURATION_IN_SECONDS
//seconds.
#define DEMOD_WATERFALL_DURATION_IN_SECONDS 4.0

#define CHANNELIZER_RATE_MAX 500000

#define MANUAL_SAMPLE_RATE_MIN 2000000 // 2MHz
#define MANUAL_SAMPLE_RATE_MAX 200000000 // 200MHz (We are 2017+ after all)

//Represents the amount of time to process in the FFT distributor. 
#define FFT_DISTRIBUTOR_BUFFER_IN_SECONDS 0.250

//The maximum number of listed sample rates for a device, to be able to handle 
//devices returning an insane amount because they have quasi-continuous ranges (UHD...)
#define DEVICE_SAMPLE_RATES_MAX_NB     25
