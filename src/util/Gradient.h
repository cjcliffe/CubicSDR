#pragma once

#include <vector>

class GradientColor {
public:
    float r, g, b;
    float w;

    GradientColor(float r_in, float g_in, float b_in) :
            r(r_in), g(g_in), b(b_in), w(1) {
    }
};

class Gradient {
public:
    Gradient();

    void addColor(GradientColor c);

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
