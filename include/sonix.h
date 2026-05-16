#ifndef SONIX_H
#define SONIX_H

#include <stdint.h>
#include <stdlib.h>

#define SAMPLE_RATE     44100
#define CHANNELS        1
#define BITS            16
#define BUF_FRAMES      512

#define MAX_TRACKS      8
#define MAX_STEPS       16
#define MAX_PATTERNS    8
#define NOTE_OFF        0
#define NOTE_ON         1

typedef struct {
    int   note;
    int   on;
    float velocity;
    float duration;
} Step;

typedef struct {
    Step  steps[MAX_STEPS];
    int   length;
    int   muted;
} Pattern;

typedef struct {
    Pattern patterns[MAX_PATTERNS];
    int     cur_pattern;
    int     cur_step;
    float   volume;
    int     octave;
    char    name[16];
    int     wave_type;
    float   attack;
    float   decay;
    float   sustain;
    float   release;
} Track;

typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_TRIANGLE,
    WAVE_NOISE,
    WAVE_COUNT
} WaveType;

extern const char *wave_names[WAVE_COUNT];
extern const char *note_names[12];

float midi_to_freq(int midi_note);

#endif
