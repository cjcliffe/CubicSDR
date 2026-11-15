// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cctype>
#include <string>

// Session files round-trip through TinyXML with whitespace condensing on, so
// a device name saved with runs of spaces or leading/trailing space loads
// back collapsed ("A  B " becomes "A B"). The live RtAudio enumeration keeps
// the original spacing, so an exact comparison misses and session restore
// silently falls back to the first device (GH#842). Match on the condensed
// form both sides reduce to.
inline std::string collapseAudioDeviceName(const std::string &name) {
    std::string collapsed;
    collapsed.reserve(name.size());
    bool pendingSpace = false;
    bool seenChar = false;
    for (char c : name) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            pendingSpace = seenChar;
        } else {
            if (pendingSpace) {
                collapsed.push_back(' ');
                pendingSpace = false;
            }
            collapsed.push_back(c);
            seenChar = true;
        }
    }
    return collapsed;
}

inline bool audioDeviceNameMatches(const std::string &saved, const std::string &live) {
    return saved == live || collapseAudioDeviceName(saved) == collapseAudioDeviceName(live);
}
