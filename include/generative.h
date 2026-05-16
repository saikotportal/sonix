#ifndef GENERATIVE_H
#define GENERATIVE_H

#include "sequencer.h"

void gen_euclidean_pattern(Sequencer *seq, int track_idx, int pattern_idx,
                            int pulses);
void gen_markov_melody(Sequencer *seq, int track_idx, int pattern_idx);
void gen_auto_evolve(Sequencer *seq, long tick);

#endif
