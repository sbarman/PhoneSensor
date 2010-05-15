#ifndef SCOPE
#define SCOPE

#include "sound.h"

struct alsa_shared {
	// ALSA related variables
	//AlsaSound *sound;
	AlsaDataSource *source;
	DataStream *gui_data_stream;
	// log file
	FILE* log;
	// signals when to quit
	bool running;
	float heart_rate;
};
#endif /*SCOPE*/
