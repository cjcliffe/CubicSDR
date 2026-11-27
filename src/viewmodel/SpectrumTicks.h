// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Pure geometry for the spectrum frequency axis, dB readouts, grid bands, and fill.
// The step ladder, tick loop, and label formatting replicate the panel code verbatim,
// except the minimum tick gap, which is tuned wider so labels keep breathing room;
// the dB labels share one stream, so both inherit fixed one-decimal format.

#pragma once

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace SpectrumTicks {

struct AxisTick {
    double x = 0.0;
    std::string label;
    bool major = false;
};

struct AxisGeometry {
    std::vector<AxisTick> ticks;
    double hPos = 0.0;
    double tickBase = 0.0;
    int fontSize = 12;
    double visualStep = 0.1;
    int precision = 1;
    bool empty() const {
        return ticks.empty();
    }
};

struct DbLabels {
    std::string ceil;
    std::string floor;
};

inline AxisGeometry axisGeometry(long long freqHz, long long bandwidthHz, double viewWidthPx, double viewHeightPx,
        double fontScale) {
    AxisGeometry geo;
    long long leftFreq = (double)freqHz - ((double)bandwidthHz / 2.0);
    long long rightFreq = leftFreq + (double)bandwidthHz;
    long long spanHz = rightFreq - leftFreq;
    if (spanHz <= 0 || viewWidthPx <= 0.0 || viewHeightPx <= 0.0) {
        return geo;
    }

    long double mhzStep = (100000.0 / (long double)spanHz) * 2.0;
    double mhzVisualStep = 0.1;
    int precision = 1;

    double minGapPx = 56 * fontScale;
    if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
        mhzStep = (250000.0 / (long double)spanHz) * 2.0;
        mhzVisualStep = 0.25;

        if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
            mhzStep = (500000.0 / (long double)spanHz) * 2.0;
            mhzVisualStep = 0.5;
        }

        if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
            mhzStep = (1000000.0 / (long double)spanHz) * 2.0;
            mhzVisualStep = 1.0;
        }

        if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
            mhzStep = (2500000.0 / (long double)spanHz) * 2.0;
            mhzVisualStep = 2.5;
        }

        if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
            mhzStep = (5000000.0 / (long double)spanHz) * 2.0;
            mhzVisualStep = 5.0;
        }

        if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
            mhzStep = (10000000.0 / (long double)spanHz) * 2.0;
            mhzVisualStep = 10.0;
        }

        if (mhzStep * 0.5 * viewWidthPx < minGapPx) {
            mhzStep = (50000000.0 / (long double)spanHz) * 2.0;
            mhzVisualStep = 50.0;
        }
    } else if (mhzStep * 0.5 * viewWidthPx > 10.0 * minGapPx) {
        mhzStep = (10000.0 / (long double)spanHz) * 2.0;
        mhzVisualStep = 0.01;
        precision = 2;
    }

    long long hzStep = 1000000;
    long long firstMhz = (leftFreq / hzStep) * hzStep;
    long double mhzStart = ((long double)(firstMhz - leftFreq) / (long double)spanHz) * 2.0;
    long double currentMhz = trunc(floor(firstMhz / (long double)1000000.0));

    geo.hPos = 1.0 - (16.0 / viewHeightPx) * fontScale;
    geo.tickBase = 1.0 - (5.0 / viewHeightPx);
    geo.fontSize = 12;

    if (viewHeightPx > 135) {
        geo.fontSize = 16;
        geo.hPos = 1.0 - (18.0 / viewHeightPx) * fontScale;
    }
    geo.visualStep = mhzVisualStep;
    geo.precision = precision;

    std::stringstream label;
    label.precision(precision);

    for (double m = -1.0 + mhzStart, mMax = 1.0 + ((mhzStart > 0) ? mhzStart : -mhzStart); m <= mMax; m += mhzStep) {
        if (m < -1.0) {
            currentMhz += mhzVisualStep;
            continue;
        }
        if (m > 1.0) {
            break;
        }
        if (currentMhz < -1e-9) {
            currentMhz += mhzVisualStep;
            continue;
        }
        if (currentMhz < 1e-9) {
            currentMhz = 0.0;
        }
        label << std::fixed << currentMhz;

        double fractpart, intpart;
        fractpart = modf(currentMhz, &intpart);

        AxisTick tick;
        tick.x = m;
        tick.label = label.str();
        tick.major = (fractpart < 0.001);
        geo.ticks.push_back(tick);

        label.str(std::string());

        currentMhz += mhzVisualStep;
    }

    return geo;
}

inline DbLabels dbLabels(double ceilValue, double floorValue, int fftSize, double dbOfs) {
    DbLabels labels;
    std::stringstream ssLabel("");
    if (ceilValue != floorValue && fftSize) {
        ssLabel << std::fixed << std::setprecision(1) << (dbOfs + 20.0 * log10(2.0 * ceilValue / (double)fftSize)) << "dB";
    }
    labels.ceil = ssLabel.str();

    ssLabel.str("");
    if (ceilValue != floorValue && fftSize) {
        ssLabel << (dbOfs + 20.0 * log10(2.0 * floorValue / (double)fftSize)) << "dB";
    }
    labels.floor = ssLabel.str();
    return labels;
}

struct GridBand {
    double alpha = 1.0;
    std::vector<float> verts;
};

struct GridGeometry {
    std::vector<GridBand> bands;
    bool empty() const {
        return bands.empty();
    }
};

inline GridGeometry gridGeometry(float floorValue, float ceilValue) {
    GridGeometry geo;
    double range = ceilValue - floorValue;
    double ranges[3][4] = { { 90.0, 5000.0, 10.0, 100.0 }, { 20.0, 150.0, 10.0, 10.0 }, { -20.0, 30.0, 10.0, 1.0 } };
    for (auto & i : ranges) {
        double rangeMin = i[0];
        double rangeMax = i[1];
        double rangeTrans = i[2];
        double rangeStep = i[3];

        if (range >= rangeMin && range <= rangeMax) {
            GridBand band;
            double p = 0;
            double a = 1.0;

            if (range <= rangeMin + rangeTrans) {
                a *= (range - rangeMin) / rangeTrans;
            }
            if (range >= rangeMax - rangeTrans) {
                a *= (rangeTrans - (range - (rangeMax - rangeTrans))) / rangeTrans;
            }
            band.alpha = a;

            for (double l = floorValue; l <= ceilValue + rangeStep; l += rangeStep) {
                p += rangeStep / range;
                band.verts.push_back(0.0f);
                band.verts.push_back((float)p);
                band.verts.push_back(1.0f);
                band.verts.push_back((float)p);
            }
            geo.bands.push_back(band);
        }
    }
    return geo;
}


inline std::vector<float> fillStrip(const std::vector<float> &points) {
    std::vector<float> fill;
    fill.resize(points.size() * 2);
    for (size_t i = 0, n = points.size() / 2; i < n; i++) {
        fill[i * 4] = points[i * 2];
        fill[i * 4 + 1] = points[i * 2 + 1];
        fill[i * 4 + 2] = points[i * 2];
        fill[i * 4 + 3] = 0.0f;
    }
    return fill;
}
}
