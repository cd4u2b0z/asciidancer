/*
 * Frame Recorder Implementation
 */

#include "frame_recorder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <ncurses.h>

/* Create output directory if it doesn't exist */
static void ensure_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0755);
    }
}

FrameRecorder* frame_recorder_create(int width, int height, const char *output_dir) {
    /* Validate dimensions */
    if (width <= 0 || height <= 0) return NULL;
    
    FrameRecorder *rec = calloc(1, sizeof(FrameRecorder));
    if (!rec) return NULL;
    
    rec->width = width;
    rec->height = height;
    rec->recording = false;
    rec->frame_number = 0;
    rec->total_frames = 0;
    
    /* Use default directory if none provided */
    const char *dir = output_dir;
    if (!dir || dir[0] == '\0') {
        const char *home = getenv("HOME");
        if (home) {
            static char default_dir[256];
            snprintf(default_dir, sizeof(default_dir), "%s/asciidancer_recordings", home);
            dir = default_dir;
        } else {
            dir = "/tmp/asciidancer_recordings";
        }
    }
    memset(rec->output_dir, 0, sizeof(rec->output_dir));
    snprintf(rec->output_dir, sizeof(rec->output_dir), "%s", dir);
    
    /* Allocate frame buffer */
    rec->frame_buffer = calloc(height, sizeof(char*));
    if (!rec->frame_buffer) {
        free(rec);
        return NULL;
    }
    
    for (int i = 0; i < height; i++) {
        rec->frame_buffer[i] = calloc(width * 20, sizeof(char)); /* Allow for ANSI codes */
        if (!rec->frame_buffer[i]) {
            for (int j = 0; j < i; j++) free(rec->frame_buffer[j]);
            free(rec->frame_buffer);
            free(rec);
            return NULL;
        }
    }
    
    ensure_directory(rec->output_dir);
    return rec;
}

void frame_recorder_destroy(FrameRecorder *recorder) {
    if (!recorder) return;
    
    if (recorder->frame_buffer && recorder->height > 0) {
        for (int i = 0; i < recorder->height; i++) {
            if (recorder->frame_buffer[i]) {
                free(recorder->frame_buffer[i]);
                recorder->frame_buffer[i] = NULL;
            }
        }
        free(recorder->frame_buffer);
        recorder->frame_buffer = NULL;
    }
    
    free(recorder);
}

void frame_recorder_start(FrameRecorder *recorder) {
    if (!recorder) return;
    
    recorder->recording = true;
    recorder->frame_number = 0;
    recorder->total_frames = 0;
    recorder->start_time = (double)clock() / CLOCKS_PER_SEC;
    
    /* Create timestamped subdirectory */
    time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    char subdir[sizeof(recorder->output_dir)];
    int written = snprintf(subdir, sizeof(subdir), "%s/recording_%04d%02d%02d_%02d%02d%02d",
             recorder->output_dir,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    if (written >= (int)sizeof(subdir)) subdir[sizeof(subdir) - 1] = '\0';
    memcpy(recorder->output_dir, subdir, sizeof(recorder->output_dir));
    ensure_directory(recorder->output_dir);
}

void frame_recorder_stop(FrameRecorder *recorder) {
    if (!recorder) return;
    
    recorder->recording = false;
    recorder->duration = ((double)clock() / CLOCKS_PER_SEC) - recorder->start_time;
    
    /* Write summary file */
    char summary_path[280];
    snprintf(summary_path, sizeof(summary_path), "%s/summary.txt", recorder->output_dir);
    
    FILE *f = fopen(summary_path, "w");
    if (f) {
        fprintf(f, "ASCII Dancer Recording\n");
        fprintf(f, "======================\n");
        fprintf(f, "Frames: %d\n", recorder->total_frames);
        fprintf(f, "Duration: %.2f seconds\n", recorder->duration);
        fprintf(f, "FPS: %.2f\n", recorder->total_frames / recorder->duration);
        fprintf(f, "\nTo create GIF:\n");
        fprintf(f, "  cat frame_*.txt | convert -delay 1.67 -loop 0 txt:- output.gif\n");
        fprintf(f, "\nTo create video:\n");
        fprintf(f, "  # Use asciinema or similar tool\n");
        fclose(f);
    }
}

bool frame_recorder_is_recording(FrameRecorder *recorder) {
    return recorder ? recorder->recording : false;
}

void frame_recorder_capture(FrameRecorder *recorder) {
    if (!recorder || !recorder->recording) return;
    
    /* Capture current screen content using ncurses */
    char filename[280];
    snprintf(filename, sizeof(filename), "%s/frame_%06d.txt", 
             recorder->output_dir, recorder->frame_number);
    
    FILE *f = fopen(filename, "w");
    if (!f) return;
    
    /* Read screen content with colors */
    for (int y = 0; y < LINES && y < recorder->height; y++) {
        for (int x = 0; x < COLS && x < recorder->width; x++) {
            chtype ch = mvinch(y, x);
            
            /* Extract character and attributes */
            char c = ch & A_CHARTEXT;
            (void)(ch & A_ATTRIBUTES);  /* attrs reserved for future use */
            short pair_num = PAIR_NUMBER(ch);
            
            /* Write with ANSI color codes if colored */
            if (pair_num > 0) {
                short fg, bg;
                pair_content(pair_num, &fg, &bg);
                
                /* Convert to ANSI */
                if (fg >= 0) {
                    fprintf(f, "\033[38;5;%dm", fg);
                }
            }
            
            fputc(c ? c : ' ', f);
        }
        fprintf(f, "\033[0m\n"); /* Reset colors at line end */
    }
    
    fclose(f);
    
    recorder->frame_number++;
    recorder->total_frames++;
}

void frame_recorder_get_stats(const FrameRecorder *recorder, int *frames, double *duration) {
    if (!recorder) return;
    if (frames) *frames = recorder->total_frames;
    if (duration) {
        if (recorder->recording) {
            *duration = ((double)clock() / CLOCKS_PER_SEC) - recorder->start_time;
        } else {
            *duration = recorder->duration;
        }
    }
}
