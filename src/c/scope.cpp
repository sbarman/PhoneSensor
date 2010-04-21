#include <iostream>
#include <cmath>
#include <fftw3.h>
#include <alsa/asoundlib.h>
#include <SDL/SDL.h>
#include <semaphore.h>

#include "sound.h"
#include "graphics.h"
#include "scope.h"

/* Sound related variables */
extern snd_pcm_t *pcm_handle;
extern snd_pcm_uframes_t frames;
extern snd_pcm_uframes_t buffer_size;
extern unsigned int sample_rate;
extern unsigned int periods;

/* Graphics related variables */
extern SDL_Surface *screen;

static void *reader_thread(void *data) {
	alsa_shared *shared_data = (alsa_shared *) data;
	fprintf(stdout, "Buffer address is %x\n", shared_data->buffer);
	while (shared_data->running) {
		int frames_left = frames;
		while (frames_left > 0) {
			// Use a buffer large enough to hold one period
			// 2 bytes/sample, 2 channels
			signed short *buffer = (signed short *) (((char*) shared_data->buffer)
					+ (shared_data->frame_size * shared_data->writer_position));

			//	fprintf(stdout,
			//			"Reading from address %x with writer position %d, shared buffer %x,
			//frame size %d for %d frames\n",
			//			buffer, shared_data->writer_position, shared_data->buffer,
			//shared_data->frame_size, frames_left);

			// Read audio data from mic
			int pcmreturn;
			while ((pcmreturn = snd_pcm_readi(pcm_handle, buffer, frames_left)) < 0) {
				if (pcmreturn == -EPIPE) {
					/* EPIPE means overrun */
					fprintf(stdout, "Overrun occurred\n");
				} else if (pcmreturn < 0) {
					fprintf(stdout, "Error from read: %d, %s\n", pcmreturn, snd_strerror(
							pcmreturn));
				}
				exit(1);
				snd_pcm_prepare(pcm_handle);
			}
			if (pcmreturn != (int) frames) {
				fprintf(stdout, "Short read, read %d frames\n", pcmreturn);
			}

			frames_left -= pcmreturn;
			shared_data->writer_position += pcmreturn;
			if (shared_data->writer_position > shared_data->frames_in_buffer) {
				shared_data->writer_position -= shared_data->frames_in_buffer;
			}
		}
		sem_post(shared_data->unwritten_periods);
		int num_unwritten_periods;
		sem_getvalue(shared_data->unwritten_periods, &num_unwritten_periods);
		if (num_unwritten_periods > shared_data->max_periods) {
			fprintf(stdout, "Log not able to keep up with pcm_read\n");
		}
	}

	while()
	fprintf(stdout, "Closing reader thread\n");
	return NULL;
}

static void *log_thread(void *data) {
	alsa_shared *shared_data = (alsa_shared *) data;
	signed short *buffer = shared_data->buffer;
	sem_t *unwritten_periods = shared_data->unwritten_periods;
	FILE *log = shared_data->log;

	int num_unwritten_periods = sem_getvalue(unwritten_periods,
			&num_unwritten_periods);
	unsigned long count = 0;

	if (log != NULL) {
		while (shared_data->running || num_unwritten_periods > 0) {
			sem_wait(unwritten_periods);
			int pos = shared_data->log_position;
			for (snd_pcm_uframes_t i = pos; i < pos + frames; i++) {
				fprintf(log, "%lu,%d,%d\n", count, buffer[2 * i], buffer[2 * i + 1]);
				count++;
			}
			shared_data->log_position += frames;
			if (shared_data->log_position >= shared_data->frames_in_buffer) {
				shared_data->log_position -= shared_data->frames_in_buffer;
			}
			sem_getvalue(unwritten_periods, &num_unwritten_periods);
		}
	}
	fprintf(stdout, "Closing log thread\n");
	return NULL;
}

