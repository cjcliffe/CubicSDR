#include "Demodulate.h"
#include "CubicSDRDefs.h"

#include <math.h>
#include <cstring>
#include <stdlib.h>
#include <iostream>

#define DEFAULT_SAMPLE_RATE 24000
#define DEFAULT_BUF_LENGTH  (1 * 16384)
#define MAXIMUM_OVERSAMPLE  16
#define MAXIMUM_BUF_LENGTH  (MAXIMUM_OVERSAMPLE * DEFAULT_BUF_LENGTH)
#define AUTO_GAIN   -100
#define BUFFER_DUMP 4096
//#define MAXIMUM_RATE    2400000
#define PI_INT  (1<<14)
#define ONE_INT (1<<14)
static int *atan_lut = NULL;
static int atan_lut_size = 131072; /* 512 KB */
static int atan_lut_coef = 8;
//static uint32_t MINIMUM_RATE = 1000000;

/* {length, coef, coef, coef} and scaled by 2^15
 for now, only length 9, optimal way to get +85% bandwidth */
#define CIC_TABLE_MAX 10
int cic_9_tables[][10] = { { 0, }, { 9, -156, -97, 2798, -15489, 61019, -15489, 2798, -97, -156 }, { 9, -128, -568, 5593, -24125, 74126, -24125, 5593,
        -568, -128 }, { 9, -129, -639, 6187, -26281, 77511, -26281, 6187, -639, -129 },
        { 9, -122, -612, 6082, -26353, 77818, -26353, 6082, -612, -122 }, { 9, -120, -602, 6015, -26269, 77757, -26269, 6015, -602, -120 }, { 9, -120,
                -582, 5951, -26128, 77542, -26128, 5951, -582, -120 }, { 9, -119, -580, 5931, -26094, 77505, -26094, 5931, -580, -119 }, { 9, -119,
                -578, 5921, -26077, 77484, -26077, 5921, -578, -119 }, { 9, -119, -577, 5917, -26067, 77473, -26067, 5917, -577, -119 }, { 9, -199,
                -362, 5303, -25505, 77489, -25505, 5303, -362, -199 }, };

#ifdef _MSC_VER
double log2(double n)
{
    return log(n) / log(2.0);
}
#endif
void rotate_90(unsigned char *buf, uint32_t len)
/* 90 rotation is 1+0j, 0+1j, -1+0j, 0-1j
 or [0, 1, -3, 2, -4, -5, 7, -6] */
{
    uint32_t i;
    unsigned char tmp;
    for (i = 0; i < len; i += 8) {
        /* uint8_t negation = 255 - x */
        tmp = 255 - buf[i + 3];
        buf[i + 3] = buf[i + 2];
        buf[i + 2] = tmp;
        buf[i + 4] = 255 - buf[i + 4];
        buf[i + 5] = 255 - buf[i + 5];
        tmp = 255 - buf[i + 6];
        buf[i + 6] = buf[i + 7];
        buf[i + 7] = tmp;
    }
}

int translate_init(struct translate_state *ts)
/* two pass: first to find optimal length, second to alloc/fill */
{
    int max_length = 100000;
    int i, s, c, best_i;
    double a, a2, err, best_360;
    if (fabs(ts->angle) < 2 * M_PI / max_length) {
        std::cout << "angle too small or array too short\n" << std::endl;
        return 1;
    }
    ts->i = 0;
    ts->sincos = NULL;
    if (ts->len) {
        max_length = ts->len;
        ts->sincos = (int16_t *) malloc(max_length * sizeof(int16_t));
    }
    a = 0.0;
    err = 0.0;
    best_i = 0;
    best_360 = 4.0;
    for (i = 0; i < max_length; i += 2) {
        s = (int) round(sin(a + err) * (1 << 14));
        c = (int) round(cos(a + err) * (1 << 14));
        a2 = atan2(s, c);
        err = fmod(a, 2 * M_PI) - a2;
        a += ts->angle;
        while (a > M_PI) {
            a -= 2 * M_PI;
        }
        while (a < -M_PI) {
            a += 2 * M_PI;
        }
        if (i != 0 && fabs(a) < best_360) {
            best_i = i;
            best_360 = fabs(a);
        }
        if (i != 0 && fabs(a) < 0.0000001) {
            break;
        }
        if (ts->sincos) {
            ts->sincos[i] = s;
            ts->sincos[i + 1] = c;
//fprintf(stderr, "%i %i %i\n", i, s, c);
        }
    }
    if (ts->sincos) {
        return 0;
    }
    ts->len = best_i + 2;
    return translate_init(ts);
}

