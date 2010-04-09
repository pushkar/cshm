/*
 * CSHM
 *
 *  Created on: Mar 8, 2009
 *      Desc  : This contains the code for a Circular SHM implementation
 */

#ifndef CSHM_H_
#define CSHM_H_

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define CSHM_VERSION 1

typedef enum {
	ACH_OK = 0,
	ACH_OVERFLOW,
	ACH_INVALID_NAME,
	ACH_INVALID_BUFFER,
	ACH_BAD_SHM_FILE,
	ACH_FAILED_SYSCALL,
	ACH_STALE_FRAMES,
	ACH_MISSED_FRAME,
	ACH_TIMEOUT,
	ACH_READER_SLOW,
	ACH_READER_FAST
	} cshm_error;

void cshm_error_cstr(cshm_error err);

typedef struct {
  uint32_t total_frames;     		//< number of entries in index
  size_t frame_size; 				//< size of data frame
  uint32_t last_frame;
  uint64_t last_seq_num;
  pthread_rwlock_t lock;
} cshm_header_t, cshm_stat_t;

typedef struct {
  uint64_t seq_num; 				//< number of frame
  size_t size_written;   			//< size of frame
  size_t offset;    				//< byte offset of entry from beginning of data array
  double time;						//< last written at this time
  int valid;
} cshm_buffer_index_t, cshm_buffer_info_t;


class cshm {
private:
	int fd;
	struct stat sbuf;
	char* fd_name;
	size_t total_size;
	size_t header_size;
	size_t index_size;
	size_t frame_size;

	int total_frames;

	char* head_ptr;
	char* index_ptr;
	char* frame_ptr;
	uint32_t last_index_set;
	uint64_t last_seq_set;

	int is_open;

	cshm_header_t* shm_header;
	cshm_buffer_index_t* shm_index;

private:

	void getIndex(uint32_t index);

	void setIndex(uint32_t index);

	void getFrame(uint32_t index, void* buffer, size_t buf_size);

	void setFrame(uint32_t index, void* buffer, size_t buf_size);

	void updateHeader();

	void updateFromHeader();

public:

	/// Checks whether a channel exists or not. And if it does, returns the total number of frames and frame size. Calculate total size.
	/// \param channel_name Input channel_name to query
	/// \param _total_frame Returns total number of circular buffer frames
	/// \param _frame_size Returns size of a single frame
	static int ChannelInfo(const char* channel_name, int* _total_frame, size_t* _frame_size);

	/// Get latest index and sequence number from the CSHM header
	void getLastWrittenBufferInfo(uint32_t *_index, uint64_t *_sequence);

	/// Get all the info on the current channel
	void getChannelStats(cshm_stat_t *stats);

	/// Create an instance of the CSHM
	/// \param channel_name Input channel_name
	/// \param _total_frames Input total number of frames
	/// \param _frame_size Input maximum size of each frame
	cshm(const char* channel_name, int _total_frames, size_t _frame_size);

	/// Try to open file and map it to our pointers
	cshm_error Open();

	/// Write buffer to the CHSM
	/// \param buffer Input buffer address
	/// \param buffer_size Input size of buffer
	/// \param c_time Input current time
	cshm_error Write(void* buffer, size_t buffer_size, double c_time);

	/// Read buffer from the CSHM
	/// \param buffer Input buffer address
	/// \param buffer_size Size of input buffer after read
	/// \param index Input index to read from
	/// \param sequence Sequence of buffer is CSHM
	cshm_error Read(void* buffer, cshm_buffer_info_t* info, uint32_t index);

	/// Read latest buffer written to the CSHM
	/// \param buffer Input buffer address
	/// \param buffer_size Size of input buffer after read
	/// \param index Input Index of buffer after read
	/// \param sequence Sequence of buffer is CSHM
	cshm_error ReadLatest(void* buffer, cshm_buffer_info_t* info);

	/// Close the connection to CSHM instance
	void close_cshm();

	/// Close the connection to CSHM instance. Same as close_ach()
	~cshm(void);
};

#endif /* CSHM_H_ */
