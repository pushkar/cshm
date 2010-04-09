#include "achshm.h"
#include "timer.h"

cshmSubscriber::cshmSubscriber(const char* channel_name) {
	last_index_read = 0;
	last_sequence_read = 0;
	if(cshm::ChannelInfo(channel_name, &total_frames, &frame_size)) {
		shm = new cshm(channel_name, total_frames, frame_size);
		is_open = 1;
		return;
	}

	is_open = 0;
}

size_t cshmSubscriber::getFrameSize() {
	return frame_size;
}

int cshmSubscriber::getTotalFrames() {
	return total_frames;
}

cshm_error cshmSubscriber::open() {
	if(!is_open) return ACH_BAD_SHM_FILE;
	err = shm->Open();

	if(err != ACH_OK)
		return err;
	shm->getLastWrittenBufferInfo(&last_index_read, &last_sequence_read);
	// Bug.. last_index_read should be something different
	return err;
}

void cshmSubscriber::reset() {
	last_index_read = 0;
	last_sequence_read = 0;
}

cshm_error cshmSubscriber::getNext(void* buffer, cshm_buffer_info_t* info) {
	if(!is_open) return ACH_BAD_SHM_FILE;
	err = shm->Read(buffer, info, last_index_read);
	if(err != ACH_OK) return err;

	// Deprecated, For future release
	// Check if we are slow/fast
	// May be use getLatest and reinitialize last_index_read and last_sequence_read
	// if(info->seq_num < last_sequence_read - total_frames) err = ACH_READER_SLOW;
	// if(info->seq_num == last_sequence_read) err = ACH_READER_FAST;

	last_sequence_read++;
	last_index_read = last_sequence_read % total_frames;
	last_time_read = Timer::getCurrentTime();
	return err;
}

cshm_error cshmSubscriber::getLatest(void* buffer, cshm_buffer_info_t *info) {
	if(!is_open) return ACH_BAD_SHM_FILE;
	last_time_read = Timer::getCurrentTime();
	return shm->ReadLatest(buffer, info);
}

void cshmSubscriber::setFrequencyErrorCallBack(void (*func)(void), int _frequency) {
	callback = func;
	last_time = &last_time_read;
	frequency = 1/_frequency;
	interval = _frequency;
	Timer::startTimer();
	Timer::subscribeToTimer(callback, interval, last_time);
}

cshmSubscriber::~cshmSubscriber() {
	delete shm;
}
