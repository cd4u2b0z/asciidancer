/*
 * Dynamic Energy Analysis Implementation - ASCII Dancer v3.0
 */

#include "energy_analyzer.h"
#include "../constants.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ATTACK_TIME_DEFAULT 0.01f   /* 10ms attack */
#define RELEASE_TIME_DEFAULT 0.3f   /* 300ms release */
/* DB_FLOOR defined in constants.h */

/* Convert amplitude to dB */
static float amp_to_db(float amp) {
    if (amp < 0.000001f) return DB_FLOOR;
    return 20.0f * log10f(amp);
}

/* Calculate RMS energy from audio samples */
static float calculate_rms(const float *samples, int count) {
    float sum_squares = 0.0f;
    for (int i = 0; i < count; i++) {
        sum_squares += samples[i] * samples[i];
    }
    return sqrtf(sum_squares / count);
}

/* Find peak amplitude */
static float find_peak(const float *samples, int count) {
    float peak = 0.0f;
    for (int i = 0; i < count; i++) {
        float abs_val = fabsf(samples[i]);
        if (abs_val > peak) peak = abs_val;
    }
    return peak;
}

/* Calculate spectral centroid (brightness) */
static float calculate_centroid(const float *magnitudes, int bin_count, float sample_rate) {
    float weighted_sum = 0.0f;
    float magnitude_sum = 0.0f;
    
    for (int i = 0; i < bin_count; i++) {
        float freq = (i * sample_rate) / (2.0f * bin_count);
        weighted_sum += freq * magnitudes[i];
        magnitude_sum += magnitudes[i];
    }
    
    if (magnitude_sum < 0.0001f) return 0.0f;
    return weighted_sum / magnitude_sum;
}

/* Calculate spectral spread (frequency variance) */
static float calculate_spread(const float *magnitudes, int bin_count, float sample_rate, float centroid) {
    float sum = 0.0f;
    float magnitude_sum = 0.0f;
    
    for (int i = 0; i < bin_count; i++) {
        float freq = (i * sample_rate) / (2.0f * bin_count);
        float diff = freq - centroid;
        sum += diff * diff * magnitudes[i];
        magnitude_sum += magnitudes[i];
    }
    
    if (magnitude_sum < 0.0001f) return 0.0f;
    return sqrtf(sum / magnitude_sum);
}

/* Calculate spectral rolloff (85% energy point) */
static float calculate_rolloff(const float *magnitudes, int bin_count, float sample_rate) {
    float total_energy = 0.0f;
    for (int i = 0; i < bin_count; i++) {
        total_energy += magnitudes[i];
    }
    
    float threshold = total_energy * 0.85f;
    float cumulative = 0.0f;
    
    for (int i = 0; i < bin_count; i++) {
        cumulative += magnitudes[i];
        if (cumulative >= threshold) {
            return (i * sample_rate) / (2.0f * bin_count);
        }
    }
    
    return sample_rate * 0.5f;  /* Nyquist */
}

/* Classify energy into intensity zone */
static void classify_zone(EnergyAnalyzer *analyzer) {
    float energy = analyzer->smoothed_energy;
    
    /* Adaptive thresholds based on recent history */
    IntensityZone zone;
    float confidence = 1.0f;
    
    if (energy < analyzer->silence_threshold) {
        zone = ZONE_SILENT;
    } else if (energy < analyzer->low_threshold) {
        zone = ZONE_LOW;
        /* Confidence based on distance from boundaries */
        float range = analyzer->low_threshold - analyzer->silence_threshold;
        float pos = (energy - analyzer->silence_threshold) / range;
        confidence = fminf(1.0f, pos * 2.0f);  /* Ramp up confidence */
    } else if (energy < analyzer->medium_threshold) {
        zone = ZONE_MEDIUM;
        float range = analyzer->medium_threshold - analyzer->low_threshold;
        float pos = (energy - analyzer->low_threshold) / range;
        confidence = 0.5f + pos * 0.5f;
    } else if (energy < analyzer->high_threshold) {
        zone = ZONE_HIGH;
        float range = analyzer->high_threshold - analyzer->medium_threshold;
        float pos = (energy - analyzer->medium_threshold) / range;
        confidence = 0.7f + pos * 0.3f;
    } else {
        zone = ZONE_PEAK;
        confidence = fminf(1.0f, energy / analyzer->high_threshold);
    }
    
    analyzer->current_zone = zone;
    analyzer->zone_confidence = confidence;
}

