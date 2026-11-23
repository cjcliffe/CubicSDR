// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>

namespace FileUtil {
std::string joinPath(const std::string &dir, const std::string &file);
bool fileExists(const std::string &path);
bool isFileReadable(const std::string &path);
bool isFileWritable(const std::string &path);
bool isDirWritable(const std::string &filePath);
bool copyFile(const std::string &src, const std::string &dst);
bool dirExists(const std::string &path);
bool makeDir(const std::string &path);
}
