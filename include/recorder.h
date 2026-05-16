#ifndef RECORDER_H
#define RECORDER_H

#include <stdint.h>

int  rec_start(const char *path);
void rec_write(int16_t *buf, int frames);
void rec_stop(void);
int  rec_is_active(void);
long rec_frame_count(void);

#endif
