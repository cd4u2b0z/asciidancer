// rhythm.c - Beat detection and rhythm analysis for ASCII Dancer v2.3
// Implements spectral flux onset detection, autocorrelation BPM, beat phase tracking

#include "rhythm.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Get current time in seconds
static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

RhythmState *rhythm_init(void) {
    RhythmState *state = calloc(1, sizeof(RhythmState));
    if (!state) return NULL;
    
    state->current_bpm = 120.0f;  // Default assumption
    state->bpm_confidence = 0.0f;
    state->onset_threshold = ONSET_THRESHOLD;
    state->adaptive_threshold = ONSET_THRESHOLD;
    state->current_time = get_time();
    
    return state;
}

void rhythm_destroy(RhythmState *state) {
    if (state) {
        free(state);
    }
}

// Calculate spectral flux (measure of spectral change between frames)
static float calculate_spectral_flux(RhythmState *state, const float *spectrum, int num_bins) {
    float flux = 0.0f;
    int bins_per_band = num_bins / SPECTRAL_BANDS;
    if (bins_per_band < 1) bins_per_band = 1;
    
    for (int band = 0; band < SPECTRAL_BANDS && band * bins_per_band < num_bins; band++) {
        // Sum magnitude for this band
        float band_mag = 0.0f;
        int start = band * bins_per_band;
        int end = start + bins_per_band;
        if (end > num_bins) end = num_bins;
        
        for (int i = start; i < end; i++) {
            band_mag += spectrum[i];
        }
        band_mag /= (end - start);
        
        // Half-wave rectified difference (only count increases)
        float diff = band_mag - state->prev_spectrum[band];
        if (diff > 0) {
            // Weight lower frequencies more heavily for beat detection
            float weight = 1.0f + (1.0f - (float)band / SPECTRAL_BANDS) * 2.0f;
            flux += diff * weight;
        }
        
        // Store for next frame
        state->prev_spectrum[band] = band_mag;
    }
    
    return flux;
}

// Calculate local mean and deviation of onset values
static void calculate_onset_stats(RhythmState *state, float *mean, float *stddev) {
    float sum = 0.0f;
    float sum_sq = 0.0f;
    int count = ONSET_HISTORY_SIZE;
    
    for (int i = 0; i < count; i++) {
        sum += state->onset_values[i];
        sum_sq += state->onset_values[i] * state->onset_values[i];
    }
    
    *mean = sum / count;
    float variance = (sum_sq / count) - (*mean * *mean);
    *stddev = (variance > 0) ? sqrtf(variance) : 0.0f;
}

// Estimate BPM using autocorrelation of beat times
static float estimate_bpm_autocorrelation(RhythmState *state) {
    if (state->beat_count < 4) return state->current_bpm;
    
    // Calculate inter-beat intervals
    float intervals[ONSET_HISTORY_SIZE];
    int interval_count = 0;
    
    for (int i = 1; i < state->beat_count && interval_count < ONSET_HISTORY_SIZE - 1; i++) {
        int idx = (state->beat_write_index - i + ONSET_HISTORY_SIZE) % ONSET_HISTORY_SIZE;
        int prev_idx = (idx - 1 + ONSET_HISTORY_SIZE) % ONSET_HISTORY_SIZE;
        
        double interval = state->beat_times[idx] - state->beat_times[prev_idx];
        if (interval > 0.2 && interval < 2.0) {  // Reasonable beat interval
            intervals[interval_count++] = (float)interval;
        }
    }
    
    if (interval_count < 3) return state->current_bpm;
    
    // Find most common interval using histogram
    float best_interval = 0.5f;  // 120 BPM default
    int best_count = 0;
    
    // Check intervals from 60 BPM (1.0s) to 200 BPM (0.3s)
    for (float test_interval = 0.3f; test_interval <= 1.0f; test_interval += 0.01f) {
        int count = 0;
        for (int i = 0; i < interval_count; i++) {
            // Check if interval matches (or is multiple/divisor)
            float ratio = intervals[i] / test_interval;
            float remainder = fabsf(ratio - roundf(ratio));
            if (remainder < 0.1f) {
                count++;
            }
        }
        if (count > best_count) {
            best_count = count;
            best_interval = test_interval;
        }
    }
    
    float bpm = 60.0f / best_interval;
    
    // Clamp to reasonable range
    if (bpm < BPM_MIN) bpm = BPM_MIN;
    if (bpm > BPM_MAX) bpm = BPM_MAX;
    
    // Calculate confidence based on how consistent the intervals are
    state->bpm_confidence = (float)best_count / interval_count;
    
    return bpm;
}

