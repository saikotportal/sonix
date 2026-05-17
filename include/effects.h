#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>

typedef struct {
    float reverb_wet;
    float reverb_decay;
    float lp_cutoff;
} FxParams;
   
void fx_init(void);
void fx_lowpass(int16_t *buf, int frames, float cutoff);
void fx_reverb(int16_t *buf, int frames, float wet, float decay);
void fx_apply(int16_t *buf, int frames, FxParams *p);

#endif
