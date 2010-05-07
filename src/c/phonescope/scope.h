#ifndef SCOPE
#define SCOPE

#include "sound.h"

class PhoneScopeGui;

struct alsa_shared {
	// ALSA related variables
	//AlsaSound *sound;
	AlsaDataSource *source;

	char *buffer;
	unsigned int frames_in_buffer;
	unsigned int writer_position;

	// log file
	FILE* log;

	// signals when to quit
	bool running;

	PhoneScopeGui* gui;
};
#endif /*SCOPE*/
