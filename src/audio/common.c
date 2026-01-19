// Common audio buffer handling functions
// Derived from cava's input/common.c

#include "audio.h"
#include <limits.h>

// Write samples to the cava input buffer
// Handles different bit depths and converts to double
int write_to_cava_input_buffers(int16_t size, unsigned char *buf, void *data) {
    struct audio_data *audio = (struct audio_data *)data;

    pthread_mutex_lock(&audio->lock);

    int bytes_per_sample = audio->format / 8;
    int samples = size / audio->channels;

    // Shift existing samples
    if (audio->samples_counter + samples > audio->cava_buffer_size) {
        int overflow = audio->samples_counter + samples - audio->cava_buffer_size;
        for (int i = 0; i < audio->cava_buffer_size - overflow; i++) {
            audio->cava_in[i] = audio->cava_in[i + overflow];
        }
        audio->samples_counter = audio->cava_buffer_size - samples;
    }

    // Convert and store new samples
    int n = 0;
    for (int i = 0; i < (int)(samples * audio->channels); i++) {
        switch (bytes_per_sample) {
        case 1: {
            const uint8_t *buf8 = (const uint8_t *)&buf[n];
            audio->cava_in[i + audio->samples_counter] = *buf8 * UCHAR_MAX;
            break;
        }
        case 3:
        case 4:
            if (audio->IEEE_FLOAT) {
                const float *ieee_float = (const float *)&buf[n];
                audio->cava_in[i + audio->samples_counter] = *ieee_float * USHRT_MAX;
            } else {
                const int32_t *buf32 = (const int32_t *)&buf[n];
                audio->cava_in[i + audio->samples_counter] = (double)*buf32 / USHRT_MAX;
            }
            break;
        default: {
            const int16_t *buf16 = (const int16_t *)&buf[n];
            audio->cava_in[i + audio->samples_counter] = *buf16;
            break;
        }
        }
        n += bytes_per_sample;
    }
    audio->samples_counter += samples;

    pthread_mutex_unlock(&audio->lock);
    return 0;
}

// Reset output buffers to zero (silence)
void reset_output_buffers(struct audio_data *data) {
    struct audio_data *audio = (struct audio_data *)data;

    pthread_mutex_lock(&audio->lock);
    for (uint16_t n = 0; n < audio->cava_buffer_size; n++) {
        audio->cava_in[n] = 0;
    }
    audio->samples_counter = audio->cava_buffer_size;
    pthread_mutex_unlock(&audio->lock);
}

// Signal that thread parameters are ready
void signal_threadparams(struct audio_data *audio) {
    pthread_mutex_lock(&audio->lock);
    audio->threadparams = 0;
    pthread_mutex_unlock(&audio->lock);
}

// Signal thread to terminate
void signal_terminate(struct audio_data *audio) {
    pthread_mutex_lock(&audio->lock);
    audio->terminate = 1;
    pthread_mutex_unlock(&audio->lock);
}
