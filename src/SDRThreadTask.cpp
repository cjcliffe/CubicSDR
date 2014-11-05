#include "SDRThreadTask.h"

void SDRThreadTask::setUInt(unsigned int i) {
    arg_int = i;
}

unsigned int SDRThreadTask::getUInt() {
    return arg_int;
}
