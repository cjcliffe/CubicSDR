#include "DemodulatorThreadTask.h"

void DemodulatorThreadTask::setData(std::vector<unsigned char> &data_in) {
    data = data_in;
}
std::vector<unsigned char> &DemodulatorThreadTask::getData() {
    return data;
}
