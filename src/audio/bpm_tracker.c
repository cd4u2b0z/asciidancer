/*
 * Advanced BPM Tracker Implementation - ASCII Dancer v3.0
 */

#include "bpm_tracker.h"
#include "../constants.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Local aliases from constants.h for clarity */
#define MIN_BPM             BPM_MIN
#define MAX_BPM             BPM_MAX
#define SMOOTHING_FACTOR    TEMPO_SMOOTHING
#define STABILITY_THRESHOLD TEMPO_STABILITY_REQ
#define CONFIDENCE_THRESHOLD TEMPO_LOCK_THRESHOLD

/* Calculate inter-tap intervals and cluster them */
static void analyze_taps(BPMTracker *tracker) {
    if (tracker->tap_count < 4) {
        tracker->confidence = 0.0f;
        tracker->stability = 0.0f;
        return;
    }
    
    /* Calculate intervals between beats */
    float intervals[BPM_TAP_HISTORY];
    int interval_count = 0;
    
    for (int i = 1; i < tracker->tap_count && i < BPM_TAP_HISTORY; i++) {
        int prev_idx = (tracker->tap_write_index - i - 1 + BPM_TAP_HISTORY) % BPM_TAP_HISTORY;
        int curr_idx = (tracker->tap_write_index - i + BPM_TAP_HISTORY) % BPM_TAP_HISTORY;
        
        double interval = tracker->tap_times[curr_idx] - tracker->tap_times[prev_idx];
        
        if (interval > 0.25 && interval < 1.5) {  /* 40-240 BPM range */
            intervals[interval_count++] = (float)interval;
        }
    }
    
    if (interval_count < 3) {
        tracker->confidence = 0.2f;
        return;
    }
    
    /* Build histogram of tempos */
    memset(tracker->tempo_histogram, 0, sizeof(tracker->tempo_histogram));
    
    /* Initialize histogram centers (40-240 BPM in bands) */
    for (int i = 0; i < BPM_CONFIDENCE_BANDS; i++) {
        float bpm = MIN_BPM + (MAX_BPM - MIN_BPM) * i / (float)(BPM_CONFIDENCE_BANDS - 1);
        tracker->histogram_centers[i] = 60.0f / bpm;  /* Convert to interval */
    }
    
    /* Bin intervals into histogram */
    for (int i = 0; i < interval_count; i++) {
        /* Find closest histogram bin */
        int best_bin = 0;
        float best_dist = fabsf(intervals[i] - tracker->histogram_centers[0]);
        
        for (int j = 1; j < BPM_CONFIDENCE_BANDS; j++) {
            float dist = fabsf(intervals[i] - tracker->histogram_centers[j]);
            if (dist < best_dist) {
                best_dist = dist;
                best_bin = j;
            }
        }
        
        tracker->tempo_histogram[best_bin]++;
    }
    
    /* Find dominant tempo cluster */
    int max_count = 0;
    int dominant_bin = 0;
    
    for (int i = 0; i < BPM_CONFIDENCE_BANDS; i++) {
        if (tracker->tempo_histogram[i] > max_count) {
            max_count = tracker->tempo_histogram[i];
            dominant_bin = i;
        }
    }
    
    /* Calculate weighted average around dominant bin */
    float weighted_sum = 0.0f;
    float weight_total = 0.0f;
    
    for (int i = 0; i < interval_count; i++) {
        float dist = fabsf(intervals[i] - tracker->histogram_centers[dominant_bin]);
        if (dist < 0.15f) {  /* Within cluster tolerance */
            float weight = expf(-dist * 5.0f);  /* Gaussian weighting */
            weighted_sum += intervals[i] * weight;
            weight_total += weight;
        }
    }
    
    if (weight_total > 0.0f) {
        float avg_interval = weighted_sum / weight_total;
        tracker->current_bpm = 60.0f / avg_interval;
        
        /* Clamp to valid range */
        if (tracker->current_bpm < MIN_BPM) tracker->current_bpm = MIN_BPM;
        if (tracker->current_bpm > MAX_BPM) tracker->current_bpm = MAX_BPM;
        
        /* Calculate confidence based on cluster consistency */
        tracker->confidence = (float)max_count / (float)interval_count;
        tracker->confidence = fminf(1.0f, tracker->confidence * 1.3f);  /* Boost confidence */
        
        /* Check for half-time / double-time */
        float half_time = tracker->current_bpm * 0.5f;
        float double_time = tracker->current_bpm * 2.0f;
        
        if (half_time >= MIN_BPM && half_time <= MAX_BPM) {
            tracker->alternative_bpm = half_time;
            tracker->alt_confidence = tracker->confidence * 0.7f;
        } else if (double_time >= MIN_BPM && double_time <= MAX_BPM) {
            tracker->alternative_bpm = double_time;
            tracker->alt_confidence = tracker->confidence * 0.7f;
        }
        
    } else {
        tracker->confidence = 0.1f;
    }
}

