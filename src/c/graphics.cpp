#pragma once

#include <SDL/SDL.h>
#include <SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xsp.h>

#include "graphics.h"


SDL_Surface *screen;
SDL_Color colors[256];


// XSP pixel doubling toggle
// This sets 400x240 video mode on N810
void set_doubling(unsigned char enable) {
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	SDL_GetWMInfo(&wminfo);
	if (enable == 255)
		enable = 0;
	XSPSetPixelDoubling(wminfo.info.x11.display, 0, enable);
}

int init_graphics() {
	// video system init
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL Error: %s\n", SDL_GetError());
		return -1;
	}

	// SQL_Quit will manage the exit
	atexit( SDL_Quit);

	// set video mode
	Uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN;
	if (!(screen = SDL_SetVideoMode(DIM_tw, DIM_th, BPP, flags))) {
		printf("SDL error: %s\n", SDL_GetError());
		return -1;
	}

	// hide cursor
	SDL_ShowCursor(0);

	// XSP PIXEL DOUBLING
	//set_doubling(1);

	// set up 256 color palette (we are in 8bpp mode)
	// dark blue
	for (int i = 0; i < 64; i++) {
		colors[i].r = 0;
		colors[i].g = i * 4;
		colors[i].b = 255;
	}
	// blue
	for (int i = 0; i < 64; i++) {
		colors[64 + i].r = 0;
		colors[64 + i].g = 255;
		colors[64 + i].b = (63 - i) * 4;
	}
	// yellow
	for (int i = 0; i < 64; i++) {
		colors[128 + i].r = i * 4;
		colors[128 + i].g = 255;
		colors[128 + i].b = 0;
	}
	// red
	for (int i = 0; i < 64; i++) {
		colors[192 + i].r = 255;
		colors[192 + i].g = (63 - i) * 4;
		colors[192 + i].b = 0;
	}

	SDL_SetColors(screen, colors, 0, 256);

	return 1;
}

void deinit_graphics() {
	// restore original video mode
	set_doubling(0);
}

void take_screenshot() {
	// take screenshot
	FILE *fp = fopen("screenshot.raw", "w");
	fwrite(screen->pixels, DIM_sw * DIM_sh, 1, fp);
	fclose(fp);

	fp = fopen("screenshot.pal", "w");
	for (int i = 0; i < 256; i++) {
		char color[3];
		color[0] = colors[i].r;
		color[1] = colors[i].g;
		color[2] = colors[i].b;
		fwrite(&color, sizeof(char), 3, fp);
	}
	fclose(fp);
}

