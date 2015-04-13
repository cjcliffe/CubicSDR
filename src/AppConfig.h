#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include "DataTree.h"

class AppConfig {
public:

    std::string getConfigDir();

    void setPPM(std::string device_serial, int ppm);
    int getPPM(std::string device_serial);
    bool save();
    bool load();
    bool reset();
private:
    std::map<std::string, int> device_ppm;
};
