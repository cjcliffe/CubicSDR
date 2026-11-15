// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <cstdio>
#include <string>
#include <vector>

#include "doctest.h"
#include "demod/AudioDeviceMatch.h"
#include "DataTree.h"

TEST_SUITE("AudioDeviceMatch") {
    TEST_CASE("collapse leaves plain names alone") {
        CHECK(collapseAudioDeviceName("UM2") == "UM2");
        CHECK(collapseAudioDeviceName("Apple Inc.: Built-in Output") == "Apple Inc.: Built-in Output");
        CHECK(collapseAudioDeviceName("") == "");
    }

    TEST_CASE("collapse folds whitespace runs and trims ends") {
        CHECK(collapseAudioDeviceName("A  B") == "A B");
        CHECK(collapseAudioDeviceName("  A B  ") == "A B");
        CHECK(collapseAudioDeviceName("A\tB\nC") == "A B C");
    }

    TEST_CASE("collapse preserves non-ASCII bytes") {
        const std::string in = "Haut-parleurs (Périphérique Audio)";
        CHECK(collapseAudioDeviceName(in) == in);
    }

    TEST_CASE("exact names match") {
        CHECK(audioDeviceNameMatches("UM2", "UM2"));
        CHECK_FALSE(audioDeviceNameMatches("UM2", "UM4"));
        CHECK_FALSE(audioDeviceNameMatches("", "UM2"));
    }

    TEST_CASE("condensed save matches spaced live name") {
        const std::string live = "Burr-Brown from TI              : USB Audio CODEC ";
        const std::string saved = "Burr-Brown from TI : USB Audio CODEC";
        CHECK(audioDeviceNameMatches(saved, live));
        CHECK(audioDeviceNameMatches(live, saved));
    }

    TEST_CASE("session round-trip still matches the live device") {
        const std::string path = "audiodevicematch_test_tmp.xml";
        std::remove(path.c_str());

        const std::string live = "Burr-Brown from TI              : USB Audio CODEC ";
        std::string loaded;
        {
            DataTree tree("config");
            *tree.rootNode()->newChild("output_device") = live;
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree fromFile;
            REQUIRE(fromFile.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            fromFile.rootNode()->findAll("output_device", found);
            REQUIRE(found.size() == 1);
            loaded = static_cast<std::string>(*found[0]);
        }
        CHECK(loaded == collapseAudioDeviceName(live));
        CHECK(audioDeviceNameMatches(loaded, live));

        std::remove(path.c_str());
    }
}
