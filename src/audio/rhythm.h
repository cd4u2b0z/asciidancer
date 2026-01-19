// rhythm.h - Beat detection and rhythm analysis for ASCII Dancer v2.3
// Spectral flux onset detection, BPM estimation, and beat phase tracking

#ifndef RHYTHM_H
#define RHYTHM_H

#include <stdbool.h>

// History buffer sizes
#define ONSET_HISTORY_SIZE 64      // ~1 second at 60fps
#define BPM_HISTORY_SIZE 16        // Recent BPM estimates
#define SPECTRAL_BANDS 32          // Frequency bands for flux calculation

// Beat detection thresholds
#define ONSET_THRESHOLD 1.5        // Multiplier over local mean
#define MIN_ONSET_INTERVAL 0.1     // Minimum seconds between onsets
#define BPM_MIN 60.0
#define BPM_MAX 200.0

typedef struct {
    // Spectral flux onset detection
    float prev_spectrum[SPECTRAL_BANDS];  // Previous frame's spectrum
    float onset_values[ONSET_HISTORY_SIZE];
    int onset_index;
    float onset_threshold;
    float adaptive_threshold;
    
    // Beat times for BPM calculation
    double beat_times[ONSET_HISTORY_SIZE];
    int beat_count;
    int beat_write_index;
    
    // BPM estimation (autocorrelation)
    float bpm_history[BPM_HISTORY_SIZE];
    int bpm_index;
    float current_bpm;
    float bpm_confidence;
    
    // Beat phase tracking
    double last_beat_time;
    double predicted_next_beat;
    float beat_phase;           // 0.0 = on beat, 0.5 = off beat, 1.0 = next beat
    float phase_correction;     // Adjustment factor for drift
    
    // Current frame state
    bool onset_detected;        // True if onset this frame
    bool beat_predicted;        // True if predicted beat this frame
    float onset_strength;       // How strong the onset is (0-1)
    
    // Enhanced frequency bands
    float sub_bass;            // 20-60 Hz
    float bass;                // 60-250 Hz  
    float low_mid;             // 250-500 Hz
    float mid;                 // 500-2000 Hz
    float high_mid;            // 2000-4000 Hz
    float treble;              // 4000-20000 Hz
    
    // Transient detection
    float bass_velocity;       // Rate of change
    float treble_velocity;
    float prev_bass;
    float prev_treble;
    
    // Timing
    double current_time;
    double dt;
    
} RhythmState;

// Initialize rhythm detection state
RhythmState *rhythm_init(void);

// Destroy rhythm state
void rhythm_destroy(RhythmState *state);

// Update rhythm analysis with new FFT data
// spectrum: array of magnitude values from FFT (256 bins expected)
// num_bins: number of frequency bins
// dt: delta time since last frame
void rhythm_update(RhythmState *state, float *spectrum, int num_bins, double dt);

// Get current beat phase (0.0 = beat, 1.0 = next beat)
float rhythm_get_phase(const RhythmState *state);

// Check if we're on a beat (within tolerance)
bool rhythm_on_beat(const RhythmState *state, float tolerance);

// Get current BPM estimate
float rhythm_get_bpm(RhythmState *state);

// Get onset strength (0-1, higher = stronger transient)
float rhythm_get_onset_strength(RhythmState *state);

// Check if onset was detected this frame
bool rhythm_onset_detected(RhythmState *state);

// Get enhanced frequency band values
void rhythm_get_bands(RhythmState *state, 
                      float *sub_bass, float *bass, float *low_mid,
                      float *mid, float *high_mid, float *treble);

// Get velocity (rate of change) for bass and treble
void rhythm_get_velocity(const RhythmState *state, float *bass_vel, float *treble_vel);

#endif // RHYTHM_H
