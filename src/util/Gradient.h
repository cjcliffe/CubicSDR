// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

class GradientColor {
public:
    float r, g, b;
    float w;

    //should work with both float and double inputs
    GradientColor(double r_in, double g_in, double b_in) :
            r((float)r_in), g((float)g_in), b((float)b_in), w(1) {
    }
};

class Gradient {
public:
    Gradient();

    void addColor(GradientColor c);

	void clear();

	void addColors(const std::vector<GradientColor>& color_list);

    std::vector<float> &getRed();
    std::vector<float> &getGreen();
    std::vector<float> &getBlue();

    void generate(unsigned int len);

    ~Gradient();
private:
    std::vector<GradientColor> colors;
    std::vector<float> r_val;
    std::vector<float> g_val;
    std::vector<float> b_val;
};
