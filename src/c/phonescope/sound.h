#ifndef SOUND
#define SOUND

#include <list>
#include <semaphore.h>

#include "datastream.h"

class AlsaSound {
	// Handle for the PCM device
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *hwparams;

public:
	int init(char* pcm_name, unsigned int sample_rate,
			snd_pcm_uframes_t frames, unsigned int periods);

	snd_pcm_t *get_pcm_handle();
	snd_pcm_uframes_t get_frames();
	snd_pcm_uframes_t get_buffer_size();
	unsigned int get_periods();
	unsigned int get_rate();

	void close();

};

class AlsaDataSource;

class AlsaDataStream : public DataStream {
	AlsaDataSource *source;
	unsigned int reader_position;

public:
	sem_t *unwritten_periods;

	AlsaDataStream(AlsaDataSource *source);
	unsigned int get_data(short *buffer, int size);
	unsigned int block_size_in_frames();
	unsigned int max_buffer_size_in_frames();
	unsigned int frame_size();
	bool running();
};

class AlsaDataSource {
public:
	// alsa related variables
	snd_pcm_t *pcm_handle;
	snd_pcm_uframes_t frames_in_period;
	int frame_size;

	// buffer related variables
	char *buffer;
	snd_pcm_uframes_t frames_in_buffer;
	int max_periods;

	// sizes given in number of frames
	unsigned int writer_position;

	pthread_t reader_tid;

	std::list<AlsaDataStream*> *streams;
	bool running;

	AlsaDataSource(AlsaSound sound);
	int start();
	AlsaDataStream *getDataStream();
	int stop();
};

#endif /*SOUND*/
