// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "Gradient.h"
#include <stddef.h>

Gradient::Gradient() {
	//nothing
}

void Gradient::clear() {
	colors.clear();
}

void Gradient::addColor(GradientColor c) {
    colors.push_back(c);
}

void Gradient::addColors(const std::vector<GradientColor>& color_list) {

	for (auto single_color : color_list) {

		colors.push_back(single_color);
	}
}

std::vector<float> &Gradient::getRed() {
    return r_val;
}

std::vector<float> &Gradient::getGreen() {
    return g_val;
}

std::vector<float> &Gradient::getBlue() {
    return b_val;
}

void Gradient::generate(unsigned int len) {
    size_t chunk_size = len / (colors.size() - 1);

    size_t p = 0;
    r_val.resize(len);
    g_val.resize(len);
    b_val.resize(len);

    for (size_t j = 0, jMax = colors.size() - 1; j < jMax; j++) {
        if ((chunk_size * (jMax)) < len && (j == jMax - 1)) {
            chunk_size += len - chunk_size * (jMax);
        }

        for (size_t i = 0; i < chunk_size; i++) {
            float idx = (float) (i) / (float) chunk_size;

            float r1 = colors[j].r;
            float g1 = colors[j].g;
            float b1 = colors[j].b;

            float r2 = colors[j + 1].r;
            float g2 = colors[j + 1].g;
            float b2 = colors[j + 1].b;

            float r = r1 + (r2 - r1) * idx;
            float g = g1 + (g2 - g1) * idx;
            float b = b1 + (b2 - b1) * idx;

            if (r < 0.0)
                r = 0.0;
            if (r > 1.0)
                r = 1.0;
            if (g < 0.0)
                g = 0.0;
            if (g > 1.0)
                g = 1.0;
            if (b < 0.0)
                b = 0.0;
            if (b > 1.0)
                b = 1.0;

            r_val[p] = r;
            g_val[p] = g;
            b_val[p] = b;

            p++;
        }
    }
}

Gradient::~Gradient() {

}
