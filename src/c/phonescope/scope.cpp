#include <iostream>
#include <alsa/asoundlib.h>
#include <semaphore.h>

#include "sound.h"
#include "gui.h"
#include "scope.h"

// Maemo sound device is named "default"
const char* SOUND_DEVICE = "hw:0,0";

// desired sample rate of microphone input
const unsigned int SAMPLE_RATE = 8000;

// desired number of frames in a period
const snd_pcm_uframes_t FRAMES = 3840;

// desired number of periods in buffer
const unsigned int PERIODS = 32;

static void *log_thread(void *data);

static void *log_thread(void *data) {
	alsa_shared *shared_data = (alsa_shared *) data;
	AlsaDataSource *source = shared_data->source;
	AlsaDataStream *datastream = source->getDataStream();
	short* buffer = (short *) malloc(source->frames_in_period
			* source->frame_size);
	FILE *log = shared_data->log;

	unsigned long count = 0;
	if (log != NULL) {
		while (shared_data->running) {

			datastream->get_data(buffer, source->frames_in_period);
			for (short i = 0; i < source->frames_in_period; i++) {
				fprintf(log, "%lu,%d,%d\n", count, buffer[2 * i], buffer[2 * i + 1]);
				//fprintf(log, "%d\n", buffer[2 * i]);
				count++;
			}
		}
	}
	fprintf(stderr, "Closing log thread\n");
	return NULL;
}

int main(int argc, char** argv) {

	AlsaSound sound;

	if (sound.init(strdup(SOUND_DEVICE), SAMPLE_RATE, FRAMES, PERIODS) < 0) {
		exit(-1);
	}

	snd_pcm_uframes_t frames = sound.get_frames();
	snd_pcm_uframes_t alsa_buffer_size = sound.get_buffer_size();
	unsigned int periods = sound.get_periods();
	unsigned int rate = sound.get_rate();

	fprintf(stderr,
			"Sound initialized with sample rate %ud Hz, buffer size of %ud bytes,"
				" period size of %ud bytes and %d periods\n", rate,
			(unsigned int) alsa_buffer_size, (unsigned int) frames, periods);

	// buffer to share between threads
	alsa_shared shared_data;
	AlsaDataSource source(sound);
	shared_data.source = &source;
	shared_data.buffer = source.buffer;
	shared_data.frames_in_buffer = source.frames_in_buffer;
	shared_data.writer_position = 0;

	// log to store all microphone data
	char fname[20] = "jlog";
	FILE* log = fopen(fname, "w");
	if (log == NULL) {
		fprintf(stderr, "Unable to open log\n");
	}
	//shared_data.log = stdout;
	shared_data.log = log;

	/*
	 fprintf(
	 stderr,
	 "Shared data initialized with %d frames in buffer, %d frames in period, "
	 "%d max periods, %d bytes per frame, %d buffer size and logging to %s\n",
	 (int) shared_data.frames_in_buffer, (int) shared_data.frames_in_period,
	 shared_data.max_periods, shared_data.frame_size, buffer_size, fname);
	 */

	PhoneScopeGui gui(&shared_data);
	shared_data.gui = &gui;

	pthread_t log_tid;
	int ret = pthread_create(&log_tid, NULL, log_thread, &shared_data);
	if (ret < 0) {
		fprintf(stderr, "failed to create reader thread %d\n", ret);
		return ret;
	}

	source.start();

	gui.run();

	source.stop();

	pthread_join(log_tid, NULL);
	fprintf(stderr, "Closing scope\n");
	fclose(log);

	//	delete shared_data.unwritten_periods;
	sound.close();
}
