#ifndef DATASTREAM
#define DATASTREAM

class DataStream {
public:
	virtual unsigned int get_data(short *buffer, int frames);
	virtual unsigned int block_size_in_frames();
	virtual unsigned int max_buffer_size_in_frames();
	virtual unsigned int frame_size();
	virtual bool running();
};

#endif /*DATASTREAM*/

