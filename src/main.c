#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>

#include "sonix.h"
#include "synth.h"
#include "sequencer.h"
#include "generative.h"
#include "audio.h"
#include "ui.h"
#include "input.h"
#include "effects.h"
#include "recorder.h"

static volatile int running = 1;
static struct termios orig_termios;

static void restore_term(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\033[?25h\033[0m\n");
}

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

static void setup_term(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restore_term);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\033[?25l");
}

static int read_key(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;

    if (c == 27) {
        unsigned char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
            read(STDIN_FILENO, &seq[1], 1) == 1 &&
            seq[0] == '[') {
            if (seq[1] == 'A') return KEY_UP;
            if (seq[1] == 'B') return KEY_DOWN;
        }
        return 27;
    }
    return (int)c;
}

static float now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec / 1e9f;
}

static void sleep_ms(int ms) {
    struct timespec ts = { .tv_sec = 0, .tv_nsec = ms * 1000000L };
    nanosleep(&ts, NULL);
}

int main(void) {
    srand((unsigned)time(NULL));
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    setup_term();

    Sequencer seq;
    Synth     synth;
    seq_init(&seq);
    synth_init(&synth);
    fx_init();

    FxParams fx = {
        .reverb_wet   = 0.25f,
        .reverb_decay = 0.5f,
        .lp_cutoff    = 20000.0f,
    };

    seq.num_tracks = 6;
    seq.bpm        = 120.0f;
    seq_generate_groove(&seq);

    int   selected   = 0;
    char  status[128] = "Welcome to SONIX. Press [space] to start.";
    float last_time  = now_seconds();
    float ui_timer   = 0.0f;

    int16_t audio_buf[BUF_FRAMES];

    int has_audio = audio_open();
    if (!has_audio) {
        snprintf(status, sizeof(status),
                 "No audio output (ffplay missing). Visual mode only.");
    }

    while (running) {
        float now = now_seconds();
        float dt  = now - last_time;
        last_time = now;

        int key = read_key();
        switch (key) {
            case 'q': case 'Q': case 27:
                running = 0;
                break;

            case 'x': case 'X':
                if (rec_is_active()) {
                    rec_stop();
                    snprintf(status, sizeof(status), "Recording saved.");
                } else {
                    rec_start("sonix_recording.wav");
                    snprintf(status, sizeof(status), "Recording started → sonix_recording.wav");
                }
                break;

            case 'f': case 'F':
                fx.lp_cutoff = (fx.lp_cutoff < 2000.0f) ? 20000.0f : fx.lp_cutoff * 0.5f;
                snprintf(status, sizeof(status), "LP cutoff: %.0f Hz", fx.lp_cutoff);
                break;

            case 'v': case 'V':
                fx.reverb_wet = (fx.reverb_wet > 0.05f) ? 0.0f : 0.3f;
                snprintf(status, sizeof(status), "Reverb: %s", fx.reverb_wet > 0.0f ? "ON" : "OFF");
                break;

            case ' ':
                seq.playing = !seq.playing;
                snprintf(status, sizeof(status),
                         seq.playing ? "Playing." : "Stopped.");
                break;

            case 'g': case 'G':
                seq_generate_groove(&seq);
                snprintf(status, sizeof(status), "New groove generated.");
                break;

            case 'e': case 'E':
                gen_auto_evolve(&seq, seq.tick);
                snprintf(status, sizeof(status), "Auto-evolve triggered.");
                break;

            case 'r': case 'R':
                seq_randomize_track(&seq, selected,
                    seq.tracks[selected].cur_pattern);
                snprintf(status, sizeof(status),
                         "Track %d randomized.", selected);
                break;

            case 'm': case 'M': {
                Track   *tr  = &seq.tracks[selected];
                Pattern *pat = &tr->patterns[tr->cur_pattern];
                pat->muted = !pat->muted;
                snprintf(status, sizeof(status),
                         "Track %d %s.", selected,
                         pat->muted ? "muted" : "unmuted");
                break;
            }

            case 'w': case 'W':
                seq.tracks[selected].wave_type =
                    (seq.tracks[selected].wave_type + 1) % WAVE_COUNT;
                snprintf(status, sizeof(status),
                         "Wave: %s",
                         wave_names[seq.tracks[selected].wave_type]);
                break;

            case 's': case 'S':
                seq.cur_scale = (seq.cur_scale + 1) % 5;
                snprintf(status, sizeof(status),
                         "Scale: %s", scale_names[seq.cur_scale]);
                break;

            case 'o': case 'O':
                seq.tracks[selected].octave =
                    (seq.tracks[selected].octave % 7) + 1;
                snprintf(status, sizeof(status),
                         "Octave: %d", seq.tracks[selected].octave);
                break;

            case '+': case '=':
                seq.bpm += 5.0f;
                if (seq.bpm > 300.0f) seq.bpm = 300.0f;
                snprintf(status, sizeof(status), "BPM: %.0f", seq.bpm);
                break;

            case '-':
                seq.bpm -= 5.0f;
                if (seq.bpm < 40.0f) seq.bpm = 40.0f;
                snprintf(status, sizeof(status), "BPM: %.0f", seq.bpm);
                break;

            case KEY_UP:
                selected = (selected - 1 + seq.num_tracks) % seq.num_tracks;
                snprintf(status, sizeof(status),
                         "Selected: %s", seq.tracks[selected].name);
                break;

            case KEY_DOWN:
                selected = (selected + 1) % seq.num_tracks;
                snprintf(status, sizeof(status),
                         "Selected: %s", seq.tracks[selected].name);
                break;

            default: break;
        }

        seq_tick(&seq, &synth, dt);

        synth_render(&synth, audio_buf, BUF_FRAMES);
        fx_apply(audio_buf, BUF_FRAMES, &fx);

        if (rec_is_active())
            rec_write(audio_buf, BUF_FRAMES);

        if (has_audio) {
            audio_write(audio_buf, BUF_FRAMES);
        }

        float rms = 0.0f;
        for (int i = 0; i < BUF_FRAMES; i++) {
            float s = audio_buf[i] / 32768.0f;
            rms += s * s;
        }
        rms = sqrtf(rms / BUF_FRAMES) * 4.0f;

        for (int t = 0; t < seq.num_tracks; t++) {
            if (synth.voices[t].active) {
                ui_update_vu(t, rms * synth.voices[t].amplitude);
            } else {
                ui_update_vu(t, 0.0f);
            }
        }

        ui_timer += dt;
        if (ui_timer >= 0.05f) {
            ui_timer = 0.0f;
            ui_draw(&seq, &synth, selected, status);
        }

        sleep_ms(4);
    }

    audio_close();
    if (rec_is_active()) rec_stop();
    printf("\n  Goodbye.\n\n");
    return 0;
}
