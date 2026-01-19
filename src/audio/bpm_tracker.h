/*
 * Advanced BPM Tracker - ASCII Dancer v3.0
 *
 * Multi-tap tempo averaging with confidence scoring
 * Adaptive tempo tracking handles gradual tempo changes
 * Stability detection filters out false positives
 */

#ifndef BPM_TRACKER_H
#define BPM_TRACKER_H

#include <stdbool.h>

#define BPM_TAP_HISTORY 32      /* Recent beat tap times */
#define BPM_CONFIDENCE_BANDS 8  /* Tempo grouping bands */
#define BPM_STABILITY_WINDOW 16 /* Frames to check stability */

typedef struct {
    /* Multi-tap tempo tracking */
    double tap_times[BPM_TAP_HISTORY];
    int tap_count;
    int tap_write_index;
    
    /* Current BPM analysis */
    float current_bpm;
    float smoothed_bpm;        /* Low-pass filtered for display */
    float confidence;          /* 0-1, how confident we are */
    float stability;           /* 0-1, how stable the tempo is */
    
    /* Tempo histogram for clustering */
    int tempo_histogram[BPM_CONFIDENCE_BANDS];
    float histogram_centers[BPM_CONFIDENCE_BANDS];
    
    /* Adaptive tracking */
    float drift_rate;          /* BPM per second drift */
    bool tempo_locked;         /* True when confident and stable */
    
    /* Stability tracking */
    float recent_bpms[BPM_STABILITY_WINDOW];
    int stability_index;
    
    /* Half-time / double-time detection */
    float alternative_bpm;     /* Likely half or double tempo */
    float alt_confidence;
    
    /* Statistics */
    float min_bpm;
    float max_bpm;
    float mean_bpm;
    float variance;
    
    /* Timing */
    double last_tap_time;
    double current_time;
    
} BPMTracker;

/* ============ Lifecycle ============ */

/* Create BPM tracker */
BPMTracker* bpm_tracker_create(void);

/* Destroy tracker */
void bpm_tracker_destroy(BPMTracker *tracker);

/* Reset tracker state */
void bpm_tracker_reset(BPMTracker *tracker);

/* ============ Updates ============ */

/* Register a beat tap (onset detected) */
void bpm_tracker_tap(BPMTracker *tracker, double time);

/* Update per-frame (recalculates stats) */
void bpm_tracker_update(BPMTracker *tracker, double dt);

/* ============ Queries ============ */

/* Get current BPM estimate (smoothed for display) */
float bpm_tracker_get_bpm(BPMTracker *tracker);

/* Get raw BPM (unsmoothed, more reactive) */
float bpm_tracker_get_raw_bpm(BPMTracker *tracker);

/* Get confidence level (0-1, display as percentage) */
float bpm_tracker_get_confidence(BPMTracker *tracker);

/* Get stability (0-1, shows if tempo is steady) */
float bpm_tracker_get_stability(BPMTracker *tracker);

/* Check if tempo is locked (confident + stable) */
bool bpm_tracker_is_locked(BPMTracker *tracker);

/* Get alternative tempo (half/double time) */
float bpm_tracker_get_alternative(const BPMTracker *tracker, float *confidence);

/* Get BPM range (min, max, mean) */
void bpm_tracker_get_range(const BPMTracker *tracker, float *min, float *max, float *mean);

#endif /* BPM_TRACKER_H */
