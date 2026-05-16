#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "sonix.h"
#include "synth.h"

typedef struct {
    Track   tracks[MAX_TRACKS];
    int     num_tracks;
    float   bpm;
    int     playing;
    int     cur_beat;
    long    tick;
    float   beat_acc;
    int     cur_scale;
    int     root_note;
} Sequencer;

extern const char *scale_names[];

void seq_init(Sequencer *seq);
void seq_tick(Sequencer *seq, Synth *synth, float dt);
void seq_generate_groove(Sequencer *seq);
void seq_randomize_track(Sequencer *seq, int track_idx, int pattern_idx);

#endif
