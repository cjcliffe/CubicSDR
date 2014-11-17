#include "DemodulatorThreadTask.h"

void DemodulatorThreadTask::setData(std::vector<signed char> &data_in) {
    data = data_in;
}
std::vector<signed char> &DemodulatorThreadTask::getData() {
    return data;
}
