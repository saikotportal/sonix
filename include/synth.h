#ifndef SYNTH_H
#define SYNTH_H

#include "sonix.h"
#include <stdint.h>

typedef struct {
    float frequency;
    float amplitude;
    float velocity;
    float duration;
    float attack;
    float decay;
    float sustain;
    float release;
    float time;
    int   wave_type;
    int   active;
} SynthVoice;

typedef struct {
    SynthVoice voices[MAX_TRACKS];
    int        num_voices;
} Synth;

void synth_init(Synth *s);
void synth_render(Synth *s, int16_t *buf, int frames);
void synth_note_on(Synth *s, int track_idx, float freq, float amp,
                   float velocity, float dur,
                   float att, float dec, float sus, float rel,
                   int wave);

#endif
