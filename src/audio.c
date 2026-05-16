#include "audio.h"
#include "sonix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *audio_pipe = NULL;

int audio_open(void) {
    const char *cmd =
        "ffplay -f s16le -ar 44100 -ac 1 -nodisp -loglevel quiet -i pipe:0 2>/dev/null";
    audio_pipe = popen(cmd, "w");
    return audio_pipe != NULL;
}

void audio_write(int16_t *buf, int frames) {
    if (!audio_pipe) return;
    fwrite(buf, sizeof(int16_t), frames, audio_pipe);
    fflush(audio_pipe);
}

void audio_close(void) {
    if (audio_pipe) {
        pclose(audio_pipe);
        audio_pipe = NULL;
    }
}
