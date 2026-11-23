// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/FileUtil.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace {
std::string tempRoot() {
    const char *env = std::getenv("TEMP");
    if (!env) {
        env = std::getenv("TMPDIR");
    }
    std::string base = env ? env : ".";
    return FileUtil::joinPath(base, "cubicsdr_fileutil_test");
}

void removeDir(const std::string &path) {
#ifdef _WIN32
    _rmdir(path.c_str());
#else
    rmdir(path.c_str());
#endif
}
}

TEST_SUITE("FileUtil") {
    TEST_CASE("joinPath composes directory and file") {
        CHECK(FileUtil::joinPath("dir", "file.xml") == "dir/file.xml");
        CHECK(FileUtil::joinPath("dir/", "file.xml") == "dir/file.xml");
        CHECK(FileUtil::joinPath("dir\\", "file.xml") == "dir\\file.xml");
        CHECK(FileUtil::joinPath("", "file.xml") == "file.xml");
    }

    TEST_CASE("missing paths report absent") {
        std::string missing = FileUtil::joinPath(tempRoot(), "no_such_file.xml");
        CHECK(!FileUtil::fileExists(missing));
        CHECK(!FileUtil::isFileReadable(missing));
        CHECK(!FileUtil::isFileWritable(missing));
    }

    TEST_CASE("makeDir creates writable directories") {
        std::string root = tempRoot();
        removeDir(root);
        CHECK(FileUtil::makeDir(root));
        CHECK(FileUtil::dirExists(root));
        CHECK(FileUtil::makeDir(root));
        CHECK(!FileUtil::dirExists(FileUtil::joinPath(root, "nope")));
        CHECK(FileUtil::isDirWritable(FileUtil::joinPath(root, "f.xml")));
        CHECK(!FileUtil::isDirWritable(FileUtil::joinPath(FileUtil::joinPath(root, "nope"), "f.xml")));
        removeDir(root);
    }

    TEST_CASE("copyFile duplicates content and overwrites") {
        std::string root = tempRoot();
        FileUtil::makeDir(root);
        std::string src = FileUtil::joinPath(root, "src.xml");
        std::string dst = FileUtil::joinPath(root, "dst.xml");
        {
            std::ofstream out(src.c_str(), std::ios::binary);
            out << "payload";
        }
        CHECK(FileUtil::fileExists(src));
        CHECK(FileUtil::isFileReadable(src));
        CHECK(FileUtil::isFileWritable(src));
        {
            std::ofstream out(dst.c_str(), std::ios::binary);
            out << "stale";
        }
        CHECK(FileUtil::copyFile(src, dst));
        std::ifstream in(dst.c_str(), std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        CHECK(content == "payload");
        CHECK(!FileUtil::copyFile(FileUtil::joinPath(root, "missing.xml"), dst));
        std::remove(src.c_str());
        std::remove(dst.c_str());
        removeDir(root);
    }
}
