// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/StringUtil.h"

TEST_SUITE("StringUtil") {
    TEST_CASE("empty strings round-trip") {
        CHECK(utf8ToWstring("") == L"");
        CHECK(wstringToUtf8(L"") == "");
    }

    TEST_CASE("ascii passes through unchanged") {
        CHECK(utf8ToWstring("hello") == L"hello");
        CHECK(wstringToUtf8(L"hello") == "hello");
    }

    TEST_CASE("multibyte utf-8 decodes to wide") {
        CHECK(utf8ToWstring("h\xc3\xa9llo") == L"h\x00e9llo");
        CHECK(utf8ToWstring("\xe2\x82\xac") == L"\x20ac");
    }

    TEST_CASE("wide encodes to multibyte utf-8") {
        CHECK(wstringToUtf8(L"h\x00e9llo") == "h\xc3\xa9llo");
        CHECK(wstringToUtf8(L"\x2603") == "\xe2\x98\x83");
    }

    TEST_CASE("multibyte round-trips both directions") {
        std::string utf8("h\xc3\xa9llo \xe2\x98\x83");
        CHECK(wstringToUtf8(utf8ToWstring(utf8)) == utf8);
        std::wstring wide(L"h\x00e9llo \x2603");
        CHECK(utf8ToWstring(wstringToUtf8(wide)) == wide);
    }
}
