// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

inline int utf8CharBytes(const char *p, const char *end) {
    unsigned char c = (unsigned char)*p;
    int bytes = 1;
    if ((c & 0x80) == 0) {
        bytes = 1;
    } else if ((c & 0xE0) == 0xC0) {
        bytes = 2;
    } else if ((c & 0xF0) == 0xE0) {
        bytes = 3;
    } else if ((c & 0xF8) == 0xF0) {
        bytes = 4;
    }
    if (p + bytes > end) {
        bytes = (int)(end - p);
    }
    return bytes;
}

inline unsigned int utf8Decode(const char *p, const char *end, int &bytes) {
    bytes = utf8CharBytes(p, end);
    unsigned int c = (unsigned char)p[0];
    if (bytes == 1) {
        return c;
    }
    if (bytes == 2) {
        return ((c & 0x1F) << 6) | ((unsigned char)p[1] & 0x3F);
    }
    if (bytes == 3) {
        return ((c & 0x0F) << 12) | (((unsigned char)p[1] & 0x3F) << 6) | ((unsigned char)p[2] & 0x3F);
    }
    return ((c & 0x07) << 18) | (((unsigned char)p[1] & 0x3F) << 12) | (((unsigned char)p[2] & 0x3F) << 6)
        | ((unsigned char)p[3] & 0x3F);
}
