// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "DataTree.h"
#include "AppFrame.h"


class SessionMgr {
public:
    void saveSession(std::string fileName);
    bool loadSession(std::string fileName);
};
