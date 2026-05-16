#include "effects.h"
#include <math.h>
#include <string.h>

#define REVERB_SIZE 22050

static float reverb_buf[REVERB_SIZE];
static int   reverb_pos = 0;

static float lp_prev = 0.0f;

void fx_init(void) {
    memset(reverb_buf, 0, sizeof(reverb_buf));
    reverb_pos = 0;
    lp_prev    = 0.0f;
}

void fx_lowpass(int16_t *buf, int frames, float cutoff) {
    float rc  = 1.0f / (cutoff * 2.0f * 3.14159f);
    float dt  = 1.0f / 44100.0f;
    float alpha = dt / (rc + dt);

    for (int i = 0; i < frames; i++) {
        float s = buf[i] / 32768.0f;
        lp_prev = lp_prev + alpha * (s - lp_prev);
        int32_t out = (int32_t)(lp_prev * 32767.0f);
        if (out >  32767) out =  32767;
        if (out < -32768) out = -32768;
        buf[i] = (int16_t)out;
    }
}

void fx_reverb(int16_t *buf, int frames, float wet, float decay) {
    for (int i = 0; i < frames; i++) {
        float dry = buf[i] / 32768.0f;
        float rev = reverb_buf[reverb_pos];

        float mixed = dry + wet * rev;
        reverb_buf[reverb_pos] = dry + decay * rev;
        reverb_pos = (reverb_pos + 1) % REVERB_SIZE;

        int32_t out = (int32_t)(mixed * 32767.0f);
        if (out >  32767) out =  32767;
        if (out < -32768) out = -32768;
        buf[i] = (int16_t)out;
    }
}

void fx_apply(int16_t *buf, int frames, FxParams *p) {
    if (p->reverb_wet > 0.001f)
        fx_reverb(buf, frames, p->reverb_wet, p->reverb_decay);
    if (p->lp_cutoff < 20000.0f)
        fx_lowpass(buf, frames, p->lp_cutoff);
}
