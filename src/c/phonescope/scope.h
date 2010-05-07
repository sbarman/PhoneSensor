#ifndef SCOPE
#define SCOPE

#include "sound.h"

struct alsa_shared {
	// ALSA related variables
	//AlsaSound *sound;
	AlsaDataSource *source;

	// log file
	FILE* log;

	// signals when to quit
	bool running;
};
#endif /*SCOPE*/
