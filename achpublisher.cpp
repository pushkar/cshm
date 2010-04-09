#include "achshm.h"
#include "timer.h"

cshmPublisher::cshmPublisher(const char* channel_name, int _total_frames, size_t _frame_size) {
	shm = new cshm(channel_name, _total_frames, _frame_size);
}

cshm_error cshmPublisher::open() {
	return shm->Open();
}

cshm_error cshmPublisher::set(void* buffer, size_t buffer_size) {
	double c_time = Timer::getCurrentTime();
	return shm->Write(buffer, buffer_size, c_time);
}

cshmPublisher::~cshmPublisher() {
	delete shm;
}

cshmVirtualPublisher::cshmVirtualPublisher(const char* channel_name, int _total_frames, size_t _frame_size) {
	shm = new cshm(channel_name, _total_frames, _frame_size);
}

cshm_error cshmVirtualPublisher::open() {
	return shm->Open();
}

// Read time from the buffer info, instead of the current time
cshm_error cshmVirtualPublisher::set(void* buffer, cshm_buffer_info_t *info) {
	double c_time = info->time;
	return shm->Write(buffer, info->size_written, c_time);
}

cshmVirtualPublisher::~cshmVirtualPublisher() {
	delete shm;
}
