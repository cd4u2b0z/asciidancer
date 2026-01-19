/*
 * Performance Profiler Implementation
 */

#include "profiler.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <math.h>
#include <stdatomic.h>

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

Profiler* profiler_create(void) {
    Profiler *prof = calloc(1, sizeof(Profiler));
    if (!prof) return NULL;
    
    prof->fps_min = 999999.0;
    prof->fps_max = 0.0;
    prof->x = 2;
    prof->y = 2;
    prof->enabled = false;
    
    return prof;
}

void profiler_destroy(Profiler *prof) {
    if (prof) free(prof);
}

/* Thread-safe frame timing using atomic operations */
static _Atomic double frame_start_time = 0.0;

void profiler_frame_start(Profiler *prof) {
    if (!prof) return;
    atomic_store(&frame_start_time, get_time_ms());
}

void profiler_frame_end(Profiler *prof) {
    if (!prof) return;
    
    double frame_end = get_time_ms();
    double frame_time = frame_end - atomic_load(&frame_start_time);
    
    /* Store frame time */
    prof->frame_times[prof->frame_index] = frame_time;
    prof->frame_index = (prof->frame_index + 1) % PROF_HISTORY_SIZE;
    
    /* Calculate current FPS */
    prof->frame_time_ms = frame_time;
    prof->fps_current = frame_time > 0 ? 1000.0 / frame_time : 0.0;
    
    /* Calculate average FPS from history */
    double sum = 0.0;
    double min_time = 999999.0;
    double max_time = 0.0;
    
    for (int i = 0; i < PROF_HISTORY_SIZE; i++) {
        double t = prof->frame_times[i];
        if (t > 0) {
            sum += t;
            if (t < min_time) min_time = t;
            if (t > max_time) max_time = t;
        }
    }
    
    prof->frame_time_avg_ms = sum / PROF_HISTORY_SIZE;
    prof->fps_average = prof->frame_time_avg_ms > 0 ? 
                       1000.0 / prof->frame_time_avg_ms : 0.0;
    
    prof->fps_min = max_time > 0 ? 1000.0 / max_time : 0.0;
    prof->fps_max = min_time > 0 ? 1000.0 / min_time : 0.0;
}

void profiler_mark_audio(Profiler *prof, double ms) {
    if (prof) prof->audio_time_ms = ms;
}

void profiler_mark_update(Profiler *prof, double ms) {
    if (prof) prof->update_time_ms = ms;
}

void profiler_mark_render(Profiler *prof, double ms) {
    if (prof) prof->render_time_ms = ms;
}

void profiler_set_counts(Profiler *prof, int particles, int trails) {
    if (!prof) return;
    prof->active_particles = particles;
    prof->trail_segments = trails;
}

void profiler_toggle(Profiler *prof) {
    if (prof) prof->enabled = !prof->enabled;
}

bool profiler_is_enabled(Profiler *prof) {
    return prof ? prof->enabled : false;
}

void profiler_render(Profiler *prof) {
    if (!prof || !prof->enabled) return;
    
    int x = prof->x;
    int y = prof->y;
    
    /* Draw box */
    attron(COLOR_PAIR(7) | A_BOLD);
    mvprintw(y, x, "╔═══════════════════════════╗");
    mvprintw(y + 1, x, "║ PERFORMANCE METRICS     ║");
    mvprintw(y + 2, x, "╟───────────────────────────╢");
    attroff(A_BOLD);
    
    /* FPS stats */
    mvprintw(y + 3, x, "║ FPS: %5.1f / %5.1f avg ║", 
             prof->fps_current, prof->fps_average);
    mvprintw(y + 4, x, "║ Min: %5.1f Max: %5.1f   ║",
             prof->fps_min, prof->fps_max);
    
    /* Frame time breakdown */
    mvprintw(y + 5, x, "╟───────────────────────────╢");
    mvprintw(y + 6, x, "║ Frame: %4.1fms (%4.1favg) ║",
             prof->frame_time_ms, prof->frame_time_avg_ms);
    mvprintw(y + 7, x, "║ Audio: %4.1fms           ║", prof->audio_time_ms);
    mvprintw(y + 8, x, "║ Update:%4.1fms           ║", prof->update_time_ms);
    mvprintw(y + 9, x, "║ Render:%4.1fms           ║", prof->render_time_ms);
    
    /* Object counts */
    mvprintw(y + 10, x, "╟───────────────────────────╢");
    mvprintw(y + 11, x, "║ Particles: %4d/256     ║", prof->active_particles);
    mvprintw(y + 12, x, "║ Trails:    %4d          ║", prof->trail_segments);
    
    /* Performance bar (60fps = 16.67ms target) */
    float perf_ratio = prof->frame_time_ms / 16.67f;
    int bar_len = (int)(perf_ratio * 20);
    if (bar_len > 20) bar_len = 20;
    
    mvprintw(y + 13, x, "╟───────────────────────────╢");
    mvprintw(y + 14, x, "║ ");
    
    if (perf_ratio < 0.8) {
        attron(COLOR_PAIR(2)); /* Green */
    } else if (perf_ratio < 1.0) {
        attron(COLOR_PAIR(3)); /* Yellow */
    } else {
        attron(COLOR_PAIR(1)); /* Red */
    }
    
    for (int i = 0; i < 20; i++) {
        if (i < bar_len) {
            mvaddch(y + 14, x + 2 + i, '█');
        } else {
            mvaddch(y + 14, x + 2 + i, '░');
        }
    }
    
    attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3));
    mvprintw(y + 14, x + 23, " %3d%% ║", (int)(perf_ratio * 100));
    
    mvprintw(y + 15, x, "╚═══════════════════════════╝");
    attroff(COLOR_PAIR(7));
    
    /* Instructions */
    mvprintw(y + 16, x, " Press I to hide");
}

void profiler_get_stats(Profiler *prof, double *fps, double *frame_ms) {
    if (!prof) return;
    if (fps) *fps = prof->fps_average;
    if (frame_ms) *frame_ms = prof->frame_time_avg_ms;
}