// Update beat phase based on predicted and actual beats
static void update_beat_phase(RhythmState *state, bool onset) {
    double now = state->current_time;
    float beat_period = 60.0f / state->current_bpm;
    
    if (onset) {
        // An onset was detected - update phase tracking
        double __attribute__((unused)) time_since_last = now - state->last_beat_time;
        
        // If this onset is close to our prediction, we're on track
        if (state->predicted_next_beat > 0) {
            double prediction_error = now - state->predicted_next_beat;
            
            // Adjust phase correction based on error
            if (fabs(prediction_error) < beat_period * 0.3f) {
                // Good prediction - minor correction
                state->phase_correction = prediction_error * 0.1f;
            } else {
                // Reset to this beat
                state->phase_correction = 0;
            }
        }
        
        state->last_beat_time = now;
        state->predicted_next_beat = now + beat_period;
    }
    
    // Calculate current phase (0 = on beat, 1 = next beat)
    if (state->last_beat_time > 0 && beat_period > 0) {
        double time_since_beat = now - state->last_beat_time;
        state->beat_phase = fmodf(time_since_beat / beat_period, 1.0f);
        
        // Apply phase correction for smoother tracking
        state->beat_phase += state->phase_correction;
        if (state->beat_phase < 0) state->beat_phase += 1.0f;
        if (state->beat_phase >= 1.0f) state->beat_phase -= 1.0f;
    }
    
    // Check if we predict a beat this frame
    state->beat_predicted = false;
    if (state->predicted_next_beat > 0) {
        double time_to_beat = state->predicted_next_beat - now;
        if (time_to_beat <= 0 && time_to_beat > -state->dt * 2) {
            state->beat_predicted = true;
            state->predicted_next_beat += beat_period;
        }
    }
}

// Extract enhanced frequency bands from spectrum
static void extract_bands(RhythmState *state, const float *spectrum, int num_bins, float sample_rate) {
    // Assume sample_rate of 44100 if not specified
    if (sample_rate <= 0) sample_rate = 44100.0f;
    
    float bin_freq = sample_rate / (2.0f * num_bins);
    
    // Band boundaries in Hz
    // Sub-bass: 20-60, Bass: 60-250, Low-mid: 250-500
    // Mid: 500-2000, High-mid: 2000-4000, Treble: 4000+
    
    state->sub_bass = 0;
    state->bass = 0;
    state->low_mid = 0;
    state->mid = 0;
    state->high_mid = 0;
    state->treble = 0;
    
    int sub_bass_count = 0, bass_count = 0, low_mid_count = 0;
    int mid_count = 0, high_mid_count = 0, treble_count = 0;
    
    for (int i = 0; i < num_bins; i++) {
        float freq = i * bin_freq;
        float mag = spectrum[i];
        
        if (freq < 60) {
            state->sub_bass += mag;
            sub_bass_count++;
        } else if (freq < 250) {
            state->bass += mag;
            bass_count++;
        } else if (freq < 500) {
            state->low_mid += mag;
            low_mid_count++;
        } else if (freq < 2000) {
            state->mid += mag;
            mid_count++;
        } else if (freq < 4000) {
            state->high_mid += mag;
            high_mid_count++;
        } else {
            state->treble += mag;
            treble_count++;
        }
    }
    
    // Average
    if (sub_bass_count > 0) state->sub_bass /= sub_bass_count;
    if (bass_count > 0) state->bass /= bass_count;
    if (low_mid_count > 0) state->low_mid /= low_mid_count;
    if (mid_count > 0) state->mid /= mid_count;
    if (high_mid_count > 0) state->high_mid /= high_mid_count;
    if (treble_count > 0) state->treble /= treble_count;
    
    // Calculate velocities (rate of change)
    state->bass_velocity = (state->bass - state->prev_bass) / (state->dt > 0 ? state->dt : 0.016f);
    state->treble_velocity = (state->treble - state->prev_treble) / (state->dt > 0 ? state->dt : 0.016f);
    
    state->prev_bass = state->bass;
    state->prev_treble = state->treble;
}

