/*
 * Dynamic Energy Analysis - ASCII Dancer v3.0
 *
 * Real-time intensity tracking with multiple metrics:
 * - RMS energy (root mean square)
 * - Peak detection
 * - Intensity zones (low/medium/high/peak)
 * - Envelope tracking with attack/release
 * - Spectral centroid (brightness)
 */

#ifndef ENERGY_ANALYZER_H
#define ENERGY_ANALYZER_H

#include <stdbool.h>

#define ENERGY_HISTORY_SIZE 128   /* Frames of energy history */
#define ENERGY_BANDS 6            /* Bass, Low-mid, Mid, High-mid, Treble, Sub-bass */

typedef enum {
    ZONE_SILENT = 0,
    ZONE_LOW,
    ZONE_MEDIUM,
    ZONE_HIGH,
    ZONE_PEAK
} IntensityZone;

typedef struct {
    /* Current energy metrics */
    float rms_energy;              /* Root mean square energy */
    float peak_level;              /* Peak amplitude */
    float smoothed_energy;         /* Attack/release smoothed */
    
    /* Intensity classification */
    IntensityZone current_zone;
    float zone_confidence;         /* How strongly in this zone */
    
    /* Band energies */
    float band_energy[ENERGY_BANDS];
    float band_peaks[ENERGY_BANDS];
    
    /* Dynamic range */
    float dynamic_range;           /* Peak - RMS in dB */
    float compression_level;       /* How compressed the audio is */
    
    /* Envelope tracking */
    float attack_time;             /* Seconds for rise */
    float release_time;            /* Seconds for fall */
    float envelope;                /* Current envelope value */
    
    /* History for visualization */
    float energy_history[ENERGY_HISTORY_SIZE];
    int history_index;
    
    /* Spectral characteristics */
    float spectral_centroid;       /* "Brightness" of sound */
    float spectral_spread;         /* How spread out frequencies are */
    float spectral_rolloff;        /* Frequency where 85% energy below */
    
    /* Pace/tempo correlation */
    float pace_intensity;          /* Combined tempo + energy metric */
    float transient_density;       /* How many quick hits/onsets */
    
    /* Adaptive thresholds */
    float silence_threshold;
    float low_threshold;
    float medium_threshold;
    float high_threshold;
    
    /* Statistics */
    float mean_energy;
    float energy_variance;
    float min_energy;
    float max_energy;
    
} EnergyAnalyzer;

/* ============ Lifecycle ============ */

/* Create analyzer */
EnergyAnalyzer* energy_analyzer_create(void);

/* Destroy analyzer */
void energy_analyzer_destroy(EnergyAnalyzer *analyzer);

/* Reset state */
void energy_analyzer_reset(EnergyAnalyzer *analyzer);

/* ============ Updates ============ */

/* Update with audio buffer (mono float samples) */
void energy_analyzer_update(EnergyAnalyzer *analyzer, 
                           const float *samples, 
                           int sample_count,
                           float dt);

/* Update with FFT magnitude spectrum */
void energy_analyzer_update_spectrum(EnergyAnalyzer *analyzer,
                                    const float *magnitudes,
                                    int bin_count,
                                    float sample_rate);

/* Update band energies from frequency bins */
void energy_analyzer_update_bands(EnergyAnalyzer *analyzer,
                                 float sub_bass,
                                 float bass,
                                 float low_mid,
                                 float mid,
                                 float high_mid,
                                 float treble);

/* Update pace intensity (call with BPM and onset info) */
void energy_analyzer_update_pace(EnergyAnalyzer *analyzer,
                                float bpm,
                                float onset_strength,
                                float transient_count);

/* ============ Queries ============ */

/* Get current RMS energy (0-1) */
float energy_analyzer_get_rms(EnergyAnalyzer *analyzer);

/* Get smoothed energy with envelope (0-1) */
float energy_analyzer_get_smoothed(EnergyAnalyzer *analyzer);

/* Get current intensity zone */
IntensityZone energy_analyzer_get_zone(EnergyAnalyzer *analyzer);

/* Get zone as string ("Low", "Medium", etc) */
const char* energy_analyzer_get_zone_name(EnergyAnalyzer *analyzer);

/* Get zone confidence (0-1) */
float energy_analyzer_get_zone_confidence(EnergyAnalyzer *analyzer);

/* Get pace intensity (combined tempo + energy, 0-1) */
float energy_analyzer_get_pace(EnergyAnalyzer *analyzer);

/* Get dynamic range in dB */
float energy_analyzer_get_dynamic_range(EnergyAnalyzer *analyzer);

/* Get spectral brightness (0-1) */
float energy_analyzer_get_brightness(EnergyAnalyzer *analyzer);

/* Get energy history for visualization */
const float* energy_analyzer_get_history(EnergyAnalyzer *analyzer, int *size);

/* Get band energy by index (0-5) */
float energy_analyzer_get_band(EnergyAnalyzer *analyzer, int band_index);

/* Get statistics */
void energy_analyzer_get_stats(EnergyAnalyzer *analyzer,
                              float *mean,
                              float *variance,
                              float *min,
                              float *max);

#endif /* ENERGY_ANALYZER_H */
