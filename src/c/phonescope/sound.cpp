#include <alsa/asoundlib.h>
#include <list>
#include <string.h>
#include <stdio.h>

#include "sound.h"

int AlsaSound::init(char* pcm_name, unsigned int sample_rate,
		snd_pcm_uframes_t frames, unsigned int periods) {
	// record sound from mic
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

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
	unsigned int new_sample_rate = sample_rate;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &new_sample_rate, 0)
			< 0) {
		fprintf(stderr, "Error setting rate.\n");
		return (-1);
	}
	if (new_sample_rate != sample_rate) {
		fprintf(stderr, "The rate %ud Hz is not supported by your hardware.\n"
			"==> Using %ud Hz instead.\n", sample_rate, new_sample_rate);
	}

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
		fprintf(stderr, "Error setting channels.\n");
		return (-1);
	}

	snd_pcm_uframes_t buffer_size = frames * periods;
	if (snd_pcm_hw_params_set_buffer_size_max(pcm_handle, hwparams, &buffer_size)
			< 0) {
		fprintf(stderr, "Error setting buffer to maximium size.\n");
		return (-1);
	}

	// Set number of periods. Periods used to be called fragments.
	if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0) {
		fprintf(stderr, "Error setting periods.\n");
		return (-1);
	}

	// Set period size. Periods used to be called fragments.
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

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_hw_params_current(pcm_handle, hwparams);

	return 1;
}

snd_pcm_t *AlsaSound::get_pcm_handle() {
	return pcm_handle;
}

snd_pcm_uframes_t AlsaSound::get_frames() {
	snd_pcm_uframes_t frames;
	snd_pcm_hw_params_get_period_size(hwparams, &frames, 0);
	return frames;
}

snd_pcm_uframes_t AlsaSound::get_buffer_size() {
	snd_pcm_uframes_t alsa_buffer_size;
	snd_pcm_hw_params_get_buffer_size(hwparams, &alsa_buffer_size);
	return alsa_buffer_size;
}

unsigned int AlsaSound::get_periods() {
	unsigned int periods;
	snd_pcm_hw_params_get_periods(hwparams, &periods, 0);
	return periods;
}

unsigned int AlsaSound::get_rate() {
	unsigned int rate;
	snd_pcm_hw_params_get_rate(hwparams, &rate, 0);
	return rate;
}

void AlsaSound::close() {
	// Stop PCM device and drop pending frames
	snd_pcm_drop( pcm_handle);

	// Stop PCM device after pending frames have been played
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
}

// AlsaDataSource functions
AlsaDataSource::AlsaDataSource(AlsaSound sound) {
	// info from AlsaSound
	unsigned int rate = sound.get_rate();

	pcm_handle = sound.get_pcm_handle();

	frames_in_period = sound.get_frames();
	// create a buffer for ~1 sec
	max_periods = rate / frames_in_period;
	frames_in_buffer = max_periods * frames_in_period;
	// each frame should be 4 bytes (2 channels * 2 bytes per channel)
	frame_size = snd_pcm_frames_to_bytes(pcm_handle, 1);
	int buffer_size = frame_size * frames_in_buffer;

	buffer = (char *) malloc(buffer_size);
	if (buffer == NULL) {
		fprintf(stderr, "Error. Not enough memory\n");
	}
	memset(buffer, 0, buffer_size);

	writer_position = 0;
	running = true;
	streams = new std::list<AlsaDataStream*>();
}

static void *reader_thread(void *arg) {
	AlsaDataSource *source = (AlsaDataSource *) arg;
	snd_pcm_t *pcm_handle = source->pcm_handle;

	while (source->running) {
		int frames_left = source->frames_in_period;
		while (frames_left > 0) {
			// Use a buffer large enough to hold one period
			signed short *buffer = (signed short *) (source->buffer
					+ (source->frame_size * source->writer_position));

			// Read audio data from mic
			int pcmreturn;
			while ((pcmreturn = snd_pcm_readi(pcm_handle, buffer, frames_left)) < 0) {
				if (pcmreturn == -EPIPE) {
					/* EPIPE means overrun */
					fprintf(stderr, "Overrun occurred\n");
				} else if (pcmreturn < 0) {
					fprintf(stderr, "Error from read: %d, %s\n", pcmreturn, snd_strerror(
							pcmreturn));
				}
				exit(1);
				snd_pcm_prepare(pcm_handle);
			}
			if (pcmreturn != (int) frames_left) {
				fprintf(stderr, "Short read, read %d frames\n", pcmreturn);
			}

			frames_left -= pcmreturn;
			source->writer_position += pcmreturn;
			if (source->writer_position >= source->frames_in_buffer) {
				source->writer_position -= source->frames_in_buffer;
			}
		}

		std::list<AlsaDataStream*>::iterator it;
		// Alert the logger more data has arrived
		for (it = source->streams->begin(); it != source->streams->end(); it++) {
			sem_post((*it)->unwritten_periods);
		}

		// Check to see if we have overwritten the enture
		//int num_unwritten_periods;
		//sem_getvalue(shared_data->unwritten_periods, &num_unwritten_periods);
		//if (num_unwritten_periods > shared_data->max_periods) {
		//	fprintf(stderr, "Log not able to keep up with pcm_read\n");
		//}
	}

	fprintf(stderr, "Closing reader thread\n");
	return NULL;
}

int AlsaDataSource::start() {
	int ret = pthread_create(&reader_tid, NULL, reader_thread, this);
	if (ret < 0) {
		fprintf(stderr, "failed to create reader thread %d\n", ret);
		return ret;
	}
	return 1;
}

AlsaDataStream *AlsaDataSource::getDataStream() {
	AlsaDataStream *stream = new AlsaDataStream(this);
	streams->push_back(stream);
	return stream;
}

int AlsaDataSource::stop() {
	running = false;
	pthread_join(reader_tid, NULL);
	return 1;
}

AlsaDataStream::AlsaDataStream(AlsaDataSource *s) {
	source = s;
	reader_position = source->writer_position;

	unwritten_periods = new sem_t();
	sem_init(unwritten_periods, false, 0);
}

// AlsaDataStream functions
unsigned int AlsaDataStream::get_data(short *buffer, int frames) {
	snd_pcm_uframes_t frames_in_period = source->frames_in_period;
	int extra_frames = frames % frames_in_period;
	frames -= extra_frames;

	while (source->running && frames > 0) {
		signed short *sourcebuffer = (signed short *) (source->buffer
				+ (source->frame_size * reader_position));

		sem_wait( unwritten_periods);
		memcpy(buffer, sourcebuffer, source->frame_size * frames_in_period);

		frames -= frames_in_period;

		// increment the current position
		reader_position += frames_in_period;
		buffer += 2 * frames_in_period;

		// reset if we reached the end of buffer
		if (reader_position >= source->frames_in_buffer) {
			reader_position -= source->frames_in_buffer;
		}
	}
	return 0;
}

unsigned int AlsaDataStream::block_size_in_frames() {
	return source->frames_in_period;
}

unsigned int AlsaDataStream::max_buffer_size_in_frames() {
	return source->frames_in_buffer;
}

unsigned int AlsaDataStream::frame_size() {
	return source->frame_size;
}

bool AlsaDataStream::running() {
	return source->running;
}
