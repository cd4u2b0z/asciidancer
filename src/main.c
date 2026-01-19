// ASCII Dancer - Terminal Audio Visualizer
// Main entry point and event loop
// v3.0: Advanced BPM tracking, energy analysis, background effects

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <math.h>

#include "audio/audio.h"
#include "fft/cavacore.h"
#include "effects/particles.h"  // Include first for ParticleSystem typedef
#include "dancer/dancer.h"
#include "render/render.h"
#include "render/colors.h"
#include "config/config.h"
#include "audio/rhythm.h"
#include "ui/help_overlay.h"

// v3.0 modules
#include "audio/bpm_tracker.h"
#include "audio/energy_analyzer.h"
#include "effects/background_fx.h"

// v3.0+ modules
#include "export/frame_recorder.h"
#include "audio/audio_picker.h"
#include "ui/term_caps.h"
#include "ui/profiler.h"

// Default configuration
#define DEFAULT_RATE 44100
#define DEFAULT_CHANNELS 2
#define DEFAULT_FORMAT 16
#define NUM_BARS 24  // More bars for better frequency resolution

// Global state for signal handling
static volatile int running = 1;
static struct audio_data audio;
static Config cfg;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

static void print_usage(const char *name) {
    printf("Usage: %s [options]\n\n", name);
    printf("Options:\n");
    printf("  -s, --source <name>   Audio source (default: auto)\n");
#ifdef PIPEWIRE
    printf("  -p, --pulse           Use PulseAudio instead of PipeWire\n");
#endif
    printf("  -f, --fps <n>         Target framerate (default: %d)\n", cfg.target_fps);
    printf("  -t, --theme <name>    Color theme (13 available, press t to cycle)\n");
    printf("  -c, --config <file>   Config file path (default: ~/.config/braille-boogie/config.ini)\n");
    printf("      --no-ground       Disable ground line\n");
    printf("      --no-shadow       Disable shadow/reflection\n");
    printf("      --pick-source     Show audio source picker menu\n");
    printf("      --show-caps       Display terminal capabilities\n");
    printf("      --demo            Demo mode: all visual effects enabled\n");
    printf("  -h, --help            Show this help\n");
    printf("\n");
    printf("Controls:\n");
    printf("  ?, F1                 Toggle help overlay\n");
    printf("  q, ESC                Quit\n");
    printf("  +/-                   Adjust sensitivity\n");
    printf("  t                     Cycle through themes\n");
    printf("  g                     Toggle ground line\n");
    printf("  r                     Toggle reflection/shadow\n");
    printf("  p                     Toggle particles\n");
    printf("  m                     Toggle motion trails\n");
    printf("  b                     Toggle breathing animation\n");
    printf("  f                     Toggle background effects\n");
    printf("  e                     Cycle background effect types\n");
    printf("  x                     Toggle frame recording (export mode)\n");
    printf("  i                     Toggle performance profiler overlay\n");
    printf("\n");
    printf("Themes:\n");
    for (int i = 0; i < THEME_COUNT; i++) {
        printf("  %s\n", colors_get_theme_preview((ColorTheme)i));
    }
}

