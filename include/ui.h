#ifndef UI_H
#define UI_H

#include "sequencer.h"
#include "synth.h"

void ui_draw(Sequencer *seq, Synth *synth, int selected_track,
             const char *status);
void ui_update_vu(int track, float level);
void ui_flash_step(int track, int step);

#endif
