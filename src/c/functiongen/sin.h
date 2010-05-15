
enum fnc_type {
	SIN, TRIANGLE, SQUARE
};

struct snd_args {
	int argc;
	char **argv;
	fnc_info *function_info;
};

struct fnc_info {
	fnc_type type;
	double freq;
	double ampl;
	int offset;
};

struct async_private_data {
	signed short *samples;
	snd_pcm_channel_area_t *areas;
	double phase;
	fnc_info *function_info;
};

struct transfer_method {
	const char *name;
	snd_pcm_access_t access;
	int (*transfer_loop)(snd_pcm_t *handle, signed short *samples,
			snd_pcm_channel_area_t *areas, fnc_info *info);
};

void generate_sine(const snd_pcm_channel_area_t *areas,
		snd_pcm_uframes_t offset, int count, double *_phase, fnc_info * info);

int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params,
		snd_pcm_access_t access);

int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams);
int xrun_recovery(snd_pcm_t *handle, int err);

int write_loop(snd_pcm_t *handle, signed short *samples,
		snd_pcm_channel_area_t *areas);

int wait_for_poll(snd_pcm_t *handle, struct pollfd *ufds, unsigned int count);
int write_and_poll_loop(snd_pcm_t *handle, signed short *samples,
		snd_pcm_channel_area_t *areas, fnc_info *info);

void async_callback(snd_async_handler_t *ahandler);
int async_loop(snd_pcm_t *handle, signed short *samples,
		snd_pcm_channel_area_t *areas, fnc_info *info);

void async_direct_callback(snd_async_handler_t *ahandler);
int async_direct_loop(snd_pcm_t *handle, signed short *samples,
		snd_pcm_channel_area_t *areas, fnc_info *info);

int direct_loop(snd_pcm_t *handle, signed short *samples,
		snd_pcm_channel_area_t *areas, fnc_info *info);
int direct_write_loop(snd_pcm_t *handle, signed short *samples,
		snd_pcm_channel_area_t *areas, fnc_info *info);

void help(void);
int sin_start(int argc, char *argv[], fnc_info *func_info);