static void cycle_theme(void) {
    cfg.theme = (cfg.theme + 1) % THEME_COUNT;
    render_set_theme(cfg.theme);
}

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int main(int argc, char *argv[]) {
    // Initialize config with defaults
    config_init(&cfg);
    
    // Load config file if exists
    const char *config_path = config_get_default_path();
    if (config_path) {
        config_load(&cfg, config_path);
    }
    
    // Command line overrides
    const char *source = cfg.audio_source;
    int use_pulse = 0;
    int target_fps = cfg.target_fps;
    int show_ground = cfg.show_ground;
    int show_shadow = cfg.show_shadow;

    // Parse command line
    static struct option long_options[] = {
        {"source",      required_argument, 0, 's'},
        {"pulse",       no_argument,       0, 'p'},
        {"fps",         required_argument, 0, 'f'},
        {"theme",       required_argument, 0, 't'},
        {"config",      required_argument, 0, 'c'},
        {"no-ground",   no_argument,       0, 'G'},
        {"no-shadow",   no_argument,       0, 'S'},
        {"pick-source", no_argument,       0, 'P'},
        {"show-caps",   no_argument,       0, 'C'},
        {"demo",        no_argument,       0, 'D'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int show_picker = 0;
    int show_caps = 0;
    int demo_mode = 0;

    int opt;
    while ((opt = getopt_long(argc, argv, "s:pf:t:c:h", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            source = optarg;
            strncpy(cfg.audio_source, source, sizeof(cfg.audio_source) - 1);
            break;
        case 'p':
            use_pulse = 1;
            break;
        case 'f':
            target_fps = atoi(optarg);
            if (target_fps < 1 || target_fps > 120) {
                fprintf(stderr, "FPS must be between 1 and 120\n");
                return 1;
            }
            cfg.target_fps = target_fps;
            break;
        case 't':
            cfg.theme = config_theme_from_name(optarg);
            break;
        case 'c':
            config_load(&cfg, optarg);
            break;
        case 'G':
            show_ground = 0;
            cfg.show_ground = 0;
            break;
        case 'P':
            show_picker = 1;
            break;
        case 'C':
            show_caps = 1;
            break;
        case 'D':
            demo_mode = 1;
            break;
        case 'S':
            show_shadow = 0;
            cfg.show_shadow = 0;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    // Check for audio backend availability
#if !defined(PIPEWIRE) && !defined(PULSE)
    fprintf(stderr, "Error: No audio backend compiled in. Install libpipewire or libpulse dev packages.\n");
    return 1;
#endif

    // Show terminal capabilities if requested
    if (show_caps) {
        TerminalCaps *caps = term_caps_detect();
        if (caps) {
            term_caps_print(caps);
            term_caps_free(caps);
        }
        return 0;
    }

    // Show audio source picker if requested
    if (show_picker) {
        AudioSourceList *sources = audio_picker_enumerate(use_pulse);
        if (sources && sources->count > 0) {
            char *selected = audio_picker_show_menu(sources);
            if (selected) {
                source = selected;
                strncpy(cfg.audio_source, source, sizeof(cfg.audio_source) - 1);
                printf("Selected source: %s\n", source);
                // Note: selected is from sources struct, don't free it separately
            }
            audio_picker_free(sources);
        }
    }

#ifndef PIPEWIRE
    if (!use_pulse) {
        fprintf(stderr, "PipeWire not available, using PulseAudio\n");
        use_pulse = 1;
    }
#endif

#ifndef PULSE
    if (use_pulse) {
        fprintf(stderr, "PulseAudio not available\n");
        return 1;
    }
#endif

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize audio data structure
    memset(&audio, 0, sizeof(audio));
    audio.source = strdup(source);
    audio.rate = DEFAULT_RATE;
    audio.channels = DEFAULT_CHANNELS;
    audio.format = DEFAULT_FORMAT;
    audio.input_buffer_size = BUFFER_SIZE * audio.channels;
    audio.cava_buffer_size = 16384;
    audio.cava_in = (double *)calloc(audio.cava_buffer_size, sizeof(double));
    audio.terminate = 0;
    audio.threadparams = 0;
    audio.active = 1;
    audio.remix = 1;
    audio.virtual_node = 1;

    pthread_mutex_init(&audio.lock, NULL);

    // Start audio capture thread
    pthread_t audio_thread;
    int thread_result = -1;

#ifdef __APPLE__
    // macOS uses CoreAudio
    thread_result = pthread_create(&audio_thread, NULL, input_coreaudio, (void *)&audio);
#else
    // Linux audio backends
#ifdef PULSE
    if (use_pulse) {
        if (strcmp(audio.source, "auto") == 0) {
            getPulseDefaultSink((void *)&audio);
        }
        thread_result = pthread_create(&audio_thread, NULL, input_pulse, (void *)&audio);
    }
#endif

#ifdef PIPEWIRE
    if (!use_pulse) {
        thread_result = pthread_create(&audio_thread, NULL, input_pipewire, (void *)&audio);
    }
#endif
#endif // __APPLE__

    if (thread_result != 0) {
        fprintf(stderr, "Failed to start audio thread\n");
        free(audio.source);
        free(audio.cava_in);
        return 1;
    }

    // Wait for audio thread to initialize
    struct timespec wait = {.tv_sec = 0, .tv_nsec = 10000000};  // 10ms
    int timeout = 500;  // 5 seconds total
    while (audio.threadparams != 0 && timeout > 0 && !audio.terminate) {
        nanosleep(&wait, NULL);
        timeout--;
    }

    if (audio.terminate) {
        fprintf(stderr, "Audio thread error: %s\n", audio.error_message);
        pthread_join(audio_thread, NULL);
        free(audio.source);
        free(audio.cava_in);
        return 1;
    }

    // Initialize FFT processing
    struct cava_plan *plan = cava_init(NUM_BARS, audio.rate, audio.channels, 1,
                                       0.77, 50, 10000);
    if (plan->status != 0) {
        fprintf(stderr, "FFT init error: %s\n", plan->error_message);
        audio.terminate = 1;
        pthread_join(audio_thread, NULL);
        free(audio.source);
        free(audio.cava_in);
        free(plan);
        return 1;
    }

    // Allocate output buffer
    double *cava_out = (double *)calloc(NUM_BARS, sizeof(double));

    // Initialize dancer state
    struct dancer_state dancer;
    dancer_init(&dancer);

    // Initialize rhythm detection (v2.3)
    RhythmState *rhythm = rhythm_init();
    float spectrum[NUM_BARS];  // Spectrum buffer for rhythm analysis

    // Initialize help overlay (v2.4+)
    HelpOverlay *help = help_overlay_create();

    // Initialize v3.0 modules
    BPMTracker *bpm_tracker = bpm_tracker_create();
    EnergyAnalyzer *energy = energy_analyzer_create();
    
    // Background FX needs particle system - get from dancer
    ParticleSystem *particles = dancer_get_particle_system();
    BackgroundFX *bg_fx = background_fx_create(particles);
    BackgroundFXType current_bg_effect = BG_AMBIENT_FIELD;  // Match bg_fx default
    bool bg_fx_enabled = false;

    // v3.0+ modules (initialized after ncurses)
    FrameRecorder *recorder = NULL;
    bool recording = false;
    Profiler *profiler = NULL;
    bool show_profiler = false;
    double audio_start = 0, update_start = 0, render_start = 0;

    // Initialize ncurses with 256-color support
    if (render_init() != 0) {
        fprintf(stderr, "Failed to initialize ncurses\n");
        audio.terminate = 1;
        pthread_join(audio_thread, NULL);
        cava_destroy(plan);
        free(cava_out);
        free(audio.source);
        free(audio.cava_in);
        return 1;
    }

    // Apply config settings
    render_set_theme(cfg.theme);
    render_set_ground(show_ground);
    render_set_shadow(show_shadow);
    dancer_set_ground(show_ground);   // Braille dancer ground
    dancer_set_shadow(show_shadow);   // Braille dancer shadow

    // Demo mode: enable all visual effects for maximum wow
    if (demo_mode) {
        dancer_set_particles(true);
        dancer_set_trails(true);
        dancer_set_breathing(true);
        bg_fx_enabled = true;
        background_fx_enable(bg_fx, true);
        background_fx_set_type(bg_fx, BG_AMBIENT_FIELD);
        cfg.theme = THEME_SYNTHWAVE;  // Eye-catching theme
        render_set_theme(cfg.theme);
    }

    // Initialize v3.0+ modules (needs ncurses for screen size)
    int sw, sh;
    getmaxyx(stdscr, sh, sw);
    recorder = frame_recorder_create(sw, sh, NULL);  // NULL = use timestamp dir
    profiler = profiler_create();

    // Main loop timing
    struct timespec frame_time = {
        .tv_sec = 0,
        .tv_nsec = 1000000000 / target_fps
    };

    double sensitivity = cfg.sensitivity;
    char info_text[256];
    int debug_mode = 0;

    // Theme names for display
    const char *theme_names[] = {
        "default", "fire", "ice", "neon", "matrix", "synthwave", "mono",
        "aurora", "sunset", "ocean", "candy", "vapor", "ember"
    };
    
    // Effect names for display
    const char *effect_names[] = {
        "None", "Ambient", "Waves", "Aura", "Burst", "Ribbons", "Rain", "Vortex"
    };

    // Main loop
    while (running && !audio.terminate) {
        // Start profiler frame timing
        if (show_profiler) {
            profiler_frame_start(profiler);
            audio_start = get_time_ms();
        }

        // Process audio
        pthread_mutex_lock(&audio.lock);
        int samples_available = audio.samples_counter;
        if (samples_available > 0) {
            cava_execute(audio.cava_in, audio.samples_counter, cava_out, plan);
            audio.samples_counter = 0;
        }
        pthread_mutex_unlock(&audio.lock);

        // Apply sensitivity
        for (int i = 0; i < NUM_BARS; i++) {
            cava_out[i] *= sensitivity;
            if (cava_out[i] > 1.0) cava_out[i] = 1.0;
        }

        // Convert to float spectrum for rhythm analysis
        for (int i = 0; i < NUM_BARS; i++) {
            spectrum[i] = (float)cava_out[i];
        }
        
        // Update visualizer with raw spectrum (cava-style)
        dancer_update_spectrum(spectrum, NUM_BARS);

        // Mark audio time
        if (show_profiler) {
            profiler_mark_audio(profiler, get_time_ms() - audio_start);
            update_start = get_time_ms();
        }

        // Update rhythm detection (v2.3)
        rhythm_update(rhythm, spectrum, NUM_BARS, 1.0 / target_fps);

        // Calculate frequency bands
        double bass, mid, treble;
        calculate_bands(cava_out, NUM_BARS, &bass, &mid, &treble);

        // v3.0: Update BPM tracker on detected onsets
        float dt = 1.0f / target_fps;
        if (rhythm_onset_detected(rhythm)) {
            static double elapsed_time = 0.0;
            elapsed_time += dt;
            bpm_tracker_tap(bpm_tracker, elapsed_time);
        }
        bpm_tracker_update(bpm_tracker, dt);

        // v3.0: Update energy analyzer with frequency bands
        energy_analyzer_update_bands(energy, 
            (float)bass * 0.5f,   // sub_bass estimate
            (float)bass,
            (float)(bass + mid) * 0.5f,  // low_mid
            (float)mid,
            (float)(mid + treble) * 0.5f,  // high_mid
            (float)treble);
        
        // Update pace based on BPM and onset strength
        energy_analyzer_update_pace(energy,
            bpm_tracker_get_bpm(bpm_tracker),
            rhythm_get_onset_strength(rhythm),
            rhythm_onset_detected(rhythm) ? 1.0f : 0.0f);

        // v3.0: Update background effects
        if (bg_fx_enabled && bg_fx) {
            background_fx_update(bg_fx, dt);
            background_fx_update_audio(bg_fx,
                energy_analyzer_get_smoothed(energy),
                (float)bass, (float)mid, (float)treble,
                rhythm_onset_detected(rhythm));
            background_fx_update_bands(bg_fx,
                (float)bass * 0.5f, (float)bass,
                (float)(bass + mid) * 0.5f, (float)mid,
                (float)(mid + treble) * 0.5f, (float)treble);
        }

        // Update dancer animation
        // Update dancer with rhythm info (v2.3)
        dancer_update_with_rhythm(&dancer, bass, mid, treble,
                                  rhythm_get_phase(rhythm),
                                  rhythm_get_bpm(rhythm),
                                  rhythm_onset_detected(rhythm),
                                  rhythm_get_onset_strength(rhythm));

        // Mark update time
        if (show_profiler) {
            profiler_mark_update(profiler, get_time_ms() - update_start);
            render_start = get_time_ms();
        }

        // Render
        render_clear();
        render_dancer(&dancer);
        render_bars(bass, mid, treble);

        // Update and render help overlay
        help_overlay_update(help, 1.0f / target_fps);
        if (help_overlay_is_active(help)) {
            int help_sw, help_sh;
            getmaxyx(stdscr, help_sh, help_sw);
            help_overlay_render(help, help_sw, help_sh,
                              theme_names[cfg.theme],
                              bpm_tracker_get_bpm(bpm_tracker), sensitivity,
                              show_ground, show_shadow,
                              dancer_get_particles(), dancer_get_trails(),
                              dancer_get_breathing());
        }

        // v3.0: Enhanced info display with confidence and energy zone
        const char *zone_name = energy_analyzer_get_zone_name(energy);
        float bpm_conf = bpm_tracker_get_confidence(bpm_tracker);
        float energy_ovr = dancer_get_energy_override();
        bool energy_locked = dancer_is_energy_locked();
        snprintf(info_text, sizeof(info_text),
                 "%.0fbpm(%d%%) %s %s%s%s%s%s%s%s%s%s%s p:%d",
                 bpm_tracker_get_bpm(bpm_tracker),
                 (int)(bpm_conf * 100),
                 zone_name,
                 theme_names[cfg.theme],
                 show_ground ? "[G]" : "",
                 show_shadow ? "[R]" : "",
                 dancer_get_particles() ? "[P]" : "",
                 dancer_get_trails() ? "[M]" : "",
                 dancer_get_breathing() ? "[B]" : "",
                 bg_fx_enabled ? "[FX]" : "",
                 bg_fx_enabled ? effect_names[current_bg_effect] : "",
                 recording ? "[REC]" : "",
                 energy_locked ? "[LOCK]" : (fabsf(energy_ovr) > 0.05f ? 
                     (energy_ovr > 0 ? "[+E]" : "[-E]") : ""),
                 dancer_get_particle_count());
        render_info(info_text);
        
        // Mark render time
        if (show_profiler) {
            profiler_mark_render(profiler, get_time_ms() - render_start);
            profiler_frame_end(profiler);
            
            // Update counts and render
            int particle_count = dancer_get_particle_count();
            int trail_count = dancer_get_trails() ? 100 : 0;
            profiler_set_counts(profiler, particle_count, trail_count);
            profiler_render(profiler);
        }
        
        render_refresh();

        // v3.0+: Capture frame if recording
        if (recording && recorder) {
            frame_recorder_capture(recorder);
        }

        // Handle input
        int ch = render_getch();
        switch (ch) {
        case 'q':
        case 'Q':
        case 27:  // ESC
            running = 0;
            break;
        case '+':
        case '=':
        case KEY_UP:  // Up arrow - increase energy
            // v3.1: Increase dancer energy
            dancer_adjust_energy(0.25f);
            break;
        case '-':
        case '_':
        case KEY_DOWN:  // Down arrow - decrease energy
            // v3.1: Decrease dancer energy
            dancer_adjust_energy(-0.25f);
            break;
        case 'l':
        case 'L':
            // v3.1: Toggle energy lock (ignore audio)
            dancer_toggle_energy_lock();
            break;
        case '[':
            // v3.1: Trigger counter-clockwise spin
            dancer_trigger_spin(-1);
            break;
        case ']':
            // v3.1: Trigger clockwise spin
            dancer_trigger_spin(1);
            break;
        case 't':
        case 'T':
            cycle_theme();
            break;
        case 'g':
        case 'G':
            show_ground = !show_ground;
            render_set_ground(show_ground);
            dancer_set_ground(show_ground);  // Braille dancer ground
            break;
        case 'r':
        case 'R':
            show_shadow = !show_shadow;
            render_set_shadow(show_shadow);
            dancer_set_shadow(show_shadow);  // Braille dancer shadow
            break;
        case 'p':
        case 'P':
            dancer_set_particles(!dancer_get_particles());
            break;
        case 'm':
        case 'M':
            dancer_set_trails(!dancer_get_trails());
            break;
        case 'b':
        case 'B':
            dancer_set_breathing(!dancer_get_breathing());
            break;
        case 'f':
        case 'F':
            // v3.0: Toggle background effects
            bg_fx_enabled = !bg_fx_enabled;
            background_fx_enable(bg_fx, bg_fx_enabled);
            break;
        case 'e':
        case 'E':
            // v3.0: Cycle background effect type
            if (bg_fx) {
                current_bg_effect = (current_bg_effect + 1) % BG_COUNT;
                background_fx_set_type(bg_fx, current_bg_effect);
                if (current_bg_effect != BG_NONE && !bg_fx_enabled) {
                    bg_fx_enabled = true;
                    background_fx_enable(bg_fx, true);
                }
            }
            break;
        case 'x':
        case 'X':
            // v3.0+: Toggle frame recording
            if (recorder) {
                if (recording) {
                    frame_recorder_stop(recorder);
                    recording = false;
                } else {
                    frame_recorder_start(recorder);
                    recording = true;
                }
            }
            break;
        case 'i':
        case 'I':
            // v3.0+: Toggle profiler
            show_profiler = !show_profiler;
            break;
        case 'v':
        case 'V':
            // Toggle audio visualizer bars
            dancer_set_visualizer(!dancer_get_visualizer());
            break;
        case 'd':
        case 'D':
            debug_mode = !debug_mode;
            break;
        case '?':
        case KEY_F(1):
            help_overlay_toggle(help);
            break;
        }

        // Wait for next frame
        nanosleep(&frame_time, NULL);
    }

    // v3.0+ cleanup
    if (recording && recorder) {
        frame_recorder_stop(recorder);
    }
    frame_recorder_destroy(recorder);
    profiler_destroy(profiler);

    // Cleanup
    render_cleanup();

    audio.terminate = 1;
    pthread_join(audio_thread, NULL);
    pthread_mutex_destroy(&audio.lock);

    cava_destroy(plan);
    dancer_cleanup();
    rhythm_destroy(rhythm);
    help_overlay_destroy(help);
    
    // v3.0 cleanup
    bpm_tracker_destroy(bpm_tracker);
    energy_analyzer_destroy(energy);
    background_fx_destroy(bg_fx);
    
    free(cava_out);
    free(audio.source);
    free(audio.cava_in);

    printf("Goodbye!\n");
    return 0;
}
