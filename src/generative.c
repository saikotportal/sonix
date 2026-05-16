#include "sonix.h"
#include "sequencer.h"
#include "generative.h"
#include <string.h>
#include <math.h>

static void euclidean(int pulses, int steps, int *out) {
    memset(out, 0, steps * sizeof(int));
    if (pulses <= 0) return;
    if (pulses >= steps) { for (int i = 0; i < steps; i++) out[i] = 1; return; }

    int pattern[MAX_STEPS] = {0};
    int remainder[MAX_STEPS];
    int count[MAX_STEPS];
    int divisor = steps - pulses;

    remainder[0] = pulses;
    int level = 0;

    do {
        count[level]     = divisor / remainder[level];
        remainder[level+1] = divisor % remainder[level];
        divisor          = remainder[level];
        level++;
    } while (remainder[level] > 1);

    count[level] = divisor;

    int build[MAX_STEPS][2];
    int blen = 0;

    void build_pattern(int lv) {
        if (lv == -1) {
            build[blen][0] = 0; build[blen][1] = 1; blen++;
        } else if (lv == -2) {
            build[blen][0] = 0; build[blen][1] = 0; blen++;
        } else {
            for (int i = 0; i < count[lv]; i++) build_pattern(lv - 1);
            if (remainder[lv] != 0) build_pattern(lv - 2);
        }
    }

    build_pattern(level);

    int idx = 0;
    for (int i = 0; i < blen && idx < steps; i++) {
        pattern[idx++] = build[i][1];
    }

    for (int i = 0; i < steps; i++) out[i] = pattern[i];
    (void)pattern;
}

void gen_euclidean_pattern(Sequencer *seq, int track_idx, int pattern_idx,
                            int pulses) {
    if (track_idx < 0 || track_idx >= MAX_TRACKS) return;
    Track   *tr  = &seq->tracks[track_idx];
    Pattern *pat = &tr->patterns[pattern_idx];
    int      hits[MAX_STEPS];

    euclidean(pulses, MAX_STEPS, hits);

    for (int s = 0; s < MAX_STEPS; s++) {
        pat->steps[s].on = hits[s];
        if (hits[s]) {
            pat->steps[s].velocity = 0.7f + 0.3f * ((float)rand() / RAND_MAX);
            pat->steps[s].note     = tr->octave * 12 + 36;
            pat->steps[s].duration = 0.08f;
        }
    }
}

static int markov[12][12] = {
    {3,1,0,0,2,1,0,2,0,1,0,1},
    {1,1,2,0,0,0,0,1,0,0,0,0},
    {0,1,1,2,0,1,0,0,1,0,0,0},
    {0,0,2,1,2,0,0,0,1,0,0,0},
    {2,0,0,1,1,1,0,2,0,1,0,0},
    {1,0,1,0,1,2,1,0,0,0,1,0},
    {0,0,0,0,0,1,1,2,0,0,0,0},
    {3,1,0,0,1,0,1,2,1,0,0,1},
    {0,0,1,1,0,0,0,1,1,2,0,0},
    {1,0,0,0,1,0,0,0,1,2,1,0},
    {0,0,0,0,0,1,0,0,0,1,1,1},
    {1,0,0,0,0,0,0,1,0,0,1,2},
};

static int markov_next(int cur) {
    int total = 0;
    for (int i = 0; i < 12; i++) total += markov[cur][i];
    int r = rand() % total;
    int acc = 0;
    for (int i = 0; i < 12; i++) {
        acc += markov[cur][i];
        if (r < acc) return i;
    }
    return 0;
}

void gen_markov_melody(Sequencer *seq, int track_idx, int pattern_idx) {
    if (track_idx < 0 || track_idx >= MAX_TRACKS) return;
    Track   *tr  = &seq->tracks[track_idx];
    Pattern *pat = &tr->patterns[pattern_idx];

    int cur_pc = rand() % 12;
    int base   = seq->root_note + (tr->octave - 4) * 12;

    for (int s = 0; s < MAX_STEPS; s++) {
        float r = (float)rand() / RAND_MAX;
        pat->steps[s].on = r < 0.55f;
        if (pat->steps[s].on) {
            cur_pc = markov_next(cur_pc);
            int octave_shift = (rand() % 3 - 1) * 12;
            pat->steps[s].note     = base + cur_pc + octave_shift;
            pat->steps[s].velocity = 0.5f + 0.5f * ((float)rand() / RAND_MAX);
            pat->steps[s].duration = 0.05f + 0.25f * ((float)rand() / RAND_MAX);
        }
    }
}

void gen_auto_evolve(Sequencer *seq, long tick) {
    if (tick % 32 == 0) {
        int t = rand() % seq->num_tracks;
        int p = seq->tracks[t].cur_pattern;
        if (t == 0) {
            int pulses = 2 + rand() % 6;
            gen_euclidean_pattern(seq, t, p, pulses);
        } else {
            gen_markov_melody(seq, t, p);
        }
    }

    if (tick % 64 == 0 && tick > 0) {
        seq->bpm = 90.0f + (float)(rand() % 60);
    }
}
