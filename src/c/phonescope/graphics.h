#ifndef GRAPHICS
#define GRAPHICS

#pragma once

const int DIM_sw = 800;
const int DIM_sh = 480;
const int BPP = 8;
const int DIM_tw = 800;
const int DIM_th = 480;

int init_graphics();
void close_graphics();
void take_screenshot();

#endif /*GRAPHICS*/
