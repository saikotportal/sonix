#include "ui.h"
#include "sonix.h"
#include "sequencer.h"
#include "synth.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define CLR         "\033[2J\033[H"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define RST         "\033[0m"
#define RED         "\033[31m"
#define GRN         "\033[32m"
#define YLW         "\033[33m"
#define BLU         "\033[34m"
#define MAG         "\033[35m"
#define CYN         "\033[36m"
#define WHT         "\033[37m"
#define BRED        "\033[91m"
#define BGRN        "\033[92m"
#define BYLW        "\033[93m"
#define BBLU        "\033[94m"
#define BMAG        "\033[95m"
#define BCYN        "\033[96m"
#define BWHT        "\033[97m"
#define BG_DARK     "\033[48;5;234m"
#define BG_MID      "\033[48;5;236m"
#define BG_ACCENT   "\033[48;5;17m"

static const char *track_colors[MAX_TRACKS] = {
    BRED, BGRN, BCYN, BMAG, BYLW, BBLU, BWHT, RED
};

static float vu_levels[MAX_TRACKS] = {0};

void ui_update_vu(int track, float level) {
    vu_levels[track] = vu_levels[track] * 0.85f + level * 0.15f;
    if (level > vu_levels[track]) vu_levels[track] = level;
}

static void draw_vu(float level, int width) {
    int filled = (int)(level * width);
    if (filled > width) filled = width;

    for (int i = 0; i < width; i++) {
        float pos = (float)i / width;
        if (i < filled) {
            if (pos > 0.8f) printf(RED  "█" RST);
            else if (pos > 0.5f) printf(YLW  "█" RST);
            else             printf(BGRN "█" RST);
        } else {
            printf(DIM "░" RST);
        }
    }
}

static void draw_step_grid(Sequencer *seq, int track_idx) {
    Track   *tr  = &seq->tracks[track_idx];
    Pattern *pat = &tr->patterns[tr->cur_pattern];

    for (int s = 0; s < MAX_STEPS; s++) {
        int is_cur = (s == seq->cur_beat && seq->playing);
        int on     = pat->steps[s].on;

        if (is_cur && on) {
            printf(BG_ACCENT "%s●" RST " ", track_colors[track_idx]);
        } else if (is_cur) {
            printf(BYLW "▶" RST " ");
        } else if (on) {
            float vel = pat->steps[s].velocity;
            if (vel > 0.8f)      printf("%s■" RST " ", track_colors[track_idx]);
            else if (vel > 0.5f) printf("%s▪" RST " ", track_colors[track_idx]);
            else                 printf("%s·" RST " ", track_colors[track_idx]);
        } else {
            printf(DIM "·" RST " ");
        }

        if (s == 3 || s == 7 || s == 11) printf(" ");
    }
}

static const char *note_name_for(int midi) {
    static char buf[8];
    int pc  = midi % 12;
    int oct = midi / 12 - 1;
    snprintf(buf, sizeof(buf), "%s%d", note_names[pc], oct);
    return buf;
}

void ui_draw(Sequencer *seq, Synth *synth, int selected_track,
             const char *status) {
    printf(CLR);

    printf(BOLD BCYN);
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║   S O N I X  —  generative music engine                 ║\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\n");
    printf(RST "\n");

    printf("  " BOLD "BPM:" RST " " BYLW "%.0f" RST
           "   " BOLD "SCALE:" RST " " BCYN "%s" RST
           "   " BOLD "ROOT:" RST " " BMAG "%s" RST
           "   " BOLD "TICK:" RST " " DIM "%ld" RST
           "   %s\n\n",
           seq->bpm,
           scale_names[seq->cur_scale],
           note_names[seq->root_note % 12],
           seq->tick,
           seq->playing ? (BGRN "▶  PLAYING" RST) : (BRED "■  STOPPED" RST));

    printf("  " BOLD "TRACKS\n" RST);
    printf("  ────────────────────────────────────────────────────────────\n");

    for (int t = 0; t < seq->num_tracks; t++) {
        Track   *tr  = &seq->tracks[t];
        Pattern *pat = &tr->patterns[tr->cur_pattern];
        int      is_sel = (t == selected_track);
        int      muted  = pat->muted;

        if (is_sel) printf(BG_MID);

        printf("  %s%s%s ",
               is_sel ? (BOLD BCYN "▶ ") : "  ",
               track_colors[t],
               tr->name);
        printf(RST);
        if (is_sel) printf(BG_MID);

        if (muted) {
            printf(DIM " [MUTE] " RST);
            if (is_sel) printf(BG_MID);
        } else {
            printf(" ");
        }

        printf("  ");
        if (is_sel) printf(BG_MID);

        draw_step_grid(seq, t);

        printf(" ");
        draw_vu(vu_levels[t], 8);

        printf("  " DIM "%s" RST, wave_names[tr->wave_type]);

        if (synth->voices[t].active) {
            printf("  %s%s" RST,
                   track_colors[t],
                   note_name_for(
                       (int)(69 + 12 * log2f(
                           synth->voices[t].frequency / 440.0f))));
        }

        printf(RST "\n");
    }

    printf("\n");
    printf("  ────────────────────────────────────────────────────────────\n");
    printf("  " BOLD "CONTROLS\n" RST);
    printf("  " DIM
           "  [space] play/stop   [g] generate   [e] evolve   [q] quit\n"
           "  [↑/↓]   select trk  [m] mute trk   [r] randomize trk\n"
           "  [+/-]   BPM         [w] wave cycle  [s] scale    [o] octave\n"
           "  [x]     record/stop  [f] LP filter   [v] reverb\n"
           RST "\n");

    if (status && status[0]) {
        printf("  " BYLW "▸ %s" RST "\n", status);
    }

    fflush(stdout);
}

void ui_flash_step(int track, int step) {
    vu_levels[track] = 1.0f;
    (void)step;
}
