#ifndef GRAPHICS
#define GRAPHICS

#pragma once

const int DIM_sw = 800;
const int DIM_sh = 480;
const int BPP = 8;
const int DIM_tw = 800;
const int DIM_th = 480;


// XSP pixel doubling toggle
// This sets 400x240 video mode on N810
void set_doubling(unsigned char enable);

int init_graphics();
void deinit_graphics();
void take_screenshot();

#endif /*GRAPHICS*/
