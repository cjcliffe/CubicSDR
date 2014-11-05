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
#define MAXIMUM_RATE    2400000
#define PI_INT  (1<<14)
#define ONE_INT (1<<14)
static int *atan_lut = NULL;
static int atan_lut_size = 131072; /* 512 KB */
static int atan_lut_coef = 8;
// rewrite as dynamic and thread-safe for multi demod/dongle
#define SHARED_SIZE 6
int16_t shared_samples[SHARED_SIZE][MAXIMUM_BUF_LENGTH];
int ss_busy[SHARED_SIZE] = { 0 };

/* more cond dumbness */
#define safe_cond_signal(n, m) pthread_mutex_lock(m); pthread_cond_signal(n); pthread_mutex_unlock(m)
#define safe_cond_wait(n, m) pthread_mutex_lock(m); pthread_cond_wait(n, m); pthread_mutex_unlock(m)
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
    /*
     int capture_freq, capture_rate;
     struct dongle_state *d = &dongle;
     struct demod_state *dm = &demod;
     struct controller_state *cs = &controller;
     dm->downsample = (MINIMUM_RATE / dm->rate_in) + 1;
     if (dm->downsample_passes) {
     dm->downsample_passes = (int)log2(dm->downsample) + 1;
     dm->downsample = 1 << dm->downsample_passes;
     }
     capture_freq = freq;
     capture_rate = dm->downsample * dm->rate_in;
     if (d->pre_rotate) {
     capture_freq = freq + capture_rate/4;}
     capture_freq += cs->edge * dm->rate_in / 2;
     dm->output_scale = (1<<15) / (128 * dm->downsample);
     if (dm->output_scale < 1) {
     dm->output_scale = 1;}
     if (dm->mode_demod == &fm_demod) {
     dm->output_scale = 1;}
     d->freq = (uint32_t)capture_freq;
     d->rate = (uint32_t)capture_rate;

     */

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
    mode_demod = &fm_demod;
    rate_in = SRATE; //170000
    rate_out = SRATE; //170000
    rate_out2 = 32000;
    output.rate = 32000;
    custom_atan = 1;
    //demod.post_downsample = 4;
    deemph = 1;
    squelch_level = 0;
//                }
//                break;

    rate_in *= post_downsample;
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
}