/* Update adaptive thresholds based on history */
static void update_thresholds(EnergyAnalyzer *analyzer) {
    /* Calculate percentiles from history */
    float sorted[ENERGY_HISTORY_SIZE];
    memcpy(sorted, analyzer->energy_history, sizeof(sorted));
    
    /* Simple bubble sort (small array) */
    for (int i = 0; i < ENERGY_HISTORY_SIZE - 1; i++) {
        for (int j = 0; j < ENERGY_HISTORY_SIZE - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                float temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    /* Set thresholds at percentiles */
    analyzer->silence_threshold = sorted[(int)(ENERGY_HISTORY_SIZE * 0.10f)];
    analyzer->low_threshold = sorted[(int)(ENERGY_HISTORY_SIZE * 0.35f)];
    analyzer->medium_threshold = sorted[(int)(ENERGY_HISTORY_SIZE * 0.65f)];
    analyzer->high_threshold = sorted[(int)(ENERGY_HISTORY_SIZE * 0.90f)];
}

/* ============ Public API ============ */

EnergyAnalyzer* energy_analyzer_create(void) {
    EnergyAnalyzer *analyzer = calloc(1, sizeof(EnergyAnalyzer));
    if (!analyzer) return NULL;
    
    analyzer->attack_time = ATTACK_TIME_DEFAULT;
    analyzer->release_time = RELEASE_TIME_DEFAULT;
    
    /* Default thresholds */
    analyzer->silence_threshold = 0.01f;
    analyzer->low_threshold = 0.15f;
    analyzer->medium_threshold = 0.35f;
    analyzer->high_threshold = 0.65f;
    
    return analyzer;
}

void energy_analyzer_destroy(EnergyAnalyzer *analyzer) {
    if (analyzer) {
        free(analyzer);
    }
}

void energy_analyzer_reset(EnergyAnalyzer *analyzer) {
    if (!analyzer) return;
    
    memset(analyzer->energy_history, 0, sizeof(analyzer->energy_history));
    analyzer->history_index = 0;
    analyzer->envelope = 0.0f;
    analyzer->current_zone = ZONE_SILENT;
}

void energy_analyzer_update(EnergyAnalyzer *analyzer,
                           const float *samples,
                           int sample_count,
                           float dt) {
    if (!analyzer || !samples || sample_count <= 0) return;
    
    /* Calculate RMS and peak */
    analyzer->rms_energy = calculate_rms(samples, sample_count);
    analyzer->peak_level = find_peak(samples, sample_count);
    
    /* Calculate dynamic range */
    float rms_db = amp_to_db(analyzer->rms_energy);
    float peak_db = amp_to_db(analyzer->peak_level);
    analyzer->dynamic_range = peak_db - rms_db;
    
    /* Estimate compression (0-1, higher = more compressed) */
    /* Heavily compressed audio has low dynamic range */
    analyzer->compression_level = 1.0f - fminf(1.0f, analyzer->dynamic_range / 20.0f);
    
    /* Envelope follower (attack/release) */
    float target = analyzer->rms_energy;
    float diff = target - analyzer->envelope;
    
    if (diff > 0.0f) {
        /* Attack */
        float attack_coeff = 1.0f - expf(-dt / analyzer->attack_time);
        analyzer->envelope += diff * attack_coeff;
    } else {
        /* Release */
        float release_coeff = 1.0f - expf(-dt / analyzer->release_time);
        analyzer->envelope += diff * release_coeff;
    }
    
    analyzer->smoothed_energy = analyzer->envelope;
    
    /* Store in history */
    analyzer->energy_history[analyzer->history_index] = analyzer->smoothed_energy;
    analyzer->history_index = (analyzer->history_index + 1) % ENERGY_HISTORY_SIZE;
    
    /* Update statistics */
    float sum = 0.0f;
    float sum_sq = 0.0f;
    analyzer->min_energy = analyzer->max_energy = analyzer->energy_history[0];
    
    for (int i = 0; i < ENERGY_HISTORY_SIZE; i++) {
        float e = analyzer->energy_history[i];
        sum += e;
        sum_sq += e * e;
        if (e < analyzer->min_energy) analyzer->min_energy = e;
        if (e > analyzer->max_energy) analyzer->max_energy = e;
    }
    
    analyzer->mean_energy = sum / ENERGY_HISTORY_SIZE;
    analyzer->energy_variance = (sum_sq / ENERGY_HISTORY_SIZE) - 
                                (analyzer->mean_energy * analyzer->mean_energy);
    
    /* Update adaptive thresholds periodically */
    if (analyzer->history_index % 32 == 0) {
        update_thresholds(analyzer);
    }
    
    /* Classify into zone */
    classify_zone(analyzer);
}

void energy_analyzer_update_spectrum(EnergyAnalyzer *analyzer,
                                    const float *magnitudes,
                                    int bin_count,
                                    float sample_rate) {
    if (!analyzer || !magnitudes || bin_count <= 0) return;
    
    /* Calculate spectral features */
    analyzer->spectral_centroid = calculate_centroid(magnitudes, bin_count, sample_rate);
    analyzer->spectral_spread = calculate_spread(magnitudes, bin_count, sample_rate, 
                                                 analyzer->spectral_centroid);
    analyzer->spectral_rolloff = calculate_rolloff(magnitudes, bin_count, sample_rate);
}

void energy_analyzer_update_bands(EnergyAnalyzer *analyzer,
                                 float sub_bass,
                                 float bass,
                                 float low_mid,
                                 float mid,
                                 float high_mid,
                                 float treble) {
    if (!analyzer) return;
    
    analyzer->band_energy[0] = sub_bass;
    analyzer->band_energy[1] = bass;
    analyzer->band_energy[2] = low_mid;
    analyzer->band_energy[3] = mid;
    analyzer->band_energy[4] = high_mid;
    analyzer->band_energy[5] = treble;
    
    /* Update band peaks (slow decay) */
    for (int i = 0; i < ENERGY_BANDS; i++) {
        if (analyzer->band_energy[i] > analyzer->band_peaks[i]) {
            analyzer->band_peaks[i] = analyzer->band_energy[i];
        } else {
            analyzer->band_peaks[i] *= 0.98f;  /* Decay */
        }
    }
}

void energy_analyzer_update_pace(EnergyAnalyzer *analyzer,
                                float bpm,
                                float onset_strength,
                                float transient_count) {
    if (!analyzer) return;
    
    /* Combine BPM and energy for pace metric */
    /* Normalize BPM to 0-1 (assuming 60-180 range) */
    float bpm_norm = fminf(1.0f, (bpm - 60.0f) / 120.0f);
    
    /* Pace is weighted combination */
    analyzer->pace_intensity = bpm_norm * 0.4f + 
                              analyzer->smoothed_energy * 0.4f +
                              onset_strength * 0.2f;
    
    analyzer->transient_density = transient_count;
}

float energy_analyzer_get_rms(EnergyAnalyzer *analyzer) {
    return analyzer ? analyzer->rms_energy : 0.0f;
}

float energy_analyzer_get_smoothed(EnergyAnalyzer *analyzer) {
    return analyzer ? analyzer->smoothed_energy : 0.0f;
}

IntensityZone energy_analyzer_get_zone(EnergyAnalyzer *analyzer) {
    return analyzer ? analyzer->current_zone : ZONE_SILENT;
}

const char* energy_analyzer_get_zone_name(EnergyAnalyzer *analyzer) {
    if (!analyzer) return "Unknown";
    
    switch (analyzer->current_zone) {
        case ZONE_SILENT: return "Silent";
        case ZONE_LOW: return "Low";
        case ZONE_MEDIUM: return "Medium";
        case ZONE_HIGH: return "High";
        case ZONE_PEAK: return "Peak";
        default: return "Unknown";
    }
}

float energy_analyzer_get_zone_confidence(EnergyAnalyzer *analyzer) {
    return analyzer ? analyzer->zone_confidence : 0.0f;
}

float energy_analyzer_get_pace(EnergyAnalyzer *analyzer) {
    return analyzer ? analyzer->pace_intensity : 0.0f;
}

float energy_analyzer_get_dynamic_range(EnergyAnalyzer *analyzer) {
    return analyzer ? analyzer->dynamic_range : 0.0f;
}

float energy_analyzer_get_brightness(EnergyAnalyzer *analyzer) {
    if (!analyzer) return 0.0f;
    /* Normalize spectral centroid to 0-1 (assuming 0-10kHz range) */
    return fminf(1.0f, analyzer->spectral_centroid / 10000.0f);
}

const float* energy_analyzer_get_history(EnergyAnalyzer *analyzer, int *size) {
    if (!analyzer) return NULL;
    if (size) *size = ENERGY_HISTORY_SIZE;
    return analyzer->energy_history;
}

float energy_analyzer_get_band(EnergyAnalyzer *analyzer, int band_index) {
    if (!analyzer || band_index < 0 || band_index >= ENERGY_BANDS) return 0.0f;
    return analyzer->band_energy[band_index];
}

void energy_analyzer_get_stats(EnergyAnalyzer *analyzer,
                              float *mean,
                              float *variance,
                              float *min,
                              float *max) {
    if (!analyzer) return;
    if (mean) *mean = analyzer->mean_energy;
    if (variance) *variance = analyzer->energy_variance;
    if (min) *min = analyzer->min_energy;
    if (max) *max = analyzer->max_energy;
}
