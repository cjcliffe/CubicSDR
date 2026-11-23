// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AppServices.h"

#include <cassert>

namespace {
AppServices *appServicesInstance = nullptr;
}

void SetAppServices(AppServices *services) {
    appServicesInstance = services;
}

AppServices &GetAppServices() {
    assert(appServicesInstance != nullptr);
    return *appServicesInstance;
}