/* Calculate tempo stability (variance of recent BPMs) */
static void update_stability(BPMTracker *tracker) {
    /* Store current BPM in stability buffer */
    tracker->recent_bpms[tracker->stability_index] = tracker->current_bpm;
    tracker->stability_index = (tracker->stability_index + 1) % BPM_STABILITY_WINDOW;
    
    /* Calculate mean and variance */
    float sum = 0.0f;
    float sum_sq = 0.0f;
    int count = (tracker->tap_count < BPM_STABILITY_WINDOW) ? tracker->tap_count : BPM_STABILITY_WINDOW;
    
    for (int i = 0; i < count; i++) {
        float bpm = tracker->recent_bpms[i];
        sum += bpm;
        sum_sq += bpm * bpm;
    }
    
    if (count > 1) {
        tracker->mean_bpm = sum / count;
        tracker->variance = (sum_sq / count) - (tracker->mean_bpm * tracker->mean_bpm);
        
        /* Convert variance to stability metric (0-1) */
        /* Lower variance = higher stability */
        float std_dev = sqrtf(fabsf(tracker->variance));
        tracker->stability = expf(-std_dev * 0.1f);  /* Exponential decay */
        
        /* Track min/max */
        tracker->min_bpm = tracker->max_bpm = tracker->recent_bpms[0];
        for (int i = 1; i < count; i++) {
            if (tracker->recent_bpms[i] < tracker->min_bpm) tracker->min_bpm = tracker->recent_bpms[i];
            if (tracker->recent_bpms[i] > tracker->max_bpm) tracker->max_bpm = tracker->recent_bpms[i];
        }
    } else {
        tracker->mean_bpm = tracker->current_bpm;
        tracker->stability = 0.0f;
        tracker->min_bpm = tracker->max_bpm = tracker->current_bpm;
    }
    
    /* Update tempo lock status */
    tracker->tempo_locked = (tracker->confidence >= CONFIDENCE_THRESHOLD) && 
                            (tracker->stability >= STABILITY_THRESHOLD);
}

/* ============ Public API ============ */

BPMTracker* bpm_tracker_create(void) {
    BPMTracker *tracker = calloc(1, sizeof(BPMTracker));
    if (!tracker) return NULL;
    
    tracker->current_bpm = 120.0f;
    tracker->smoothed_bpm = 120.0f;
    tracker->min_bpm = MIN_BPM;
    tracker->max_bpm = MAX_BPM;
    tracker->mean_bpm = 120.0f;
    
    return tracker;
}

void bpm_tracker_destroy(BPMTracker *tracker) {
    if (tracker) {
        free(tracker);
    }
}

void bpm_tracker_reset(BPMTracker *tracker) {
    if (!tracker) return;
    
    memset(tracker->tap_times, 0, sizeof(tracker->tap_times));
    tracker->tap_count = 0;
    tracker->tap_write_index = 0;
    tracker->confidence = 0.0f;
    tracker->stability = 0.0f;
    tracker->tempo_locked = false;
    tracker->current_bpm = 120.0f;
    tracker->smoothed_bpm = 120.0f;
}

void bpm_tracker_tap(BPMTracker *tracker, double time) {
    if (!tracker) return;
    
    /* Ignore taps that are too close together */
    if (tracker->tap_count > 0) {
        double dt = time - tracker->last_tap_time;
        if (dt < 0.25) return;  /* Minimum 240 BPM */
    }
    
    /* Record tap time */
    tracker->tap_times[tracker->tap_write_index] = time;
    tracker->tap_write_index = (tracker->tap_write_index + 1) % BPM_TAP_HISTORY;
    tracker->tap_count++;
    
    tracker->last_tap_time = time;
    
    /* Analyze taps to update BPM */
    analyze_taps(tracker);
    update_stability(tracker);
}

void bpm_tracker_update(BPMTracker *tracker, double dt) {
    if (!tracker) return;
    
    tracker->current_time += dt;
    
    /* Smooth BPM for display (low-pass filter) */
    tracker->smoothed_bpm += (tracker->current_bpm - tracker->smoothed_bpm) * SMOOTHING_FACTOR;
    
    /* Decay confidence if no recent taps */
    if (tracker->tap_count > 0) {
        double time_since_tap = tracker->current_time - tracker->last_tap_time;
        if (time_since_tap > 3.0) {
            tracker->confidence *= 0.95f;  /* Slow decay */
            tracker->stability *= 0.98f;
        }
    }
}

float bpm_tracker_get_bpm(BPMTracker *tracker) {
    return tracker ? tracker->smoothed_bpm : 120.0f;
}

float bpm_tracker_get_raw_bpm(BPMTracker *tracker) {
    return tracker ? tracker->current_bpm : 120.0f;
}

float bpm_tracker_get_confidence(BPMTracker *tracker) {
    return tracker ? tracker->confidence : 0.0f;
}

float bpm_tracker_get_stability(BPMTracker *tracker) {
    return tracker ? tracker->stability : 0.0f;
}

bool bpm_tracker_is_locked(BPMTracker *tracker) {
    return tracker ? tracker->tempo_locked : false;
}

float bpm_tracker_get_alternative(BPMTracker *tracker, float *confidence) {
    if (!tracker) return 120.0f;
    if (confidence) *confidence = tracker->alt_confidence;
    return tracker->alternative_bpm;
}

void bpm_tracker_get_range(BPMTracker *tracker, float *min, float *max, float *mean) {
    if (!tracker) return;
    if (min) *min = tracker->min_bpm;
    if (max) *max = tracker->max_bpm;
    if (mean) *mean = tracker->mean_bpm;
}
