#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include "DataTree.h"

class AppConfig {
public:

    std::string getConfigDir();

    void setPPM(std::string deviceId, int ppm);
    int getPPM(std::string deviceId);
    bool save();
    bool load();
    bool reset();
private:
    std::map<std::string, int> device_ppm;
};