void rhythm_update(RhythmState *state, float *spectrum, int num_bins, double dt) {
    if (!state || !spectrum || num_bins <= 0) return;
    
    state->dt = dt;
    state->current_time = get_time();
    
    // Extract frequency bands
    extract_bands(state, spectrum, num_bins, 44100.0f);
    
    // Calculate spectral flux
    float flux = calculate_spectral_flux(state, spectrum, num_bins);
    
    // Store in history
    state->onset_values[state->onset_index] = flux;
    state->onset_index = (state->onset_index + 1) % ONSET_HISTORY_SIZE;
    
    // Calculate adaptive threshold
    float mean, stddev;
    calculate_onset_stats(state, &mean, &stddev);
    state->adaptive_threshold = mean + stddev * state->onset_threshold;
    
    // Detect onset
    state->onset_detected = false;
    state->onset_strength = 0.0f;
    
    double time_since_last = state->current_time - state->last_beat_time;
    
    if (flux > state->adaptive_threshold && time_since_last > MIN_ONSET_INTERVAL) {
        state->onset_detected = true;
        state->onset_strength = (flux - mean) / (stddev > 0 ? stddev : 1.0f);
        if (state->onset_strength > 1.0f) state->onset_strength = 1.0f;
        
        // Record beat time
        state->beat_times[state->beat_write_index] = state->current_time;
        state->beat_write_index = (state->beat_write_index + 1) % ONSET_HISTORY_SIZE;
        if (state->beat_count < ONSET_HISTORY_SIZE) state->beat_count++;
        
        // Update BPM estimate
        float new_bpm = estimate_bpm_autocorrelation(state);
        
        // Smooth BPM changes
        state->bpm_history[state->bpm_index] = new_bpm;
        state->bpm_index = (state->bpm_index + 1) % BPM_HISTORY_SIZE;
        
        float bpm_sum = 0;
        for (int i = 0; i < BPM_HISTORY_SIZE; i++) {
            bpm_sum += state->bpm_history[i] > 0 ? state->bpm_history[i] : state->current_bpm;
        }
        state->current_bpm = bpm_sum / BPM_HISTORY_SIZE;
    }
    
    // Update beat phase tracking
    update_beat_phase(state, state->onset_detected);
}

float rhythm_get_phase(const RhythmState *state) {
    return state ? state->beat_phase : 0.0f;
}

bool rhythm_on_beat(const RhythmState *state, float tolerance) {
    if (!state) return false;
    return state->beat_phase < tolerance || state->beat_phase > (1.0f - tolerance);
}

float rhythm_get_bpm(RhythmState *state) {
    return state ? state->current_bpm : 120.0f;
}

float rhythm_get_onset_strength(RhythmState *state) {
    return state ? state->onset_strength : 0.0f;
}

bool rhythm_onset_detected(RhythmState *state) {
    return state ? state->onset_detected : false;
}

void rhythm_get_bands(RhythmState *state, 
                      float *sub_bass, float *bass, float *low_mid,
                      float *mid, float *high_mid, float *treble) {
    if (!state) {
        if (sub_bass) *sub_bass = 0;
        if (bass) *bass = 0;
        if (low_mid) *low_mid = 0;
        if (mid) *mid = 0;
        if (high_mid) *high_mid = 0;
        if (treble) *treble = 0;
        return;
    }
    
    if (sub_bass) *sub_bass = state->sub_bass;
    if (bass) *bass = state->bass;
    if (low_mid) *low_mid = state->low_mid;
    if (mid) *mid = state->mid;
    if (high_mid) *high_mid = state->high_mid;
    if (treble) *treble = state->treble;
}

void rhythm_get_velocity(const RhythmState *state, float *bass_vel, float *treble_vel) {
    if (!state) {
        if (bass_vel) *bass_vel = 0;
        if (treble_vel) *treble_vel = 0;
        return;
    }
    
    if (bass_vel) *bass_vel = state->bass_velocity;
    if (treble_vel) *treble_vel = state->treble_velocity;
}
