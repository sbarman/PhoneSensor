#include <iostream>
#include <cmath>
#include <fftw3.h>
#include <alsa/asoundlib.h>
#include <SDL/SDL.h>

#include "sound.h"
#include "graphics.h"
#include "scope.h"

/* Sound related variables */
extern snd_pcm_t *pcm_handle;
extern signed char* data;
extern unsigned int frames;

/* Graphics related variables */
extern SDL_Surface* screen;

int VOLTAGE_DIV = 24;
int threshold = 700;
int THRESH_ENABLE = false;

int main(int argc, char** argv) {
	if (!init_graphics())
		exit(-1);
	if (!init_sound())
		exit(-1);

	/* Create log to store incoming microphone data */
	char fname[80] = "jlog";
	if (argc >= 2)
		strcpy(fname, argv[1]);
	FILE* jlog = fopen(fname, "w");

	/* Main loop */
	bool running = true;
	int iteration = 0;
	int* in = (int *) malloc(frames);
	unsigned long j = 0;

	while (running) {
		iteration++;

		/* Read keyboard events. Quit on backspace */
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					take_screenshot();
					break;
				case SDLK_q:
					/* Voltage scale up */
					VOLTAGE_DIV *= 2;
					break;
				case SDLK_a:
					/* Voltage scale down */
					if (VOLTAGE_DIV >= 2)
						VOLTAGE_DIV /= 2;
					break;
				case SDLK_w:
					/* Threshold voltage up */
					threshold += 100;
					break;
				case SDLK_s:
					/* Threshold voltage down */
					threshold -= 100;
					break;
				case SDLK_x:
					/* No threshold (free running waveform) */
					THRESH_ENABLE = !THRESH_ENABLE;
					break;
				case SDLK_e:
					/* Time scale up */
					// TODO(sbarman)
					break;
				case SDLK_d:
					/* Time scale down */
					// TODO(sbarman)
					break;
				case SDLK_BACKSPACE:
					running = false;
					break;
				case SDLK_z:
					/* Print latest info from buffer */
					print_data();
					break;
				default:
					/* Ignore */
					break;
				}
			}
		}

		/* Read audio data from mic */
		int pcmreturn;
		while ((pcmreturn = snd_pcm_readi(pcm_handle, data, frames)) < 0) {
			/* Probably a buffer overrun */
			fprintf(stdout, "Error %d: %s\n", pcmreturn, snd_strerror(pcmreturn));
			snd_pcm_prepare(pcm_handle);
		}

		/*
		 double accum1 = 0;
		 double accum2 = 0;
		 signed short maxval = 0;
		 signed short minval = 0;

		 for (snd_pcm_uframes_t i = 0; i < frames; i++) {

		 double d = ((signed short*) data)[2 * i];
		 accum1 += sqrt(d * d);

		 // just monitoring 2nd channel to make sure it's the same
		 // even with physical mic jack
		 d = ((signed short*) data)[2 * i + 1];
		 accum2 += sqrt(d * d);

		 // experimenting...
		 //when I checked on 3/4/09, the two channels had exactly the same data
		 //  CHECK AGAIN LATER ONCE USING MICROPHONE JACK RATHER THAN EXTERNAL MICROPHONE
		 //  But for now, ignoring the second channel.
		 // Update (long after the fact): I vaguely recall that the channels are in fact different
		 // when driving microphone jack.  But they are pretty similar.  I should probably figure out
		 // what's up, but it only a few percent difference, so not a priority.
		 in[i] = ((short*) data)[2 * i]; // read as non-interleaved and ignore other half

		 if (in[i] > maxval)
		 maxval = in[i];
		 if (in[i] < minval)
		 minval = in[i];

		 j++;

		 }
		 */

		for (snd_pcm_uframes_t i = 0; i < frames; i++) {
			fprintf(jlog, "%ld,%d,%d,%d,%d,\n", j, iteration, (int) i,
					((signed short*) data)[2 * i], ((signed short*) data)[2 * i + 1]);
			j++;
		}

		int start = 0;

		if (THRESH_ENABLE) {
			int seenlessthresh = -1;
			for (int i = 1; i < frames; i++) {
				if (in[i] < threshold && in[i - 1] < threshold)
					seenlessthresh = i;
				if (in[i] >= threshold && seenlessthresh != -1 && i - seenlessthresh
						< 10) //J: for positive thresh value, this gives rising edge
				{
					start = i;
					break;
				}
			}
		}

		if (!THRESH_ENABLE || // free-running waveform; don't need threshold
				(THRESH_ENABLE && start != 0 && start < frames - 50)) // found a valid value above threshold
		{
			int end = DIM_sw;
			if (frames - start < DIM_sw)
				end = frames - start; //should clear the unused columns, but that can wait


			for (int c = 1; c < end; c++) {
				int vlast = -in[c - 1 + start] / VOLTAGE_DIV; // minus sign so that positive means up
				int v = -in[c + start] / VOLTAGE_DIV; // minus sign so that positive means up
				v += DIM_sh / 2; // signed to unsigned
				vlast += DIM_sh / 2;
				for (int r = 0; r < DIM_sh; r++) {
					if (vlast <= v) {
						((char*) screen->pixels)[DIM_sw * r + c]
								= (vlast <= r && r <= v) ? 200 : 0;
					}

					if (vlast > v) {
						((char*) screen->pixels)[DIM_sw * r + c]
								= (v <= r && r <= vlast) ? 200 : 0;
					}
				}
			}
		}

		// flip buffers
		SDL_UpdateRect(screen, 0, 0, DIM_sw, DIM_sh);
	}

	fclose(jlog);
	free(in);
	deinit_graphics();
	deinit_sound();
}

void print_data() {
	double avg = 0;
	int maxval = INT_MIN;
	int minval = INT_MAX;

	for (snd_pcm_uframes_t i = 0; i < frames; i++) {
		signed short value = ((signed short*) data)[2 * i];
		avg += value;
		if (value > maxval) {
			maxval = value;
		}
		if (value < minval) {
			minval = value;
		}
	}
	avg /= frames;
	fprintf(stdout, "max:%d  min:%d avg: %f\n", maxval, minval, avg);
}
