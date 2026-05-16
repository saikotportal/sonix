#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

int  audio_open(void);
void audio_write(int16_t *buf, int frames);
void audio_close(void);

#endif
