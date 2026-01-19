// ASCII Dancer - Terminal Audio Visualizer
// Main entry point and event loop
// v2.2: Effects system (particles, trails, breathing)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>

#include "audio/audio.h"
#include "fft/cavacore.h"
#include "dancer/dancer.h"
#include "render/render.h"
#include "render/colors.h"
#include "config/config.h"

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
    printf("  -t, --theme <name>    Color theme: default, fire, ice, neon, matrix, synthwave, mono\n");
    printf("  -c, --config <file>   Config file path (default: ~/.config/asciidancer/config.ini)\n");
    printf("      --no-ground       Disable ground line\n");
    printf("      --no-shadow       Disable shadow/reflection\n");
    printf("  -h, --help            Show this help\n");
    printf("\n");
    printf("Controls:\n");
    printf("  q, ESC                Quit\n");
    printf("  +/-                   Adjust sensitivity\n");
    printf("  t                     Cycle through themes\n");
    printf("  g                     Toggle ground line\n");
    printf("  r                     Toggle reflection/shadow\n");
    printf("  p                     Toggle particles\n");
    printf("  m                     Toggle motion trails\n");
    printf("  b                     Toggle breathing animation\n");
    printf("\n");
    printf("Themes:\n");
    for (int i = 0; i <= THEME_MONO; i++) {
        printf("  %s\n", colors_get_theme_preview((ColorTheme)i));
    }
}

static void cycle_theme(void) {
    cfg.theme = (cfg.theme + 1) % (THEME_MONO + 1);
    render_set_theme(cfg.theme);
}

int main(int argc, char *argv[]) {
    // Initialize config with defaults
    config_init(&cfg);
    
    // Load config file if exists
    char *config_path = config_get_default_path();
    if (config_path) {
        config_load(&cfg, config_path);
    }
    
    // Command line overrides
    char *source = cfg.audio_source;
    int use_pulse = 0;
    int target_fps = cfg.target_fps;
    int show_ground = cfg.show_ground;
    int show_shadow = cfg.show_shadow;

    // Parse command line
    static struct option long_options[] = {
        {"source",    required_argument, 0, 's'},
        {"pulse",     no_argument,       0, 'p'},
        {"fps",       required_argument, 0, 'f'},
        {"theme",     required_argument, 0, 't'},
        {"config",    required_argument, 0, 'c'},
        {"no-ground", no_argument,       0, 'G'},
        {"no-shadow", no_argument,       0, 'S'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

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
        "default", "fire", "ice", "neon", "matrix", "synthwave", "mono"
    };

    // Main loop
    while (running && !audio.terminate) {
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

        // Calculate frequency bands
        double bass, mid, treble;
        calculate_bands(cava_out, NUM_BARS, &bass, &mid, &treble);

        // Update dancer animation
        dancer_update(&dancer, bass, mid, treble);

        // Render
        render_clear();
        render_dancer(&dancer);
        render_bars(bass, mid, treble);

        snprintf(info_text, sizeof(info_text),
                 "sens:%.1f %s %s%s%s%s%s p:%d | %s",
                 sensitivity,
                 theme_names[cfg.theme],
                 show_ground ? "[G]" : "",
                 show_shadow ? "[R]" : "",
                 dancer_get_particles() ? "[P]" : "",
                 dancer_get_trails() ? "[M]" : "",
                 dancer_get_breathing() ? "[B]" : "",
                 dancer_get_particle_count(),
                 use_pulse ? "PulseAudio" : "PipeWire");
        render_info(info_text);
        render_refresh();

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
            sensitivity *= 1.2;
            if (sensitivity > 10.0) sensitivity = 10.0;
            break;
        case '-':
        case '_':
            sensitivity /= 1.2;
            if (sensitivity < 0.1) sensitivity = 0.1;
            break;
        case 't':
        case 'T':
            cycle_theme();
            break;
        case 'g':
        case 'G':
            show_ground = !show_ground;
            render_set_ground(show_ground);
            break;
        case 'r':
        case 'R':
            show_shadow = !show_shadow;
            render_set_shadow(show_shadow);
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
        case 'd':
        case 'D':
            debug_mode = !debug_mode;
            break;
        }

        // Wait for next frame
        nanosleep(&frame_time, NULL);
    }

    // Cleanup
    render_cleanup();

    audio.terminate = 1;
    pthread_join(audio_thread, NULL);
    pthread_mutex_destroy(&audio.lock);

    cava_destroy(plan);
    dancer_cleanup();
    free(cava_out);
    free(audio.source);
    free(audio.cava_in);

    printf("Goodbye!\n");
    return 0;
}
