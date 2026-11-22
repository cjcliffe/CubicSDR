// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>

std::wstring utf8ToWstring(const std::string &utf8);
std::string wstringToUtf8(const std::wstring &wide);
