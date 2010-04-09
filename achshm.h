/*
 * AchShm.h
 *
 *  	 Created on : Mar 25, 2009
 * 		Description : Contains description of a publish subscribe model using circular shared memory buffers.
 */

#ifndef ACHSHM_H_
#define ACHSHM_H_

#include "cshm.h"

/// The Publisher Class is used for publishing data to a circular shared memory buffer channel.
/// The shared memory resides until the publisher exists. It is destroyed as soon as the Publisher is closed or destroyed.
class cshmPublisher {
private:
	cshm *shm;
	double last_time_set;

public:
	/// This is used to create an instance of the publisher
	/// \param channel_name Input channel name to map. /dev/shm/channel_name.dat is mapped
	/// \param _total_frames Input total number of buffers/frames
	/// \param _frame_size Input maximum frame size
	cshmPublisher(const char* channel_name, int _total_frames, size_t _frame_size);

	/// This tries to create a link to the channel buffer
	cshm_error open();

	/// Set some data in the shared memory buffer
	/// \param buffer Input address of buffer to be written
	/// \param buffer_size Input size of buffer to be written
	cshm_error set(void* buffer, size_t buffer_size);

	// Delete this instance of the publisher. This will also destroy the file in the shared memory
	~cshmPublisher();
};

class cshmVirtualPublisher {
private:
	cshm *shm;

public:
	cshmVirtualPublisher(const char* channel_name, int _total_frames, size_t _frame_size);
	cshm_error open();
	cshm_error set(void* buffer, cshm_buffer_info_t *info);
	~cshmVirtualPublisher();
};

/// The Subscriber Class is used for subscribing data of a circular shared memory buffer channel.
class cshmSubscriber {
private:
	int is_open;
	int total_frames;
	size_t frame_size;
	cshm *shm;
	uint32_t last_index_read;
	uint64_t last_sequence_read;

	double frequency;
	double interval;
	double* last_time;

	cshm_error err;

	void (*callback)(void);
public:
	double last_time_read;

public:
	/// This is used to create an instance of the subscriber class
	/// \param channel_name Input channel name to map. /dev/shm/channel_name.dat is mapped
	cshmSubscriber(const char* channel_name);

	~cshmSubscriber();

	size_t getFrameSize();

	int getTotalFrames();

	/// This tries to create a link to the channel buffer
	cshm_error open();

	void reset();

	/// This is used to get the next buffer in the shared circular buffer memory.
	/// A counter is maintained in this class which decides the next buffer sequence to get
	/// \param buffer Input address to which buffer should be stored
	/// \param buffer_size Returns buffer_size written to buffer
	/// \param sequence Returns sequence number of the current buffer
	cshm_error getNext(void* buffer, cshm_buffer_info_t* info);

	/// This is used to get the latest stored buffer in the shared circular buffer memory
	/// \param buffer Input address to which buffer should be stored
	/// \param buffer_size Returns buffer_size written to buffer
	/// \param index Returns position of buffer in the circular buffer
	/// \param sequence Returns sequence number of the current buffer
	cshm_error getLatest(void* buffer, cshm_buffer_info_t* info);

	/// This function should be used sparingly.This will ensure real timeness of this instance of subscriber.
	/// After this function is registered, it is expected that there is a getNext or a getLatest call before every 1/frequency interval.
	/// Whenever this function is called and a function func is registered with a frequency. If a timer sees that a call is made later than 1/frequency interval it calls func.
	/// It is expected that the user will write an error handler in func to handle and out of frequency error.
	/// Needs further testing.
	/// While testing do not use usleep.
	/// \param func Input pointer to error function handler
	/// \param frequency Input frequency
	void setFrequencyErrorCallBack(void (*func)(void), int frequency);

	/// Get the latest written frame's index and sequence number
	void getLatest(uint32_t *index, uint64_t *seq) {
		shm->getLastWrittenBufferInfo(index, seq);
	}
};


#endif /* ACHSHM_H_ */
