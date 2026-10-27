// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "Frequency.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

std::string& filterChars(std::string& s, const std::string& allowed) {
    s.erase(remove_if(s.begin(), s.end(), [&allowed](const char& c) {
        return allowed.find(c) == std::string::npos;
    }), s.end());
    return s;
}

std::string frequencyToStr(long long freq) {
    long double freqTemp;
    
    freqTemp = freq;
    std::string suffix;
    std::stringstream freqStr;
    
    if (freqTemp >= 1.0e9) {
        freqTemp /= 1.0e9;
        freqStr << std::setprecision(10);
        suffix = std::string("GHz");
    } else if (freqTemp >= 1.0e6) {
        freqTemp /= 1.0e6;
        freqStr << std::setprecision(7);
        suffix = std::string("MHz");
    } else if (freqTemp >= 1.0e3) {
        freqTemp /= 1.0e3;
        freqStr << std::setprecision(4);
        suffix = std::string("kHz");
    }
    
    freqStr << freqTemp;
    freqStr << suffix;
    
    return freqStr.str();
}

long long strToFrequency(std::string freqStr, FrequencyUnit defaultUnit) {
    std::string filterStr = filterChars(freqStr, std::string("0123456789.MKGHmkgh"));
    
    size_t numLen = filterStr.find_first_not_of("0123456789.");
    
    if (numLen == std::string::npos) {
        numLen = freqStr.length();
    }
    
    std::string numPartStr = freqStr.substr(0, numLen);
    std::string suffixStr = freqStr.substr(numLen);
    
    std::stringstream numPartStream;
    numPartStream.str(numPartStr);
    
    long double freqTemp = 0;
    
    numPartStream >> freqTemp;
    
    if (suffixStr.length()) {
        if (suffixStr.find_first_of("Gg") != std::string::npos) {
            freqTemp *= 1.0e9;
        } else if (suffixStr.find_first_of("Mm") != std::string::npos) {
            freqTemp *= 1.0e6;
        } else if (suffixStr.find_first_of("Kk") != std::string::npos) {
            freqTemp *= 1.0e3;
        } else if (suffixStr.find_first_of("Hh") != std::string::npos) {
            // ...
        }
    } else {
        switch (defaultUnit) {
        case FrequencyUnit::Hz: break;
        case FrequencyUnit::kHz: freqTemp *= 1.0e3; break;
        case FrequencyUnit::MHz: freqTemp *= 1.0e6; break;
        case FrequencyUnit::GHz: freqTemp *= 1.0e9; break;
        case FrequencyUnit::Auto:
            if (numPartStr.find_first_of('.') != std::string::npos || freqTemp <= 3000) {
                freqTemp *= 1.0e6;
            }
            break;
        }
    }
    
    return (long long) freqTemp;
}
