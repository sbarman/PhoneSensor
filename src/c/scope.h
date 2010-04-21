#ifndef SCOPE
#define SCOPE

struct alsa_shared {
	snd_pcm_t *pcm_handle;
	snd_pcm_uframes_t frames_in_period;
	unsigned int max_periods;
	snd_pcm_uframes_t frames_in_buffer;
	unsigned int frame_size;
	signed short *buffer;

	FILE* log;

	// in frames
	unsigned int writer_position;
	unsigned int log_position;
	sem_t *unwritten_periods;

	bool running;
};

static void *reader_thread(void *data);
static void *log_thread(void *data);

#endif /*SCOPE*/