void translate(Demodulate *d) {
    int i, len, sc_i, sc_len;
    int32_t tmp, ar, aj, br, bj;
    int16_t *buf = d->lowpassed;
    int16_t *sincos = d->rotate.sincos;
    len = d->lp_len;
    sc_i = d->rotate.i;
    sc_len = d->rotate.len;
    for (i = 0; i < len; i += 2, sc_i += 2) {
        sc_i = sc_i % sc_len;
        ar = (int32_t) buf[i];
        aj = (int32_t) buf[i + 1];
        br = (int32_t) sincos[sc_i];
        bj = (int32_t) sincos[sc_i + 1];
        tmp = ar * br - aj * bj;
        buf[i] = (int16_t) (tmp >> 14);
        tmp = aj * br + ar * bj;
        buf[i + 1] = (int16_t) (tmp >> 14);
    }
    d->rotate.i = sc_i;
}

void low_pass(Demodulate *d)
/* simple square window FIR */
{
    int i = 0, i2 = 0;
    while (i < d->lp_len) {
        d->now_r += d->lowpassed[i];
        d->now_j += d->lowpassed[i + 1];
        i += 2;
        d->prev_index++;
        if (d->prev_index < d->downsample) {
            continue;
        }
        d->lowpassed[i2] = d->now_r; // * d->output_scale;
        d->lowpassed[i2 + 1] = d->now_j; // * d->output_scale;
        d->prev_index = 0;
        d->now_r = 0;
        d->now_j = 0;
        i2 += 2;
    }
    d->lp_len = i2;
}

int low_pass_simple(int16_t *signal2, int len, int step)
// no wrap around, length must be multiple of step
        {
    int i, i2, sum;
    for (i = 0; i < len; i += step) {
        sum = 0;
        for (i2 = 0; i2 < step; i2++) {
            sum += (int) signal2[i + i2];
        }
//signal2[i/step] = (int16_t)(sum / step);
        signal2[i / step] = (int16_t) (sum);
    }
    signal2[i / step + 1] = signal2[i / step];
    return len / step;
}

void low_pass_real(Demodulate *s)
/* simple square window FIR */
// add support for upsampling?
        {
    int16_t *lp = s->lowpassed;
    int i = 0, i2 = 0;
    int fast = (int) s->rate_out;
    int slow = s->rate_out2;
    while (i < s->lp_len) {
        s->now_lpr += lp[i];
        i++;
        s->prev_lpr_index += slow;
        if (s->prev_lpr_index < fast) {
            continue;
        }
        lp[i2] = (int16_t) (s->now_lpr / (fast / slow));
        s->prev_lpr_index -= fast;
        s->now_lpr = 0;
        i2 += 1;
    }
    s->lp_len = i2;
}

void fifth_order(int16_t *data, int length, int16_t *hist)
/* for half of interleaved data */
{
    int i;
    int16_t a, b, c, d, e, f;
    a = hist[1];
    b = hist[2];
    c = hist[3];
    d = hist[4];
    e = hist[5];
    f = data[0];
    /* a downsample should improve resolution, so don't fully shift */
    data[0] = (a + (b + e) * 5 + (c + d) * 10 + f) >> 4;
    for (i = 4; i < length; i += 4) {
        a = c;
        b = d;
        c = e;
        d = f;
        e = data[i - 2];
        f = data[i];
        data[i / 2] = (a + (b + e) * 5 + (c + d) * 10 + f) >> 4;
    }
    /* archive */
    hist[0] = a;
    hist[1] = b;
    hist[2] = c;
    hist[3] = d;
    hist[4] = e;
    hist[5] = f;
}

