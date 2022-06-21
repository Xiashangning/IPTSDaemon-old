/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "stylus-manager.hpp"
#include "devices.hpp"

// calibration parameters
#define IPTS_DFT_POSITION_MIN_AMP 50
#define IPTS_DFT_POSITION_MIN_MAG 2000
#define IPTS_DFT_BUTTON_MIN_MAG   1000
#define IPTS_DFT_FREQ_MIN_MAG     10000
#define IPTS_DFT_POSITION_EXP     -.7 // tune this value to minimize jagginess of diagonal lines

namespace iptsd::daemon {

static Float64 dft_interpolate_position(const struct IPTSStylusDFTWindowRow &r)
{
    // assume the center component has the max amplitude
    unsigned maxi = IPTS_DFT_NUM_COMPONENTS/2;

    // off-screen components are always zero, don't use them
    Float64 mind = -.5, maxd = .5;
    if      (r.real[maxi-1] == 0 && r.imag[maxi-1] == 0) { maxi++; mind = -1; }
    else if (r.real[maxi+1] == 0 && r.imag[maxi+1] == 0) { maxi--; maxd =  1; }

    // get phase-aligned amplitudes of the three center components
    Float64 amp = sqrt(r.real[maxi]*r.real[maxi] + r.imag[maxi]*r.imag[maxi]);
    if (amp < IPTS_DFT_POSITION_MIN_AMP) return NAN;
    Float64 sin = r.real[maxi] / amp, cos = r.imag[maxi] / amp;
    Float64 x[] = {
        sin * r.real[maxi-1] + cos * r.imag[maxi-1],
        amp,
        sin * r.real[maxi+1] + cos * r.imag[maxi+1],
    };

    // convert the amplitudes into something we can fit a parabola to
    for (unsigned i = 0; i < 3; i++) x[i] = pow(x[i], IPTS_DFT_POSITION_EXP);

    // check orientation of fitted parabola
    if (x[0] + x[2] <= 2*x[1]) return NAN;

    // find critical point of fitted parabola
    Float64 d = (x[0] - x[2]) / (2 * (x[0] - 2*x[1] + x[2]));

    return r.first + maxi + std::clamp(d, mind, maxd);
}

static Float64 dft_interpolate_frequency(const struct IPTSStylusDFTWindowRow *x, const struct IPTSStylusDFTWindowRow *y, unsigned n)
{
    if (n < 3) return NAN;

    // find max row
    unsigned maxi = 0, maxm = 0;
    for (unsigned i = 0; i < n; i++) {
        unsigned m = x[i].magnitude + y[i].magnitude;
        if (m > maxm) { maxm = m; maxi = i; }
    }
    if (maxm < 2*IPTS_DFT_FREQ_MIN_MAG) return NAN;

    Float64 mind = -.5, maxd = .5;
    if      (maxi <   1) { maxi =   1; mind = -1; }
    else if (maxi > n-2) { maxi = n-2; maxd =  1; }

    // all components in a row have the same phase, and corresponding x and y rows also have the same phase, so we can add everything together
    int real[3], imag[3];
    for (unsigned i = 0; i < 3; i++) {
        real[i] = imag[i] = 0;
        for (unsigned j = 0; j < IPTS_DFT_NUM_COMPONENTS; j++) {
            real[i] += x[maxi+i-1].real[j] + y[maxi+i-1].real[j];
            imag[i] += x[maxi+i-1].imag[j] + y[maxi+i-1].imag[j];
        }
    }

    // interpolate using Eric Jacobsen's modified quadratic estimator
    int ra = real[0] - real[2], rb = 2*real[1] - real[0] - real[2];
    int ia = imag[0] - imag[2], ib = 2*imag[1] - imag[0] - imag[2];
    Float64 d = (ra*rb + ia*ib) / (Float64)(rb*rb + ib*ib);

    return maxi + std::clamp(d, mind, maxd);
}

StylusInput *StylusManager::stop_stylus()
{
    if (input.proximity) {
        input.proximity = false;
        input.contact = false;
        input.button = false;
        input.rubber = false;
        input.pressure = 0;
        return &input;
    }
    return nullptr;
}

StylusInput *StylusManager::process(const StylusDFTData &data)
{
    if (set_rubber) {
        input.rubber = rubber;
        set_rubber = false;
    }
    Float64 p;
    switch (data.type) {
    case IPTS_DFT_ID_POSITION:
        if (data.num_cols && data.num_rows && data.dft_x[0].magnitude > IPTS_DFT_POSITION_MIN_MAG && data.dft_y[0].magnitude > IPTS_DFT_POSITION_MIN_MAG) {
            real = data.dft_x[0].real[IPTS_DFT_NUM_COMPONENTS/2] + data.dft_y[0].real[IPTS_DFT_NUM_COMPONENTS/2];
            imag = data.dft_x[0].imag[IPTS_DFT_NUM_COMPONENTS/2] + data.dft_y[0].imag[IPTS_DFT_NUM_COMPONENTS/2];
            Float64 x = dft_interpolate_position(data.dft_x[0]);
            Float64 y = dft_interpolate_position(data.dft_y[0]);
            bool prox = !std::isnan(x) && !std::isnan(y);
            if (prox) {
                input.proximity = true;
                x /= data.num_cols-1;
                y /= data.num_rows-1;
                if (conf.invert_x)
                    x = 1-x;
                if (conf.invert_y)
                    y = 1-y;
                input.x = std::clamp(x, 0., 1.);
                input.y = std::clamp(y, 0., 1.);
                return &input;
            } else
                return stop_stylus();
        } else
            return stop_stylus();
        break;

    case IPTS_DFT_ID_BUTTON:
        if (data.dft_x[0].magnitude > IPTS_DFT_BUTTON_MIN_MAG && data.dft_y[0].magnitude > IPTS_DFT_BUTTON_MIN_MAG) {
            // same phase as position signal = eraser, opposite phase = button
            int btn = real * (data.dft_x[0].real[IPTS_DFT_NUM_COMPONENTS/2] + data.dft_y[0].real[IPTS_DFT_NUM_COMPONENTS/2])
                    + imag * (data.dft_x[0].imag[IPTS_DFT_NUM_COMPONENTS/2] + data.dft_y[0].imag[IPTS_DFT_NUM_COMPONENTS/2]);
            input.button = btn < 0;
            rubber = btn > 0;
        } else {
            input.button = false;
            rubber = false;
        }
        // toggling rubber while proximity is true seems to cause issues, so set proximity off first
        if (rubber != input.rubber) {
            set_rubber = true;
            return stop_stylus();
        }
        break;

    case IPTS_DFT_ID_PRESSURE:
        p = dft_interpolate_frequency(data.dft_x, data.dft_y, IPTS_DFT_PRESSURE_ROWS);
        p = (IPTS_DFT_PRESSURE_ROWS - 1 - p) * IPTS_MAX_PRESSURE / (IPTS_DFT_PRESSURE_ROWS - 1);
        if (p > 1 && !std::isnan(p)) {
            input.contact = true;
            input.pressure = std::min(IPTS_MAX_PRESSURE, (int)p);
        } else {
            input.contact = false;
            input.pressure = 0;
        }
        break;
    }
    return nullptr;
}

}
