#include "SpectrumVisualProcessor.h"

void SpectrumVisualProcessor::process() {
        /*
         std::vector<liquid_float_complex> *data = &input->data;
         if (data && data->size()) {
         if (fft_size != data->size()) {
         setup(data->size());
         }
         if (spectrum_points.size() < fft_size * 2) {
         if (spectrum_points.capacity() < fft_size * 2) {
         spectrum_points.reserve(fft_size * 2);
         }
         spectrum_points.resize(fft_size * 2);
         }
         
         for (int i = 0; i < fft_size; i++) {
         in[i][0] = (*data)[i].real;
         in[i][1] = (*data)[i].imag;
         }
         
         fftwf_execute(plan);
         
         float fft_ceil = 0, fft_floor = 1;
         
         if (fft_result.size() != fft_size) {
         if (fft_result.capacity() < fft_size) {
         fft_result.reserve(fft_size);
         fft_result_ma.reserve(fft_size);
         fft_result_maa.reserve(fft_size);
         }
         fft_result.resize(fft_size);
         fft_result_ma.resize(fft_size);
         fft_result_maa.resize(fft_size);
         }
         
         int n;
         for (int i = 0, iMax = fft_size / 2; i < iMax; i++) {
         float a = out[i][0];
         float b = out[i][1];
         float c = sqrt(a * a + b * b);
         
         float x = out[fft_size / 2 + i][0];
         float y = out[fft_size / 2 + i][1];
         float z = sqrt(x * x + y * y);
         
         fft_result[i] = (z);
         fft_result[fft_size / 2 + i] = (c);
         }
         
         for (int i = 0, iMax = fft_size; i < iMax; i++) {
         fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.65;
         fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.65;
         
         if (fft_result_maa[i] > fft_ceil) {
         fft_ceil = fft_result_maa[i];
         }
         if (fft_result_maa[i] < fft_floor) {
         fft_floor = fft_result_maa[i];
         }
         }
         
         fft_ceil += 1;
         fft_floor -= 1;
         
         fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.01;
         fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.01;
         
         fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.01;
         fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.01;
         
         for (int i = 0, iMax = fft_size; i < iMax; i++) {
         float v = (log10(fft_result_maa[i] - fft_floor_maa) / log10(fft_ceil_maa - fft_floor_maa));
         spectrum_points[i * 2] = ((float) i / (float) iMax);
         spectrum_points[i * 2 + 1] = v;
         }
         
         }
         */
}