void generic_fir(int16_t *data, int length, int *fir, int16_t *hist)
/* Okay, not at all generic. Assumes length 9, fix that eventually. */
{
    int d, temp, sum;
    for (d = 0; d < length; d += 2) {
        temp = data[d];
        sum = 0;
        sum += (hist[0] + hist[8]) * fir[1];
        sum += (hist[1] + hist[7]) * fir[2];
        sum += (hist[2] + hist[6]) * fir[3];
        sum += (hist[3] + hist[5]) * fir[4];
        sum += hist[4] * fir[5];
        data[d] = sum >> 15;
        hist[0] = hist[1];
        hist[1] = hist[2];
        hist[2] = hist[3];
        hist[3] = hist[4];
        hist[4] = hist[5];
        hist[5] = hist[6];
        hist[6] = hist[7];
        hist[7] = hist[8];
        hist[8] = temp;
    }
}

/* define our own complex math ops
 because ARMv5 has no hardware float */
void multiply(int ar, int aj, int br, int bj, int *cr, int *cj) {
    *cr = ar * br - aj * bj;
    *cj = aj * br + ar * bj;
}
int polar_discriminant(int ar, int aj, int br, int bj) {
    int cr, cj;
    double angle;
    multiply(ar, aj, br, -bj, &cr, &cj);
    angle = atan2((double) cj, (double) cr);
    return (int) (angle / M_PI * (1 << 14));
}
int fast_atan2(int y, int x)
/* pre scaled for int16 */
{
    int yabs, angle;
    int pi4 = (1 << 12), pi34 = 3 * (1 << 12); // note pi = 1<<14
    if (x == 0 && y == 0) {
        return 0;
    }
    yabs = y;
    if (yabs < 0) {
        yabs = -yabs;
    }
    if (x >= 0) {
        angle = pi4 - pi4 * (x - yabs) / (x + yabs);
    } else {
        angle = pi34 - pi4 * (x + yabs) / (yabs - x);
    }
    if (y < 0) {
        return -angle;
    }
    return angle;
}

int polar_disc_fast(int ar, int aj, int br, int bj) {
    int cr, cj;
    multiply(ar, aj, br, -bj, &cr, &cj);
    return fast_atan2(cj, cr);
}

int atan_lut_init(void) {
    int i = 0;
    atan_lut = (int *) malloc(atan_lut_size * sizeof(int));
    for (i = 0; i < atan_lut_size; i++) {
        atan_lut[i] = (int) (atan((double) i / (1 << atan_lut_coef)) / M_PI * (1 << 14));
    }
    return 0;
}

int polar_disc_lut(int ar, int aj, int br, int bj) {
    int cr, cj, x, x_abs;
    multiply(ar, aj, br, -bj, &cr, &cj);
    /* special cases */
    if (cr == 0 || cj == 0) {
        if (cr == 0 && cj == 0) {
            return 0;
        }
        if (cr == 0 && cj > 0) {
            return 1 << 13;
        }
        if (cr == 0 && cj < 0) {
            return -(1 << 13);
        }
        if (cj == 0 && cr > 0) {
            return 0;
        }
        if (cj == 0 && cr < 0) {
            return 1 << 14;
        }
    }
    /* real range -32768 - 32768 use 64x range -> absolute maximum: 2097152 */
    x = (cj << atan_lut_coef) / cr;
    x_abs = abs(x);
    if (x_abs >= atan_lut_size) {
        /* we can use linear range, but it is not necessary */
        return (cj > 0) ? 1 << 13 : -1 << 13;
    }
    if (x > 0) {
        return (cj > 0) ? atan_lut[x] : atan_lut[x] - (1 << 14);
    } else {
        return (cj > 0) ? (1 << 14) - atan_lut[-x] : -atan_lut[-x];
    }
    return 0;
}

