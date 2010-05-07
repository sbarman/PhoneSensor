#ifndef DATASTREAM
#define DATASTREAM

class DataStream {
public:
	unsigned int get_data(short *buffer, int frames);
	unsigned int block_size_in_frames();
	unsigned int max_buffer_size_in_frames();
	bool running();
};

#endif /*DATASTREAM*/

