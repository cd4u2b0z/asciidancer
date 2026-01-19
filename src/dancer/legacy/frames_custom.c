/*
 * Custom dancer frames - loads frames from dancer_frames.txt
 * 8 frames total: 4 in top half (lines 1-13), 4 in bottom half (lines 14-26)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include "dancer.h"

#define MAX_FRAMES 8
#define FILE_FRAME_WIDTH 25
#define FILE_FRAME_HEIGHT 13
#define CHARS_PER_LINE 100
#define LINE_BUFFER_SIZE 512

static wchar_t frames[MAX_FRAMES][FILE_FRAME_HEIGHT][FILE_FRAME_WIDTH + 1];
static int num_frames = 0;
static int frame_loaded = 0;

static const char* find_data_file(void) {
    static const char* search_paths[] = {
        "./dancer_frames.txt",
        "../dancer_frames.txt",
        "/home/craig/projects/asciidancer/dancer_frames.txt",
        NULL
    };
    
    for (int i = 0; search_paths[i] != NULL; i++) {
        FILE* f = fopen(search_paths[i], "r");
        if (f) {
            fclose(f);
            return search_paths[i];
        }
    }
    return NULL;
}

int dancer_load_frames(void) {
    if (frame_loaded) return num_frames;
    
    setlocale(LC_ALL, "");
    
    const char* filepath = find_data_file();
    if (!filepath) {
        fprintf(stderr, "Error: Cannot find dancer_frames.txt\n");
        return -1;
    }
    
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", filepath);
        return -1;
    }
    
    wchar_t all_lines[26][CHARS_PER_LINE + 1];
    memset(all_lines, 0, sizeof(all_lines));
    
    char line_buf[LINE_BUFFER_SIZE];
    int line_count = 0;
    
    while (fgets(line_buf, sizeof(line_buf), f) && line_count < 26) {
        size_t len = strlen(line_buf);
        if (len > 0 && line_buf[len-1] == '\n') {
            line_buf[len-1] = '\0';
        }
        mbstowcs(all_lines[line_count], line_buf, CHARS_PER_LINE);
        line_count++;
    }
    fclose(f);
    
    /* Extract frames - skip first 2 chars of each frame to remove stray dots */
    for (int fr = 0; fr < 4; fr++) {
        for (int row = 0; row < FILE_FRAME_HEIGHT && row < line_count; row++) {
            int start = fr * FILE_FRAME_WIDTH + 2;  /* +2 to skip stray left dots */
            for (int i = 0; i < FILE_FRAME_WIDTH; i++) {
                frames[fr][row][i] = L'⠀';
            }
            frames[fr][row][FILE_FRAME_WIDTH] = L'\0';
            wcsncpy(frames[fr][row], all_lines[row] + start, FILE_FRAME_WIDTH - 2);
        }
    }
    
    for (int fr = 0; fr < 4; fr++) {
        for (int row = 0; row < FILE_FRAME_HEIGHT && (row + 13) < line_count; row++) {
            int start = fr * FILE_FRAME_WIDTH + 2;  /* +2 to skip stray left dots */
            for (int i = 0; i < FILE_FRAME_WIDTH; i++) {
                frames[fr + 4][row][i] = L'⠀';
            }
            frames[fr + 4][row][FILE_FRAME_WIDTH] = L'\0';
            wcsncpy(frames[fr + 4][row], all_lines[row + 13] + start, FILE_FRAME_WIDTH - 2);
        }
    }
    
    num_frames = 8;
    frame_loaded = 1;
    return num_frames;
}

static const wchar_t* dancer_get_frame_line(int frame_idx, int line_idx) {
    if (!frame_loaded) dancer_load_frames();
    if (frame_idx < 0 || frame_idx >= num_frames) frame_idx = 0;
    if (line_idx < 0 || line_idx >= FILE_FRAME_HEIGHT) return L"";
    return frames[frame_idx][line_idx];
}

int dancer_get_frame_count(void) {
    if (!frame_loaded) dancer_load_frames();
    return num_frames;
}

int dancer_get_frame_height(void) { return FILE_FRAME_HEIGHT; }
int dancer_get_frame_width(void) { return FILE_FRAME_WIDTH; }

/* Smooth dance animation - changes on beats, with fallback timer */
int dancer_select_frame(float bass, float mid, float treble) {
    if (!frame_loaded) dancer_load_frames();
    
    static int current_frame = 0;
    static int hold_timer = 0;
    static int idle_timer = 0;
    static float smoothed_bass = 0.0f;
    static float smoothed_energy = 0.0f;
    
    /* Smooth the inputs */
    smoothed_bass = smoothed_bass * 0.85f + bass * 0.15f;
    float energy = bass * 0.5f + mid * 0.3f + treble * 0.2f;
    smoothed_energy = smoothed_energy * 0.8f + energy * 0.2f;
    
    /* Detect beat - bass above smoothed average */
    int beat_detected = (bass > smoothed_bass * 1.3f) && (bass > 0.15f);
    
    /* Hold timer prevents too-rapid changes */
    if (hold_timer > 0) {
        hold_timer--;
        idle_timer++;
    } else {
        idle_timer++;
    }
    
    /* Change frame on beat OR after idle timeout (keeps dancing even in quiet parts) */
    int should_change = (beat_detected && hold_timer == 0) || (idle_timer > 30);
    
    if (should_change) {
        idle_timer = 0;
        
        /* Step to next frame in sequence */
        if (smoothed_energy > 0.35f) {
            /* High energy - energetic frames 4-7 */
            if (current_frame < 4) current_frame = 4;
            else current_frame = 4 + ((current_frame - 4 + 1) % 4);
        } else if (smoothed_energy > 0.15f) {
            /* Medium - all frames */
            current_frame = (current_frame + 1) % 8;
        } else {
            /* Low energy - calm frames 0-3 */
            if (current_frame >= 4) current_frame = 0;
            else current_frame = (current_frame + 1) % 4;
        }
        
        /* Hold for ~100ms at 60fps */
        hold_timer = 6;
    }
    
    return current_frame;
}

void dancer_frame_to_utf8(int frame_idx, int line_idx, char* out, size_t out_size) {
    const wchar_t* wline = dancer_get_frame_line(frame_idx, line_idx);
    wcstombs(out, wline, out_size);
}
