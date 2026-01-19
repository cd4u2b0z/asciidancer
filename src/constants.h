/*
 * constants.h - Centralized magic numbers and tuning parameters
 * 
 * This file consolidates frequently-used constants that were previously
 * scattered throughout the codebase as magic numbers. Grouping them here
 * makes tuning easier and improves code readability.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/*==========================================================================
 * BPM Detection & Tempo
 *==========================================================================*/
#define BPM_MIN              40.0f   /* Minimum detectable BPM */
#define BPM_MAX              240.0f  /* Maximum detectable BPM */
#define BPM_DEFAULT          120.0f  /* Default/fallback BPM */

/* Interval (seconds) = 60 / BPM, so these are the inverse ranges */
#define BEAT_INTERVAL_MIN    0.25f   /* 60/240 = 0.25s (fastest beat) */
#define BEAT_INTERVAL_MAX    1.5f    /* 60/40 = 1.5s (slowest beat) */

#define TEMPO_SMOOTHING      0.15f   /* Low-pass filter alpha for BPM */
#define TEMPO_CLUSTER_TOL    0.15f   /* Cluster tolerance for histogram */
#define TEMPO_STABILITY_REQ  0.75f   /* Required stability for tempo lock */
#define TEMPO_LOCK_THRESHOLD 0.8f    /* Confidence needed to lock tempo */

/*==========================================================================
 * Energy Thresholds (0.0 - 1.0 normalized)
 *==========================================================================*/
#define ENERGY_SILENT        0.02f   /* Below this = silence */
#define ENERGY_IDLE          0.15f   /* IDLE pose threshold */
#define ENERGY_CALM          0.30f   /* CALM pose threshold */
#define ENERGY_GROOVE        0.55f   /* GROOVE pose threshold */
#define ENERGY_INTENSE       0.80f   /* INTENSE pose threshold */

/* Dynamic thresholds for adaptive energy analysis */
#define ENERGY_LOW_PERCENTILE   0.35f  /* 35th percentile = low threshold */
#define ENERGY_HIGH_PERCENTILE  0.75f  /* 75th percentile = high threshold */

/*==========================================================================
 * Audio Analysis
 *==========================================================================*/
#define AUDIO_SAMPLE_RATE    44100   /* Standard sample rate */
#define FFT_SIZE             1024    /* FFT window size */
#define FFT_BARS             256     /* Number of frequency bars */

#define DB_FLOOR             -60.0f  /* Minimum dB level (silence) */
#define DB_CEILING           0.0f    /* Maximum dB level */

/* Frequency bands (Hz) */
#define FREQ_BASS_MIN        20
#define FREQ_BASS_MAX        300
#define FREQ_MID_MIN         300
#define FREQ_MID_MAX         2000
#define FREQ_TREBLE_MIN      2000
#define FREQ_TREBLE_MAX      16000

/* Band weights for combined energy */
#define WEIGHT_BASS          1.2f
#define WEIGHT_MID           1.0f
#define WEIGHT_TREBLE        0.8f

/*==========================================================================
 * Animation & Physics
 *==========================================================================*/
#define TARGET_FPS           60
#define FRAME_TIME_MS        (1000.0 / TARGET_FPS)

/* Smoothing factors for energy response */
#define SMOOTH_ATTACK        0.6f    /* Fast response to energy increase */
#define SMOOTH_RELEASE       0.75f   /* Slower decay on energy decrease */
#define SMOOTH_LONG_TERM     0.995f  /* Very slow average tracking */

/* Physics spring-damper constants */
#define SPRING_STIFFNESS_BASE  15.0f
#define SPRING_STIFFNESS_MAX   25.0f  /* At max energy */
#define SPRING_DAMPING         0.8f

/* Pose transition timing */
#define POSE_MIN_DURATION      0.3f   /* Minimum time in a pose (seconds) */
#define POSE_HISTORY_SIZE      8      /* Avoid repeating recent N poses */
#define TRANSITION_CHANCE_BASE 0.3f   /* Base probability of pose change */
#define TRANSITION_CHANCE_MAX  0.8f   /* Max probability at high energy */

/* Blend speeds */
#define BLEND_SPEED_MIN        5.0f
#define BLEND_SPEED_MAX        15.0f

/*==========================================================================
 * Visual Effects
 *==========================================================================*/
/* Particle system */
#define PARTICLE_MAX           200
#define PARTICLE_SPAWN_RATE    0.15f  /* Energy threshold to spawn */
#define PARTICLE_LIFETIME      2.0f   /* Seconds */

/* Motion trails */
#define TRAIL_MAX_LENGTH       8
#define TRAIL_FADE_RATE        0.15f

/* Background effects */
#define BG_PARTICLE_MAX        100
#define BG_WAVE_SPEED          2.0f
#define BG_AURA_PULSE_RATE     1.5f

/*==========================================================================
 * Skeleton Geometry
 *==========================================================================*/
#define SKELETON_SHOULDER_WIDTH  0.14f
#define SKELETON_HIP_WIDTH       0.08f
#define SKELETON_TORSO_LENGTH    0.20f
#define SKELETON_UPPER_LEG       0.18f
#define SKELETON_LOWER_LEG       0.17f
#define SKELETON_UPPER_ARM       0.12f
#define SKELETON_LOWER_ARM       0.10f
#define SKELETON_HEAD_RADIUS     6      /* pixels */

/*==========================================================================
 * Braille Canvas
 *==========================================================================*/
#define BRAILLE_DOTS_X        2       /* Dots per cell horizontally */
#define BRAILLE_DOTS_Y        4       /* Dots per cell vertically */
#define BRAILLE_BASE          0x2800  /* Unicode braille block start */

/* Flood fill limits */
#define FLOOD_FILL_MAX_STACK  4096    /* Max pixels to process */

/*==========================================================================
 * Buffer Sizes
 *==========================================================================*/
#define TEXT_BUFFER_SMALL     64
#define TEXT_BUFFER_MEDIUM    256
#define TEXT_BUFFER_LARGE     1024

#define HISTORY_SIZE_DEFAULT  64
#define HISTORY_SIZE_LARGE    128

/*==========================================================================
 * Genre Detection Thresholds
 *==========================================================================*/
#define GENRE_ELECTRONIC_TREBLE  0.6f   /* High treble = electronic */
#define GENRE_HIPHOP_BASS        0.55f  /* Strong bass = hip-hop */
#define GENRE_ROCK_ENERGY        0.6f   /* High energy = rock */
#define GENRE_CLASSICAL_ENERGY   0.35f  /* Low energy = classical */
#define GENRE_AMBIENT_ENERGY     0.15f  /* Very low = ambient */

#define EASTER_EGG_CHANCE        0.15f  /* 15% chance for genre easter eggs */

/*==========================================================================
 * Color System
 *==========================================================================*/
#define COLOR_DEPTH_256       256
#define COLOR_PAIRS_MAX       256

/* Theme gradient stops */
#define GRADIENT_STEPS        8

#endif /* CONSTANTS_H */
