// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "FileUtil.h"

#include <cstdio>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

namespace FileUtil {

#ifdef _WIN32
constexpr int AccessRead = 4;
constexpr int AccessWrite = 2;
#endif

std::string joinPath(const std::string &dir, const std::string &file) {
    if (dir.empty()) {
        return file;
    }
    char last = dir.back();
    if (last == '/' || last == '\\') {
        return dir + file;
    }
    return dir + "/" + file;
}

bool fileExists(const std::string &path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
}

bool isFileReadable(const std::string &path) {
#ifdef _WIN32
    return _access(path.c_str(), AccessRead) == 0;
#else
    return ::access(path.c_str(), R_OK) == 0;
#endif
}

bool isFileWritable(const std::string &path) {
#ifdef _WIN32
    return _access(path.c_str(), AccessWrite) == 0;
#else
    return ::access(path.c_str(), W_OK) == 0;
#endif
}

bool isDirWritable(const std::string &filePath) {
    size_t pos = filePath.find_last_of("/\\");
    std::string dir = ".";
    if (pos != std::string::npos && pos > 0) {
        dir = filePath.substr(0, pos);
    }
#ifdef _WIN32
    return _access(dir.c_str(), AccessWrite) == 0;
#else
    return ::access(dir.c_str(), W_OK) == 0;
#endif
}

bool copyFile(const std::string &src, const std::string &dst) {
    std::ifstream in(src.c_str(), std::ios::binary);
    if (!in) {
        return false;
    }
    std::ofstream out(dst.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << in.rdbuf();
    out.flush();
    return static_cast<bool>(out);
}

bool dirExists(const std::string &path) {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) {
        return false;
    }
#ifdef _WIN32
    return (st.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(st.st_mode);
#endif
}

bool makeDir(const std::string &path) {
#ifdef _WIN32
    if (_mkdir(path.c_str()) == 0) {
        return true;
    }
#else
    if (::mkdir(path.c_str(), 0755) == 0) {
        return true;
    }
#endif
    return dirExists(path);
}

}
