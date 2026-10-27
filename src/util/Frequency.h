// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>

enum class FrequencyUnit { Hz, kHz, MHz, GHz, Auto };

std::string& filterChars(std::string& s, const std::string& allowed);
std::string frequencyToStr(long long freq);
long long strToFrequency(std::string freqStr, FrequencyUnit defaultUnit = FrequencyUnit::MHz);