int esbensen(int ar, int aj, int br, int bj)
/*
 input signal: s(t) = a*exp(-i*w*t+p)
 a = amplitude, w = angular freq, p = phase difference
 solve w
 s' = -i(w)*a*exp(-i*w*t+p)
 s'*conj(s) = -i*w*a*a
 s'*conj(s) / |s|^2 = -i*w
 */
{
    int cj, dr, dj;
    int scaled_pi = 2608; /* 1<<14 / (2*pi) */
    dr = (br - ar) * 2;
    dj = (bj - aj) * 2;
    cj = bj * dr - br * dj; /* imag(ds*conj(s)) */
    return (scaled_pi * cj / (ar * ar + aj * aj + 1));
}

void fm_demod(Demodulate *fm) {
    int i, pcm = 0;
    int16_t *lp = fm->lowpassed;
    int16_t pr = fm->pre_r;
    int16_t pj = fm->pre_j;
    for (i = 0; i < (fm->lp_len - 1); i += 2) {
        switch (fm->custom_atan) {
        case 0:
            pcm = polar_discriminant(lp[i], lp[i + 1], pr, pj);
            break;
        case 1:
            pcm = polar_disc_fast(lp[i], lp[i + 1], pr, pj);
            break;
        case 2:
            pcm = polar_disc_lut(lp[i], lp[i + 1], pr, pj);
            break;
        case 3:
            pcm = esbensen(lp[i], lp[i + 1], pr, pj);
            break;
        }
        pr = lp[i];
        pj = lp[i + 1];
        fm->lowpassed[i / 2] = (int16_t) pcm;
    }
    fm->pre_r = pr;
    fm->pre_j = pj;
    fm->lp_len = fm->lp_len / 2;
}

void am_demod(Demodulate *fm)
// todo, fix this extreme laziness
        {
    int32_t i, pcm;
    int16_t *lp = fm->lowpassed;
    for (i = 0; i < fm->lp_len; i += 2) {
// hypot uses floats but won't overflow
//r[i/2] = (int16_t)hypot(lp[i], lp[i+1]);
        pcm = lp[i] * lp[i];
        pcm += lp[i + 1] * lp[i + 1];
        lp[i / 2] = (int16_t) sqrt(pcm) * fm->output_scale;
    }
    fm->lp_len = fm->lp_len / 2;
// lowpass? (3khz)
}

void usb_demod(Demodulate *fm) {
    int i, pcm;
    int16_t *lp = fm->lowpassed;
    for (i = 0; i < fm->lp_len; i += 2) {
        pcm = lp[i] + lp[i + 1];
        lp[i / 2] = (int16_t) pcm * fm->output_scale;
    }
    fm->lp_len = fm->lp_len / 2;
}

void lsb_demod(Demodulate *fm) {
    int i, pcm;
    int16_t *lp = fm->lowpassed;
    for (i = 0; i < fm->lp_len; i += 2) {
        pcm = lp[i] - lp[i + 1];
        lp[i / 2] = (int16_t) pcm * fm->output_scale;
    }
    fm->lp_len = fm->lp_len / 2;
}

void raw_demod(Demodulate *fm) {
    return;
}

void deemph_filter(Demodulate *fm) {
    static int avg; // cheating, not threadsafe
    int i, d;
    int16_t *lp = fm->lowpassed;
// de-emph IIR
// avg = avg * (1 - alpha) + sample * alpha;
    for (i = 0; i < fm->lp_len; i++) {
        d = lp[i] - avg;
        if (d > 0) {
            avg += (d + fm->deemph_a / 2) / fm->deemph_a;
        } else {
            avg += (d - fm->deemph_a / 2) / fm->deemph_a;
        }
        lp[i] = (int16_t) avg;
    }
}

