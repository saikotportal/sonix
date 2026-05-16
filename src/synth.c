#include "sonix.h"
#include "synth.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TWO_PI (2.0f * 3.14159265358979f)

const char *wave_names[WAVE_COUNT] = {
    "SINE", "SQUARE", "SAW", "TRIANGLE", "NOISE"
};

const char *note_names[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

float midi_to_freq(int midi_note) {
    return 440.0f * powf(2.0f, (midi_note - 69) / 12.0f);
}

static float osc_phase[MAX_TRACKS] = {0};

static float synth_wave(float phase, WaveType type) {
    switch (type) {
        case WAVE_SINE:
            return sinf(phase * TWO_PI);

        case WAVE_SQUARE:
            return (phase < 0.5f) ? 1.0f : -1.0f;

        case WAVE_SAW:
            return 2.0f * phase - 1.0f;

        case WAVE_TRIANGLE:
            return (phase < 0.5f)
                ? (4.0f * phase - 1.0f)
                : (3.0f - 4.0f * phase);

        case WAVE_NOISE:
            return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;

        default:
            return 0.0f;
    }
}

static float adsr_envelope(float t, float attack, float decay,
                            float sustain, float release,
                            float duration) {
    float env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (1.0f - sustain) * ((t - attack) / decay);
    } else if (t < duration - release) {
        env = sustain;
    } else if (t < duration) {
        env = sustain * (1.0f - (t - (duration - release)) / release);
    } else {
        env = 0.0f;
    }
    return env < 0.0f ? 0.0f : env;
}

void synth_render(Synth *s, int16_t *buf, int frames) {
    memset(buf, 0, frames * sizeof(int16_t));

    for (int t = 0; t < s->num_voices; t++) {
        SynthVoice *v = &s->voices[t];
        if (!v->active) continue;

        float inc = v->frequency / (float)SAMPLE_RATE;

        for (int i = 0; i < frames; i++) {
            float env = adsr_envelope(
                v->time + (float)i / SAMPLE_RATE,
                v->attack, v->decay, v->sustain, v->release,
                v->duration
            );

            float sample = synth_wave(osc_phase[t], (WaveType)v->wave_type);
            sample *= env * v->amplitude * v->velocity;

            osc_phase[t] += inc;
            if (osc_phase[t] >= 1.0f) osc_phase[t] -= 1.0f;

            int32_t mixed = (int32_t)buf[i] + (int32_t)(sample * 24000.0f);
            if (mixed >  32767) mixed =  32767;
            if (mixed < -32768) mixed = -32768;
            buf[i] = (int16_t)mixed;
        }

        v->time += (float)frames / SAMPLE_RATE;
        if (v->time >= v->duration) {
            v->active = 0;
        }
    }
}

void synth_note_on(Synth *s, int track_idx, float freq, float amp,
                   float velocity, float dur,
                   float att, float dec, float sus, float rel,
                   int wave) {
    if (track_idx < 0 || track_idx >= MAX_TRACKS) return;
    SynthVoice *v = &s->voices[track_idx];
    v->frequency = freq;
    v->amplitude = amp;
    v->velocity  = velocity;
    v->duration  = dur;
    v->attack    = att;
    v->decay     = dec;
    v->sustain   = sus;
    v->release   = rel;
    v->wave_type = wave;
    v->time      = 0.0f;
    v->active    = 1;
    osc_phase[track_idx] = 0.0f;
}

void synth_init(Synth *s) {
    memset(s, 0, sizeof(Synth));
    s->num_voices = MAX_TRACKS;
}