/*
 static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {
 int i;
 struct dongle_state *s = ctx;
 struct demod_state *d = s->targets[0];
 struct demod_state *d2 = s->targets[1];
 if (do_exit) {
 return;
 }
 if (!ctx) {
 return;
 }
 if (s->mute) {
 for (i = 0; i < s->mute; i++) {
 buf[i] = 127;
 }
 s->mute = 0;
 }
 if (s->pre_rotate) {
 rotate_90(buf, len);
 }
 for (i = 0; i < (int) len; i++) {
 s->buf16[i] = (int16_t) buf[i] - 127;
 }
 if (d2 != NULL) {
 pthread_rwlock_wrlock(&d2->rw);
 d2->lowpassed = mark_shared_buffer();
 memcpy(d2->lowpassed, s->buf16, 2 * len);
 d2->lp_len = len;
 pthread_rwlock_unlock(&d2->rw);
 safe_cond_signal(&d2->ready, &d2->ready_m);
 }
 pthread_rwlock_wrlock(&d->rw);
 d->lowpassed = s->buf16;
 d->lp_len = len;
 pthread_rwlock_unlock(&d->rw);
 safe_cond_signal(&d->ready, &d->ready_m);
 s->buf16 = mark_shared_buffer();
 }


 static void *demod_thread_fn(void *arg) {
 struct demod_state *d = arg;
 struct buffer_bucket *o = d->output_target;
 while (!do_exit) {
 safe_cond_wait(&d->ready, &d->ready_m);
 pthread_rwlock_wrlock(&d->rw);
 full_demod(d);
 pthread_rwlock_unlock(&d->rw);
 if (d->exit_flag) {
 do_exit = 1;
 }
 pthread_rwlock_wrlock(&o->rw);
 o->buf = d->lowpassed;
 o->len = d->lp_len;
 pthread_rwlock_unlock(&o->rw);
 if (controller.freq_len > 1 && d->squelch_level && d->squelch_hits > d->conseq_squelch) {
 unmark_shared_buffer(d->lowpassed);
 d->squelch_hits = d->conseq_squelch + 1;
 safe_cond_signal(&controller.hop, &controller.hop_m);
 continue;
 }
 safe_cond_signal(&o->ready, &o->ready_m);
 pthread_mutex_lock(&o->trycond_m);
 o->trycond = 0;
 pthread_mutex_unlock(&o->trycond_m);
 }
 return 0;
 }


 #ifndef _WIN32
 static int get_nanotime(struct timespec *ts)
 {
 int rv = ENOSYS;
 #ifdef __unix__
 rv = clock_gettime(CLOCK_MONOTONIC, ts);
 #elif __APPLE__
 struct timeval tv;
 rv = gettimeofday(&tv, NULL);
 ts->tv_sec = tv.tv_sec;
 ts->tv_nsec = tv.tv_usec * 1000L;
 #endif
 return rv;
 }
 #endif
 static void *output_thread_fn(void *arg) {
 int j, r = 0;
 struct output_state *s = arg;
 struct buffer_bucket *b0 = &s->results[0];
 struct buffer_bucket *b1 = &s->results[1];
 struct timespec start_time;
 struct timespec now_time;
 int64_t i, duration, samples, samples_now;
 samples = 0L;
 #ifndef _WIN32
 get_nanotime(&start_time);
 #endif
 while (!do_exit) {
 if (s->lrmix) {
 safe_cond_wait(&b0->ready, &b0->ready_m);
 pthread_rwlock_rdlock(&b0->rw);
 safe_cond_wait(&b1->ready, &b1->ready_m);
 pthread_rwlock_rdlock(&b1->rw);
 for (j = 0; j < b0->len; j++) {
 fwrite(b0->buf + j, 2, 1, s->file);
 fwrite(b1->buf + j, 2, 1, s->file);
 }
 unmark_shared_buffer(b1->buf);
 pthread_rwlock_unlock(&b1->rw);
 unmark_shared_buffer(b0->buf);
 pthread_rwlock_unlock(&b0->rw);
 continue;
 }
 if (!s->padded) {
 safe_cond_wait(&b0->ready, &b0->ready_m);
 pthread_rwlock_rdlock(&b0->rw);
 fwrite(b0->buf, 2, b0->len, s->file);
 unmark_shared_buffer(b0->buf);
 pthread_rwlock_unlock(&b0->rw);
 continue;
 }
 #ifndef _WIN32
 // padding requires output at constant rate
 // pthread_cond_timedwait is terrible, roll our own trycond
 // figure out how to do this with windows HPET
 usleep(2000);
 pthread_mutex_lock(&b0->trycond_m);
 r = b0->trycond;
 b0->trycond = 1;
 pthread_mutex_unlock(&b0->trycond_m);
 if (r == 0) {
 pthread_rwlock_rdlock(&b0->rw);
 fwrite(b0->buf, 2, b0->len, s->file);
 unmark_shared_buffer(b0->buf);
 samples += (int64_t)b0->len;
 pthread_rwlock_unlock(&b0->rw);
 continue;
 }
 get_nanotime(&now_time);
 duration = now_time.tv_sec - start_time.tv_sec;
 duration *= 1000000000L;
 duration += (now_time.tv_nsec - start_time.tv_nsec);
 samples_now = (duration * (int64_t)s->rate) / 1000000000L;
 if (samples_now < samples) {
 continue;}
 for (i=samples; i<samples_now; i++) {
 fputc(0, s->file);
 fputc(0, s->file);
 }
 samples = samples_now;
 #endif
 }
 return 0;
 }

 static void optimal_settings(int freq, int rate) {
 // giant ball of hacks
 // seems unable to do a single pass, 2:1
 int capture_freq, capture_rate;
 struct dongle_state *d = &dongle;
 struct demod_state *dm = &demod;
 struct controller_state *cs = &controller;
 dm->downsample = (MINIMUM_RATE / dm->rate_in) + 1;
 if (dm->downsample_passes) {
 dm->downsample_passes = (int) log2(dm->downsample) + 1;
 dm->downsample = 1 << dm->downsample_passes;
 }
 capture_freq = freq;
 capture_rate = dm->downsample * dm->rate_in;
 if (d->pre_rotate) {
 capture_freq = freq + capture_rate / 4;
 }
 capture_freq += cs->edge * dm->rate_in / 2;
 dm->output_scale = (1 << 15) / (128 * dm->downsample);
 if (dm->output_scale < 1) {
 dm->output_scale = 1;
 }
 if (dm->mode_demod == &fm_demod) {
 dm->output_scale = 1;
 }
 d->freq = (uint32_t) capture_freq;
 d->rate = (uint32_t) capture_rate;
 //d->pre_rotate = 0;
 //demod.rotate_enable = 1;
 //demod.rotate.angle = -0.25 * 2 * M_PI;
 //translate_init(&demod.rotate);
 }

 void optimal_lrmix(void) {
 double angle1, angle2;
 uint32_t freq, freq1, freq2, bw, dongle_bw, mr;
 if (controller.freq_len != 2) {
 fprintf(stderr, "error: lrmix requires two frequencies\n");
 do_exit = 1;
 exit(1);
 }
 if (output.padded) {
 fprintf(stderr, "warning: lrmix does not support padding\n");
 }
 freq1 = controller.freqs[0];
 freq2 = controller.freqs[1];
 bw = demod.rate_out;
 freq = freq1 / 2 + freq2 / 2 + bw;
 mr = (uint32_t) abs((int64_t) freq1 - (int64_t) freq2) + bw;
 if (mr > MINIMUM_RATE) {
 MINIMUM_RATE = mr;
 }
 dongle.pre_rotate = 0;
 optimal_settings(freq, bw);
 output.padded = 0;
 clone_demod(&demod2, &demod);
 //demod2 = demod;
 demod2.output_target = &output.results[1];
 dongle.targets[1] = &demod2;
 dongle_bw = dongle.rate;
 if (dongle_bw > MAXIMUM_RATE) {
 fprintf(stderr, "error: unable to find optimal settings\n");
 do_exit = 1;
 exit(1);
 }
 angle1 = ((double) freq1 - (double) freq) / (double) dongle_bw;
 demod.rotate.angle = angle1 * 2 * M_PI;
 angle2 = ((double) freq2 - (double) freq) / (double) dongle_bw;
 demod2.rotate.angle = angle2 * 2 * M_PI;
 translate_init(&demod.rotate);
 translate_init(&demod2.rotate);
 //fprintf(stderr, "a1 %f, a2 %f\n", angle1, angle2);
 }
 static void *controller_thread_fn(void *arg) {
 // thoughts for multiple dongles
 // might be no good using a controller thread if retune/rate blocks
 int i;
 struct controller_state *s = arg;
 if (s->wb_mode) {
 for (i = 0; i < s->freq_len; i++) {
 s->freqs[i] += 16000;
 }
 }
 // set up primary channel
 optimal_settings(s->freqs[0], demod.rate_in);
 demod.squelch_level = squelch_to_rms(demod.squelch_level, &dongle, &demod);
 if (dongle.direct_sampling) {
 verbose_direct_sampling(dongle.dev, dongle.direct_sampling);
 }
 if (dongle.offset_tuning) {
 verbose_offset_tuning(dongle.dev);
 }
 // set up lrmix
 if (output.lrmix) {
 optimal_lrmix();
 }
 // Set the frequency
 verbose_set_frequency(dongle.dev, dongle.freq);
 fprintf(stderr, "Oversampling input by: %ix.\n", demod.downsample);
 fprintf(stderr, "Oversampling output by: %ix.\n", demod.post_downsample);
 fprintf(stderr, "Buffer size: %0.2fms\n", 1000 * 0.5 * (float) ACTUAL_BUF_LENGTH / (float) dongle.rate);
 // Set the sample rate
 verbose_set_sample_rate(dongle.dev, dongle.rate);
 fprintf(stderr, "Output at %u Hz.\n", demod.rate_in / demod.post_downsample);
 while (!do_exit) {
 safe_cond_wait(&s->hop, &s->hop_m);
 if (s->freq_len <= 1) {
 continue;
 }
 if (output.lrmix) {
 continue;
 }
 // hacky hopping
 s->freq_now = (s->freq_now + 1) % s->freq_len;
 optimal_settings(s->freqs[s->freq_now], demod.rate_in);
 rtlsdr_set_center_freq(dongle.dev, dongle.freq);
 dongle.mute = BUFFER_DUMP;
 }
 return 0;
 }

 int main(int argc, char **argv) {

 }*/