void dc_block_filter(Demodulate *fm) {
    int i, avg;
    int64_t sum = 0;
    int16_t *lp = fm->lowpassed;
    for (i = 0; i < fm->lp_len; i++) {
        sum += lp[i];
    }
    avg = sum / fm->lp_len;
    avg = (avg + fm->dc_avg * 9) / 10;
    for (i = 0; i < fm->lp_len; i++) {
        lp[i] -= avg;
    }
    fm->dc_avg = avg;
}

int mad(int16_t *samples, int len, int step)
/* mean average deviation */
{
    int i = 0, sum = 0, ave = 0;
    if (len == 0) {
        return 0;
    }
    for (i = 0; i < len; i += step) {
        sum += samples[i];
    }
    ave = sum / (len * step);
    sum = 0;
    for (i = 0; i < len; i += step) {
        sum += abs(samples[i] - ave);
    }
    return sum / (len / step);
}

int rms(int16_t *samples, int len, int step)
/* largely lifted from rtl_power */
{
    int i;
    long p, t, s;
    double dc, err;
    p = t = 0L;
    for (i = 0; i < len; i += step) {
        s = (long) samples[i];
        t += s;
        p += s * s;
    }
    /* correct for dc offset in squares */
    dc = (double) (t * step) / (double) len;
    err = t * 2 * dc - dc * dc * len;
    return (int) sqrt((p - err) / len);
}

int squelch_to_rms(int db, struct dongle_state *dongle, Demodulate *demod)
/* 0 dB = 1 rms at 50dB gain and 1024 downsample */
{
    double linear, gain, downsample;
    if (db == 0) {
        return 0;
    }
    linear = pow(10.0, (double) db / 20.0);
    gain = 50.0;
//    if (dongle->gain != AUTO_GAIN) {
//        gain = (double) (dongle->gain) / 10.0;
//    }
    gain = 50.0 - gain;
    gain = pow(10.0, gain / 20.0);
    downsample = 1024.0 / (double) demod->downsample;
    linear = linear / gain;
    linear = linear / downsample;
    return (int) linear + 1;
}

