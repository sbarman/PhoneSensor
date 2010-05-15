#include <iostream>
#include <alsa/asoundlib.h>
#include <semaphore.h>

#include "sound.h"
#include "gui.h"
#include "scope.h"
#include "cPeaks.c"
#include "sin.h"

// Maemo sound device is named "default"
const char* SOUND_DEVICE = "hw:0,0";

// desired sample rate of microphone input
const unsigned int SAMPLE_RATE = 8000;

// desired number of frames in a period
const snd_pcm_uframes_t FRAMES = 3840;

// desired number of periods in buffer
const unsigned int PERIODS = 32;

static void *log_thread(void *data) {
	alsa_shared *shared_data = (alsa_shared *) data;
	AlsaDataSource *source = shared_data->source;
	AlsaDataStream *datastream = source->getDataStream();
	short* buffer = (short *) malloc(source->frames_in_period
			* source->frame_size);
	FILE *log = shared_data->log;
	short data_points[DTAPTS];
	int index = 0;

	long count = 0;
	long data_count = source->getRate() * 15;

	while (datastream->running()) {

		datastream->get_data(buffer, source->frames_in_period);
		if (log != NULL) {
			for (short i = 0; i < source->frames_in_period; i++) {
				fprintf(log, "%lu,%d,%d\n", count, buffer[2 * i], buffer[2 * i + 1]);
				count++;
			}
			if (count > data_count) {
				printf("Ending log\n");
				fclose(log);
				log = NULL;
				shared_data->log = NULL;
			}
		}

		for (short i = 0; i < source->frames_in_period && index < DTAPTS; i += 2) {
			data_points[index] = buffer[i];
			index++;
		}

		if (index >= DTAPTS) {
			float pbm = calcBPM(data_points);
			shared_data->heart_rate = pbm;
			index = 0;
		}
	}

	fprintf(stderr, "Closing log thread\n");
	return NULL;
}

static void *sin_thread(void *args) {
	snd_args *sound_args = (snd_args *) args;
	sin_start(sound_args->argc, sound_args->argv, sound_args->function_info);
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

	source.start();

	// log to store all microphone data
	char fname[20] = "jlog";
	FILE* log = fopen(fname, "w");
	if (log == NULL) {
		fprintf(stderr, "Unable to open log\n");
	}
	//shared_data.log = stdout;
	shared_data.log = log;
	shared_data.heart_rate = 0;

	pthread_t log_tid;
	int ret = pthread_create(&log_tid, NULL, log_thread, &shared_data);
	if (ret < 0) {
		fprintf(stderr, "failed to create reader thread %d\n", ret);
		return ret;
	}

	fnc_info function_info;
	function_info.type = SIN;
	function_info.freq = 1000;
	function_info.ampl = .25;
	function_info.offset = 0;
	snd_args sound_args = { argc, argv, &function_info };

	pthread_t sin_tid;
	if (pthread_create(&sin_tid, NULL, sin_thread, &sound_args) != 0) {
		fprintf(stdout, "Count not create sin thread.\n");
	}

	shared_data.gui_data_stream = source.getDataStream();

	PhoneScopeGui gui(&shared_data);

	// will block until close button is hit
	gui.run();

	source.stop();

	pthread_join(log_tid, NULL);
	fprintf(stderr, "Closing scope\n");
	if (shared_data.log != NULL) {
		fclose(shared_data.log);
	}

	//	delete shared_data.unwritten_periods;
	sound.close();
}
