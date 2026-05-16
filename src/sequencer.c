#include "sonix.h"
#include "sequencer.h"
#include "synth.h"
#include <string.h>
#include <stdio.h>

static const int scale_major[]  = {0, 2, 4, 5, 7, 9, 11};
static const int scale_minor[]  = {0, 2, 3, 5, 7, 8, 10};
static const int scale_pent[]   = {0, 2, 4, 7, 9};
static const int scale_blues[]  = {0, 3, 5, 6, 7, 10};
static const int scale_chrom[]  = {0,1,2,3,4,5,6,7,8,9,10,11};

static const int *scales[]     = { scale_major, scale_minor, scale_pent, scale_blues, scale_chrom };
static const int  scale_lens[] = { 7, 7, 5, 6, 12 };
const char *scale_names[]      = { "MAJOR", "MINOR", "PENTATONIC", "BLUES", "CHROMATIC" };

void seq_init(Sequencer *seq) {
    memset(seq, 0, sizeof(Sequencer));
    seq->bpm         = 120.0f;
    seq->playing     = 0;
    seq->num_tracks  = 4;
    seq->cur_scale   = 0;
    seq->root_note   = 60;

    const char *names[MAX_TRACKS] = {
        "KICK", "BASS", "LEAD", "PAD",
        "ARPG", "PERC", "FX  ", "SUB "
    };
    const int waves[MAX_TRACKS] = {
        WAVE_NOISE, WAVE_SAW, WAVE_SQUARE, WAVE_SINE,
        WAVE_TRIANGLE, WAVE_NOISE, WAVE_SAW, WAVE_SINE
    };

    for (int t = 0; t < MAX_TRACKS; t++) {
        Track *tr = &seq->tracks[t];
        strncpy(tr->name, names[t], 15);
        tr->wave_type = waves[t];
        tr->volume    = 0.7f;
        tr->octave    = (t == 0) ? 3 : (t == 1) ? 2 : 4;
        tr->attack    = 0.005f;
        tr->decay     = 0.1f;
        tr->sustain   = 0.6f;
        tr->release   = 0.05f;

        for (int p = 0; p < MAX_PATTERNS; p++) {
            tr->patterns[p].length = MAX_STEPS;
            tr->patterns[p].muted  = (t >= seq->num_tracks);
            for (int s = 0; s < MAX_STEPS; s++) {
                tr->patterns[p].steps[s].on       = 0;
                tr->patterns[p].steps[s].note     = 60;
                tr->patterns[p].steps[s].velocity = 0.8f;
                tr->patterns[p].steps[s].duration = 0.1f;
            }
        }
    }
}

void seq_randomize_track(Sequencer *seq, int track_idx, int pattern_idx) {
    if (track_idx < 0 || track_idx >= MAX_TRACKS) return;
    Track   *tr  = &seq->tracks[track_idx];
    Pattern *pat = &tr->patterns[pattern_idx];
    const int *sc  = scales[seq->cur_scale];
    int        slen = scale_lens[seq->cur_scale];

    float density = (track_idx == 0) ? 0.4f
                  : (track_idx == 1) ? 0.3f
                  : 0.25f;

    for (int s = 0; s < MAX_STEPS; s++) {
        pat->steps[s].on = ((float)rand() / RAND_MAX) < density;
        if (pat->steps[s].on) {
            int degree   = rand() % slen;
            int octshift = (rand() % 2) * 12;
            pat->steps[s].note     = seq->root_note + sc[degree] + octshift
                                     + (tr->octave - 4) * 12;
            pat->steps[s].velocity = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;
            pat->steps[s].duration = 0.05f + ((float)rand() / RAND_MAX) * 0.2f;
        }
    }
}

void seq_generate_groove(Sequencer *seq) {
    for (int t = 0; t < seq->num_tracks; t++) {
        for (int p = 0; p < MAX_PATTERNS; p++) {
            seq_randomize_track(seq, t, p);
        }
    }
    seq->tracks[0].patterns[0].steps[0].on  = 1;
    seq->tracks[0].patterns[0].steps[8].on  = 1;
    seq->tracks[0].patterns[0].steps[0].velocity = 1.0f;
}

void seq_tick(Sequencer *seq, Synth *synth, float dt) {
    if (!seq->playing) return;

    float beat_dur = 60.0f / seq->bpm / 4.0f;
    seq->beat_acc += dt;

    if (seq->beat_acc < beat_dur) return;
    seq->beat_acc -= beat_dur;
    seq->cur_beat  = (seq->cur_beat + 1) % MAX_STEPS;

    for (int t = 0; t < seq->num_tracks; t++) {
        Track   *tr  = &seq->tracks[t];
        Pattern *pat = &tr->patterns[tr->cur_pattern];
        if (pat->muted) continue;

        Step *st = &pat->steps[seq->cur_beat];
        if (!st->on) continue;

        float freq = midi_to_freq(st->note);
        synth_note_on(synth, t, freq, tr->volume,
                      st->velocity, st->duration,
                      tr->attack, tr->decay, tr->sustain, tr->release,
                      tr->wave_type);
    }

    seq->tick++;
}
