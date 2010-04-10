#include <iostream>
#include <cmath>
#include <fftw3.h>
#include <alsa/asoundlib.h>
#include <SDL/SDL.h>

#include "sound.h"
#include "graphics.h"


extern snd_pcm_t *pcm_handle;
extern const char* SOUND_DEVICE;
extern signed char* data;
extern snd_pcm_uframes_t periodsize;
extern unsigned int frames;

extern SDL_Surface* screen;

int VOLTAGE_DIV = 24;
int threshold = 700;
int THRESH_ENABLE = true;


int main(int argc, char** argv) {
	if (!init_graphics())
		exit(-1);
	if (!init_sound())
		exit(-1);

	char fname[80] = "jlog";
	if (argc >= 2)
		strcpy(fname, argv[1]);
	// log file for inputs
	FILE* jlog = fopen(fname, "w");

	// init fftw
	double *in, *out;
	fftw_complex *out_complex;

	//in = (double*) fftw_malloc(sizeof(double) * periodsize);
	//out = (double*) fftw_malloc(sizeof(double) * periodsize);
	//out_complex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * periodsize);

	in = (double*) fftw_malloc(sizeof(double) * frames);
	out = (double*) fftw_malloc(sizeof(double) * frames);
	out_complex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * frames);

	fftw_plan p = fftw_plan_dft_r2c_1d(frames, in, out_complex, FFTW_MEASURE);

	/* main loop */
	bool running = true;

	int iteration = 0;
	int maxval = 0;
	int minval = 0;
	double avg = 0;
	while (running) {

		// quit on backspace
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				//if (event.key.keysym.sym == SDLK_SPACE) {
				//  take_screenshot();
				//} else {
				//  running = false;
				//}
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					take_screenshot();
					break;
				case SDLK_q:
					// voltage scale up
					VOLTAGE_DIV *= 2;
					break;
				case SDLK_a:
					// voltage scale down
					if (VOLTAGE_DIV >= 2)
						VOLTAGE_DIV /= 2;
					break;

				case SDLK_w:
					// threshold voltage up
					threshold += 100;
					break;
				case SDLK_s:
					// threshold voltage down
					threshold -= 100;
					break;
				case SDLK_x:
					// no threshold (free running waveform)
					THRESH_ENABLE = !THRESH_ENABLE;
					break;

				case SDLK_e:
					// time scale up
					break;
				case SDLK_d:
					// time scale down
					break;

				case SDLK_BACKSPACE:
					running = false;
					break;
				case SDLK_z:
					maxval = INT_MIN;
					minval = INT_MAX;
					avg = 0;
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
					break;
				default:
					// ignore
					break;

				}
			}
		}

		// read audio data from mic
		int pcmreturn;
		while ((pcmreturn = snd_pcm_readi(pcm_handle, data, frames)) < 0) {
			snd_pcm_prepare( pcm_handle); // buffer overrun
		}

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

			fprintf(jlog,"%d,%d,%d,\n",iteration,(int)i,((signed short*)data)[2*i]);


			//fprintf(jlog,"%d,%d,%d (raw input other channel)\n",iteration,(int)i,(int)(((signed short*)data)[2*i+1]) );

		}

		//fprintf(stderr,"chan1avg: %f   chang2avg: %f\n",accum1/frames,accum2/frames);
		//		fprintf(stderr, "chan1avg: %f  max:%d  min:%d    chan2avg:%f\n", accum1
		//				/ frames, (int) maxval, (int) minval, accum2 / frames);

		// window function
		////for (snd_pcm_uframes_t i = 0; i < periodsize; i++) {
		//for (snd_pcm_uframes_t i = 0; i < frames; i++) {
		//  in[i] = 0.5 * (1 - cos(2 * M_PI * i / (frames - 1))) * in[i];
		//  //fprintf(jlog,"%d,%d,%f (windowed input)\n",iteration,i,in[i]);
		//}

		// perform fft
		//fftw_execute(p);

		// convert complex to double and normalize
		////for (snd_pcm_uframes_t i = 0; i < periodsize; i++) {
		//for (snd_pcm_uframes_t i = 0; i < frames; i++) {
		//  out[i] = sqrt(out_complex[i][0] * out_complex[i][0]
		//	    + out_complex[i][1] * out_complex[i][1]) / 8;
		//  fprintf(jlog,"%d,%d,%f (output/8)\n",iteration,(int)i,out[i]);
		//}

		// paint screen
		//for (int i = 0; i < DIM_sh; i++) {
		//// shift everything one pixel to the left
		//memmove((char*) screen->pixels + DIM_sw * i, (char*) screen->pixels + DIM_sw * i + 1, DIM_sw - 1);
		//// paint new column on the right
		//((char*) screen->pixels)[DIM_sw * (i + 1) - 1] = int(out[DIM_sh - i]);
		//}

		//for(int j=0; j<frames;j++)
		//  in[j]= (iteration%2) ? -5000 : 3000;


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
		//temp
		//start=0;

		if (!THRESH_ENABLE || // free-running waveform; don't need threshold
				(THRESH_ENABLE && start != 0 && start < frames - 50)) // found a valid value above threshold
		{
			int end = DIM_sw;
			if (frames - start < DIM_sw)
				end = frames - start; //should clear the unused columns, but that can wait


			//for(int c=0; c< end; c++)
			for (int c = 1; c < end; c++) {
				int vlast = -in[c - 1 + start] / VOLTAGE_DIV; // minus sign so that positive means up
				int v = -in[c + start] / VOLTAGE_DIV; // minus sign so that positive means up
				v += DIM_sh / 2; // signed to unsigned
				vlast += DIM_sh / 2;
				//for(int r=0; r< DIM_sh; r++)
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

		iteration++;
	}

	fclose(jlog);

	// deinit
	fftw_free(in);
	fftw_free(out);
	fftw_free(out_complex);

	//	deinit_graphics();
	deinit_sound();
}