Demodulate::Demodulate() {
    rate_in = DEFAULT_SAMPLE_RATE;
    rate_out = DEFAULT_SAMPLE_RATE;
    squelch_level = 0;
    conseq_squelch = 10;
    terminate_on_squelch = 0;
    squelch_hits = 11;
    downsample_passes = 0;
    comp_fir_size = 0;
    prev_index = 0;
    post_downsample = 1; // once this works, default = 4
    custom_atan = 0;
    deemph = 0;
    rotate_enable = 0;
    rotate.len = 0;
    rotate.sincos = NULL;
    rate_out2 = -1; // flag for disabled
    mode_demod = &fm_demod;
//            pre_j = s->pre_r = s->now_r = s->now_j = 0;
    prev_lpr_index = 0;
    deemph_a = 0;
    now_lpr = 0;
    dc_block = 1;
    dc_avg = 0;
    output_target = &output.results[0];
    lowpassed = NULL;

    mode_demod = &fm_demod;
    rate_in = 170000;
    rate_out = 170000;
    rate_out2 = 44000;
    output.rate = 44000;
    custom_atan = 1;
//    post_downsample = 4;
    deemph = 1;
    squelch_level = 0;

    int capture_freq;

    // downsample = (SRATE / rate_in) + 1;
    downsample = (SRATE / rate_in) + 1;
    if (downsample_passes) {
        downsample_passes = (int) log2(downsample) + 1;
        downsample = 1 << downsample_passes;
    }

    if (deemph) {
        deemph_a = (int)round(1.0/((1.0-exp(-1.0/(rate_out * 75e-6)))));
    }

    capture_freq=DEFAULT_FREQ;

    //capture_freq = freq;
    //capture_rate = downsample * rate_in;

    int edge = 0;

//     if (d->pre_rotate) {
//     capture_freq = freq + capture_rate/4;}
    capture_freq += edge * rate_in / 2;
    output_scale = (1 << 15) / (128 * downsample);
    if (output_scale < 1) {
        output_scale = 1;
    }
    if (mode_demod == &fm_demod) {
        output_scale = 1;
    }

    custom_atan = 1;
    //demod.post_downsample = 4;
    deemph = 1;
    squelch_level = 0;

    int r, opt;
    int dev_given = 0;
    int custom_ppm = 0;

    output.results[0].trycond = 1;
    output.results[0].buf = NULL;
    output.results[1].trycond = 1;
    output.results[1].buf = NULL;


//    controller.freqs[0] = 100000000;
//    controller.freq_len = 0;
//    controller.edge = 0;
//    controller.wb_mode = 0;

//                if (strcmp("fm", optarg) == 0) {
//                    demod.mode_demod = &fm_demod;
//                }
//                if (strcmp("raw", optarg) == 0) {
//                    demod.mode_demod = &raw_demod;
//                }
//                if (strcmp("am", optarg) == 0) {
//                    demod.mode_demod = &am_demod;
//                }
//                if (strcmp("usb", optarg) == 0) {
//                    demod.mode_demod = &usb_demod;
//                }
//                if (strcmp("lsb", optarg) == 0) {
//                    demod.mode_demod = &lsb_demod;
//                }
//                if (strcmp("wbfm", optarg) == 0) {
//    controller.wb_mode = 1;

//                }
//                break;

    // rate_in *= post_downsample;
    if (!output.rate) {
        output.rate = rate_out;
    }

//    if (controller.freq_len > 1) {
//        terminate_on_squelch = 0;
//    }

    output.padded = 0;
}

void Demodulate::demod(std::vector<int16_t> &buffer) {
    int i, ds_p;
    int do_squelch = 0;
    int sr = 0;
    if (rotate_enable) {
        translate(this);
    }
    ds_p = downsample_passes;

    lowpassed = &buffer[0];
    lp_len = buffer.size();

    if (ds_p) {
        for (i = 0; i < ds_p; i++) {
            fifth_order(lowpassed, (lp_len >> i), lp_i_hist[i]);
            fifth_order(lowpassed + 1, (lp_len >> i) - 1, lp_q_hist[i]);
        }
        lp_len = lp_len >> ds_p;
        /* droop compensation */
        if (comp_fir_size == 9 && ds_p <= CIC_TABLE_MAX) {
            generic_fir(lowpassed, lp_len, cic_9_tables[ds_p], droop_i_hist);
            generic_fir(lowpassed + 1, lp_len - 1, cic_9_tables[ds_p], droop_q_hist);
        }
    } else {
        low_pass(this);
    }
    /* power squelch */
    if (squelch_level) {
        sr = rms(lowpassed, lp_len, 1);
        if (sr < squelch_level) {
            do_squelch = 1;
        }
    }
    if (do_squelch) {
        squelch_hits++;
        for (i = 0; i < lp_len; i++) {
            lowpassed[i] = 0;
        }
    } else {
        squelch_hits = 0;
    }

    mode_demod(this); /* lowpassed -> lowpassed */
    if (mode_demod == &raw_demod) {
        return;
    }

    if (dc_block) {
        dc_block_filter(this);
    }

    /* todo, fm noise squelch */
    // use nicer filter here too?
    if (post_downsample > 1) {
        lp_len = low_pass_simple(lowpassed, lp_len, post_downsample);
    }
    if (deemph) {
        deemph_filter(this);
    }
    if (rate_out2 > 0) {
        low_pass_real(this);
    }

    output_target->buf = lowpassed;
    output_target->len = lp_len;
}

