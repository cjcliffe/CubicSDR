// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "StringUtil.h"

#include <codecvt>
#include <locale>

std::wstring utf8ToWstring(const std::string &utf8) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    return convert.from_bytes(utf8);
}

std::string wstringToUtf8(const std::wstring &wide) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    return convert.to_bytes(wide);
}
