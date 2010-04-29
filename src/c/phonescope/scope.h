#ifndef SCOPE
#define SCOPE

struct alsa_shared {
	// ALSA related variables
	snd_pcm_t *pcm_handle;
	snd_pcm_uframes_t frames_in_period;
	int frame_size;

	// log file
	FILE* log;

	// buffer related variables
	char *buffer;
	snd_pcm_uframes_t frames_in_buffer;
	int max_periods;

	// sizes given in number of frames
	unsigned int writer_position;
	unsigned int log_position;

	// used to synchronize between pcm reader and log
	sem_t *unwritten_periods;

	// signals when to quit
	bool running;
};

static void *reader_thread(void *data);
static void *log_thread(void *data);

#endif /*SCOPE*/