int main(int argc, char** argv) {

	if (!init_graphics()) {
		exit(-1);
	} else {
		fprintf(stdout, "Graphics initialized\n");
	}

	if (!init_sound()) {
		exit(-1);
	} else {
		fprintf(stdout,
				"Sound initialized with sample rate %ud Hz, buffer size of %ud bytes,"
					" period size of %ud bytes and %d periods\n", sample_rate,
				(unsigned int) buffer_size, (unsigned int) frames, periods);
	}

	// buffer to share between threads
	alsa_shared shared_data;
	shared_data.pcm_handle = pcm_handle;
	shared_data.frames_in_period = frames;
	shared_data.max_periods = sample_rate / frames;
	shared_data.frames_in_buffer = shared_data.max_periods
			* shared_data.frames_in_period;

	// each frame is 2 channels * 2 bytes per channel
	shared_data.frame_size = 4;
	int buffer_size = shared_data.frame_size * shared_data.frames_in_buffer;
	shared_data.buffer = (signed short *) malloc(buffer_size);
	memset(shared_data.buffer, 0, buffer_size);
	shared_data.writer_position = 0;
	shared_data.log_position = 0;

	shared_data.unwritten_periods = new sem_t();
	sem_init(shared_data.unwritten_periods, false, 0);
	shared_data.running = true;

	// log to store all microphone data
	char fname[20] = "jlog";
	FILE* log = fopen(fname, "w");
	if (log == NULL) {
		fprintf(stdout, "Unable to open log\n");
	}
	shared_data.log = log;

	fprintf(
			stdout,
			"Shared data initialized with %d frames in buffer, %d frames in period, "
				"%d max periods, %d bytes per frame, %d buffer size and logging to %s\n",
			shared_data.frames_in_buffer, shared_data.frames_in_period,
			shared_data.max_periods, shared_data.frame_size, buffer_size, fname);

	pthread_t log_tid;
	int ret = pthread_create(&log_tid, NULL, log_thread, &shared_data);
	if (ret < 0) {
		printf("failed to create reader thread %d\n", ret);
		return ret;
	}

	pthread_t reader_tid;
	ret = pthread_create(&reader_tid, NULL, reader_thread, &shared_data);
	if (ret < 0) {
		printf("failed to create reader thread %d\n", ret);
		return ret;
	}

	// Main loop
	int voltage_div = 24;
	int time_div = 1;
	int threshold = 700;
	int thresh_enable = false;

	while (shared_data.running) {
		// Read keyboard events. Quit on backspace
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					take_screenshot();
					break;
				case SDLK_q:
					// Voltage scale up
					voltage_div *= 2;
					break;
				case SDLK_a:
					// Voltage scale down
					if (voltage_div > 1)
						voltage_div /= 2;
					break;
				case SDLK_w:
					// Threshold voltage up
					threshold += 100;
					break;
				case SDLK_s:
					// Threshold voltage down
					threshold -= 100;
					break;
				case SDLK_x:
					// No threshold (free running waveform)
					thresh_enable = !thresh_enable;
					break;
				case SDLK_e:
					// Increase time shown
					time_div *= 2;
					break;
				case SDLK_d:
					// Decrease time shown
					if (time_div > 1)
						time_div /= 2;
					break;
				case SDLK_BACKSPACE:
					// Close program
					shared_data.running = false;
					break;
				case SDLK_z:
					// print_data();
					break;
				default:
					// Ignore
					break;
				}
			}
		}

		// Render the screen
		unsigned int start = 0;

		signed short *buffer = shared_data.buffer;

		if (thresh_enable) {
			int seenlessthresh = -1;
			for (unsigned int i = 1; i < shared_data.frames_in_buffer; i++) {
				if (buffer[i * 2] < threshold && buffer[(i - 1) * 2] < threshold)
					seenlessthresh = i;
				//J: for positive thresh value, this gives rising edge
				if (buffer[i * 2] >= threshold && seenlessthresh != -1 && i
						- seenlessthresh < 10) {
					start = i;
					break;
				}
			}
		}

		// free-running waveform or we found a valid value above threshold
		if (!thresh_enable || (thresh_enable && start != 0)) {

			signed short v = buffer[0];
			signed short vlast = buffer[0];
			int origin = DIM_sh / 2;

			for (unsigned int c = 0; c < DIM_sw; c++) {
				if ((start + c * time_div) < shared_data.frames_in_buffer) {
					v = buffer[(start + c * time_div) * 2];
				} else {
					v = 0;
				}

				int disp_v = (v / voltage_div) + origin;
				int disp_vlast = (vlast / voltage_div) + origin;

				for (int r = 0; r < DIM_sh; r++) {
					// if origin, set to red
					if (r == origin) {
						((char*) screen->pixels)[DIM_sw * r + c] = 255;
					} else if (disp_vlast <= disp_v) {
						((char*) screen->pixels)[DIM_sw * r + c] = (disp_vlast <= r && r
								<= disp_v) ? 200 : 0;
					} else if (disp_vlast > disp_v) {
						((char*) screen->pixels)[DIM_sw * r + c] = (disp_v <= r && r
								<= disp_vlast) ? 200 : 0;
					}
				}
				vlast = v;
			}
		}

		// flip buffers
		SDL_UpdateRect(screen, 0, 0, DIM_sw, DIM_sh);
	}

	pthread_join(reader_tid, NULL);
	pthread_join(log_tid, NULL);
	fprintf(stdout, "Closing scope\n");
	fclose(log);

//	sem_destroy(shared_data.unwritten_periods);
	close_graphics();
	close_sound();
}
