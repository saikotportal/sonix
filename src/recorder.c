#include "recorder.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static FILE  *rec_file   = NULL;
static long   rec_frames = 0;
static char   rec_path[256];

static void write_u16le(FILE *f, uint16_t v) {
    fputc(v & 0xFF, f);
    fputc((v >> 8) & 0xFF, f);
}

static void write_u32le(FILE *f, uint32_t v) {
    fputc(v & 0xFF, f);
    fputc((v >> 8)  & 0xFF, f);
    fputc((v >> 16) & 0xFF, f);
    fputc((v >> 24) & 0xFF, f);
}

static void write_wav_header(FILE *f, long num_frames) {
    uint32_t data_size   = (uint32_t)(num_frames * 2);
    uint32_t chunk_size  = 36 + data_size;

    fwrite("RIFF", 1, 4, f);
    write_u32le(f, chunk_size);
    fwrite("WAVE", 1, 4, f);

    fwrite("fmt ", 1, 4, f);
    write_u32le(f, 16);
    write_u16le(f, 1);
    write_u16le(f, 1);
    write_u32le(f, 44100);
    write_u32le(f, 44100 * 2);
    write_u16le(f, 2);
    write_u16le(f, 16);

    fwrite("data", 1, 4, f);
    write_u32le(f, data_size);
}

int rec_start(const char *path) {
    if (rec_file) return 0;

    rec_file = fopen(path, "wb");
    if (!rec_file) return 0;

    strncpy(rec_path, path, sizeof(rec_path) - 1);
    rec_frames = 0;

    write_wav_header(rec_file, 0);
    return 1;
}

void rec_write(int16_t *buf, int frames) {
    if (!rec_file) return;
    fwrite(buf, sizeof(int16_t), frames, rec_file);
    rec_frames += frames;
}

void rec_stop(void) {
    if (!rec_file) return;

    rewind(rec_file);
    write_wav_header(rec_file, rec_frames);
    fclose(rec_file);
    rec_file   = NULL;
    rec_frames = 0;
}

int rec_is_active(void) {
    return rec_file != NULL;
}

long rec_frame_count(void) {
    return rec_frames;
}
