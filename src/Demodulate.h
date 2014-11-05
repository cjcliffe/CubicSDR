#pragma once

/*
 * based on rtl_fm.c
 * https://github.com/keenerd/rtl-sdr/blob/master/src/rtl_fm.c
 */

#include <vector>
#include <stdint.h>
#include <stddef.h>

#define FREQUENCIES_LIMIT   1000

struct translate_state {
    double angle; /* radians */
    int16_t *sincos; /* pairs */
    int len;
    int i;
};

struct buffer_bucket {
    int16_t *buf;
    int len;
    int trycond;
};

struct output_state {
    int exit_flag;
    struct buffer_bucket results[2];
    int rate;
    int wav_format;
    int padded;
    int lrmix;
};

struct controller_state {
    int exit_flag;
    uint32_t freqs[FREQUENCIES_LIMIT];
    int freq_len;
    int freq_now;
    int edge;
    int wb_mode;
};

class Demodulate {

public:
    Demodulate();

    ~Demodulate() {

    }

    void demod(std::vector<int16_t> &buffer);

public:
    int16_t *lowpassed;
    int lp_len;
    int16_t lp_i_hist[10][6];
    int16_t lp_q_hist[10][6];
    int16_t droop_i_hist[9];
    int16_t droop_q_hist[9];
    int rate_in;
    int rate_out;
    int rate_out2;
    int now_r, now_j;
    int pre_r, pre_j;
    int prev_index;
    int downsample; /* min 1, max 256 */
    int post_downsample;
    int output_scale;
    int squelch_level, conseq_squelch, squelch_hits, terminate_on_squelch;
    int downsample_passes;
    int comp_fir_size;
    int custom_atan;
    int deemph, deemph_a;
    int now_lpr;
    int prev_lpr_index;
    int dc_block, dc_avg;
    int rotate_enable;
    struct translate_state rotate;
    void (*mode_demod)(Demodulate *);
    struct buffer_bucket *output_target;
    struct output_state output;
};
