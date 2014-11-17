#include "AudioThreadTask.h"


void AudioThreadTask::setData(std::vector<float> &data_in) {
    data = data_in;
}
std::vector<float> &AudioThreadTask::getData() {
    return data;
}
