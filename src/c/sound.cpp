#pragma once

#include <alsa/asoundlib.h>

#include "sound.h"

// Handle for the PCM device
snd_pcm_t *pcm_handle;

// Maemo sound device is named "default"
// J: THIS WORKS REALLY WELL FOR THE N900 EXTERNAL MIC
const char* SOUND_DEVICE = "hw:0,0";

// desired sample rate of microphone input
const unsigned int SAMPLE_RATE = 20000;

// desired number of frames in a period
const unsigned int FRAMES = 3840;

// desired number of periods in buffer
const unsigned int PERIODS = 32;

// actual sample rate
unsigned int sample_rate;

// actual frames in a period
snd_pcm_uframes_t frames;

// actual buffer size
snd_pcm_uframes_t buffer_size;

// actual number of periods in buffer
unsigned int periods;

int init_sound() {
	// record sound from mic
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	char *pcm_name = strdup(SOUND_DEVICE);

	/* Open PCM. The last parameter of this function is the mode.
	 * If this is set to 0, the standard mode is used. Possible
	 * other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.
	 * If SND_PCM_NONBLOCK is used, read / write access to the
	 * PCM device will return immediately. If SND_PCM_ASYNC is
	 * specified, SIGIO will be emitted whenever a period has
	 * been completely processed by the soundcard.
	 */
	if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
		fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
		return (-1);
	}

	snd_pcm_hw_params_t *hwparams;

	// Allocate the snd_pcm_hw_params_t structure on the stack.
	snd_pcm_hw_params_alloca(&hwparams);

	// Init hwparams with full configuration space
	if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "Can not configure this PCM device.\n");
		return (-1);
	}

	/* Set access type. This can be either SND_PCM_ACCESS_RW_INTERLEAVED or
	 * SND_PCM_ACCESS_RW_NONINTERLEAVED. There are also access types for MMAPed
	 * access, but this is beyond the scope of this introduction. */
	if (snd_pcm_hw_params_set_access(pcm_handle, hwparams,
			SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "Error setting access.\n");
		return (-1);
	}

	// Set sample format (Originally was SND_PCM_FORMAT_S16_LE)
	if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16)
			< 0) {
		fprintf(stderr, "Error setting format.\n");
		return (-1);
	}

	/* Set sample rate. If the exact rate is not supported
	 * by the hardware, use nearest possible rate. */
	sample_rate = SAMPLE_RATE;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &sample_rate, 0)
			< 0) {
		fprintf(stderr, "Error setting rate.\n");
		return (-1);
	}
	if (SAMPLE_RATE != sample_rate) {
		fprintf(stderr, "The rate %ud Hz is not supported by your hardware.\n"
			"==> Using %ud Hz instead.\n", SAMPLE_RATE, sample_rate);
	}

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
		fprintf(stderr, "Error setting channels.\n");
		return (-1);
	}

	snd_pcm_uframes_t size = FRAMES * PERIODS;
	buffer_size = size;
	if (snd_pcm_hw_params_set_buffer_size_max(pcm_handle, hwparams, &buffer_size)
			< 0) {
		fprintf(stderr, "Error setting buffer to maximium size.\n");
		return (-1);
	}

	// Set number of periods. Periods used to be called fragments.
	periods = PERIODS;
	if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0) {
		fprintf(stderr, "Error setting periods.\n");
		return (-1);
	}

	// Set period size. Periods used to be called fragments.
	frames = FRAMES;
	if (snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &frames, 0)
			< 0) {
		fprintf(stderr, "Error setting periods.\n");
		return (-1);
	}

	// Latency is given by  latency = frames * periods / (rate * bytes_per_frame)

	// Apply HW parameter settings to PCM device and prepare device
	if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "Error setting HW params.\n");
		return (-1);
	}

	snd_pcm_hw_params_get_period_size(hwparams, &frames, 0);
	snd_pcm_hw_params_get_periods(hwparams, &periods, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);

	return 1;
}

void close_sound() {
	// Stop PCM device and drop pending frames
	snd_pcm_drop(pcm_handle);

	// Stop PCM device after pending frames have been played
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
}

void print_info() {
	int val;

	printf("ALSA library version: %s\n", SND_LIB_VERSION_STR);

	printf("\nPCM stream types:\n");
	for (val = 0; val <= SND_PCM_STREAM_LAST; val++)
		printf("  %s\n", snd_pcm_stream_name((snd_pcm_stream_t) val));

	printf("\nPCM access types:\n");
	for (val = 0; val <= SND_PCM_ACCESS_LAST; val++)
		printf("  %s\n", snd_pcm_access_name((snd_pcm_access_t) val));

	printf("\nPCM formats:\n");
	for (val = 0; val <= SND_PCM_FORMAT_LAST; val++)
		if (snd_pcm_format_name((snd_pcm_format_t) val) != NULL)
			printf("  %s (%s)\n", snd_pcm_format_name((snd_pcm_format_t) val),
					snd_pcm_format_description((snd_pcm_format_t) val));

	printf("\nPCM subformats:\n");
	for (val = 0; val <= SND_PCM_SUBFORMAT_LAST; val++)
		printf("  %s (%s)\n", snd_pcm_subformat_name((snd_pcm_subformat_t) val),
				snd_pcm_subformat_description((snd_pcm_subformat_t) val));

	printf("\nPCM states:\n");
	for (val = 0; val <= SND_PCM_STATE_LAST; val++)
		printf("  %s\n", snd_pcm_state_name((snd_pcm_state_t) val));
}

