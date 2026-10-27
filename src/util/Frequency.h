// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>

// Suffixless-input default for strToFrequency. Auto keeps the legacy rule (decimal point or value at or below 3000 means MHz), so hand-typed full-Hz values still parse as Hz.
enum class FrequencyUnit { Hz, kHz, MHz, GHz, Auto };

std::string& filterChars(std::string& s, const std::string& allowed);
std::string frequencyToStr(long long freq);
// An explicit suffix always wins over defaultUnit.
long long strToFrequency(std::string freqStr, FrequencyUnit defaultUnit = FrequencyUnit::MHz);
