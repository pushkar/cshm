#include "cshm.h"
#include <pthread.h>

void cshm::getIndex(uint32_t index) {
	memcpy(shm_index, index_ptr + index * index_size, index_size);
}

void cshm::setIndex(uint32_t index) {
	memcpy(index_ptr + index * index_size, shm_index, index_size);
}

void cshm::getFrame(uint32_t index, void* buffer, size_t buf_size) {
	memcpy(buffer, frame_ptr + index * frame_size, buf_size);
}

void cshm::setFrame(uint32_t index, void* buffer, size_t buf_size) {
	memcpy(frame_ptr + index * frame_size, buffer, buf_size);
}

void cshm::updateHeader() {
	memcpy(head_ptr, shm_header, header_size);
}

void cshm::updateFromHeader() {
	memcpy(shm_header, head_ptr, header_size);
}

cshm::cshm(const char* channel_name, int _total_frames, size_t _frame_size) :
frame_size(_frame_size),
total_frames(_total_frames)
{
	fd = -1;
	fd_name = new char[100];
	sprintf(fd_name, "/dev/shm/%s.dat", channel_name);
	stat(fd_name, &sbuf);

	// Initialize size
	header_size = sizeof(cshm_header_t);
	index_size = sizeof(cshm_buffer_index_t);
	total_size = header_size + total_frames * (index_size + frame_size);

	last_index_set = 0;
	last_seq_set = 0;
	is_open = 0;

	// Initialize header and index
	shm_header = new cshm_header_t;
	shm_index = new cshm_buffer_index_t;

	shm_header->frame_size = frame_size;
	shm_header->total_frames = total_frames;
	shm_header->last_frame = last_index_set;
	shm_header->last_seq_num = last_seq_set;

	head_ptr = NULL;
	index_ptr = NULL;
	frame_ptr = NULL;

	pthread_rwlock_init(&shm_header->lock, NULL);
}

cshm_error cshm::Open() {
	if(is_open) return ACH_OK;

	if((fd = open(fd_name, O_CREAT | O_RDWR, S_IRWXO)) == -1 ) {
		fprintf(stderr, "Failed to open file %s\n", fd_name);
		perror("Open File Error");
		return ACH_BAD_SHM_FILE;
	}

	struct stat file_stat;
	fstat(fd, &file_stat);
	// printf("Number of hard links: %d\n", (int) file_stat.st_nlink);

	ftruncate(fd, total_size);

	if ((head_ptr = (char*) mmap((caddr_t)0, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		fprintf(stderr, "Failed to map file %s\n", fd_name);
		perror("Mapping Error");
		return ACH_FAILED_SYSCALL;
	}

	// Set pointers
	index_ptr = head_ptr + header_size;
	frame_ptr = index_ptr + total_frames * index_size;

	// printf("Opened file at %p. Index: %p, Frame: %p\n", head_ptr, index_ptr, frame_ptr);

	pthread_rwlock_wrlock(&shm_header->lock);
	updateHeader();
	pthread_rwlock_unlock(&shm_header->lock);

	is_open = 1;
	return ACH_OK;
}

cshm_error cshm::Write(void* buffer, size_t buffer_size, double c_time) {
	pthread_rwlock_wrlock(&shm_header->lock);
	updateHeader();

	last_index_set = shm_header->last_frame;
	last_seq_set = shm_header->last_seq_num;

	if(buffer_size > frame_size) {
		pthread_rwlock_unlock(&shm_header->lock);
		return ACH_OVERFLOW;
	}

	shm_index->seq_num = last_seq_set;
	shm_index->size_written = buffer_size;
	shm_index->offset = last_index_set * frame_size;
	shm_index->time = c_time;
	shm_index->valid = 1;

	setIndex(last_index_set);
	setFrame(last_index_set, buffer, buffer_size);

	last_seq_set ++;
	last_index_set = (last_seq_set) % total_frames;

	shm_header->last_frame = last_index_set;
	shm_header->last_seq_num = last_seq_set;

	updateHeader();
	pthread_rwlock_unlock(&shm_header->lock);

	return ACH_OK;
}

cshm_error cshm::Read(void* buffer, cshm_buffer_info_t* info, uint32_t index) {
	// For safety, should be,
	// index = index % total_frames
	pthread_rwlock_rdlock(&shm_header->lock);

	getIndex(index);
	getFrame(index, buffer, shm_index->size_written);
	pthread_rwlock_unlock(&shm_header->lock);

	*info = *shm_index;
	if(shm_index->valid == 0) return ACH_INVALID_BUFFER;
	return ACH_OK;
}

cshm_error cshm::ReadLatest(void* buffer, cshm_buffer_info_t* info) {
	pthread_rwlock_rdlock(&shm_header->lock);
	updateFromHeader();
	int index = shm_header->last_frame - 1;
	if(index < 0) index = 0;
	//fprintf(stderr, "CSHM: Fetching %d\n", index);
	getIndex(index);
	getFrame(index, buffer, shm_index->size_written);
	pthread_rwlock_unlock(&shm_header->lock);

	*info = *shm_index;
	if(shm_index->valid == 0) return ACH_INVALID_BUFFER;
	return ACH_OK;
}

void cshm::close_cshm() {
	if(shm_header) delete shm_header;
	if(shm_index) delete shm_index;
	if(fd_name) delete fd_name;
	if(fd > 0) close(fd);
	pthread_rwlock_destroy(&shm_header->lock);
}

cshm::~cshm(void) {
	close_cshm();
}

int cshm::ChannelInfo(const char* channel_name, int *_total_frame, size_t *_frame_size) {
	int fd = -1;
	char* fd_name = new char[100];
	sprintf(fd_name, "/dev/shm/%s.dat", channel_name);

	cshm_header_t* new_shm_header = new cshm_header_t;
	size_t header_size = sizeof(cshm_header_t);
	char* head;

	if((fd = open(fd_name, O_RDWR)) == -1 ) {
		perror("Failed to open file");
		return 0;
	}

	if ((head = (char*) mmap((caddr_t)0, header_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		perror("Failed to map file");
		return 0;
	}

	memcpy(new_shm_header, head, header_size);
	*_total_frame = new_shm_header->total_frames;
	*_frame_size = new_shm_header->frame_size;

	return 1;
}

void cshm::getLastWrittenBufferInfo(uint32_t *_index, uint64_t *_sequence) {
	pthread_rwlock_rdlock(&shm_header->lock);
	updateFromHeader();
	*_index = shm_header->last_frame;
	*_sequence = shm_header->last_seq_num;
	pthread_rwlock_unlock(&shm_header->lock);
}

void cshm::getChannelStats(cshm_stat_t *stats) {
	pthread_rwlock_rdlock(&shm_header->lock);
	updateFromHeader();
	*stats = *shm_header;
	pthread_rwlock_unlock(&shm_header->lock);
}

void cshm_error_cstr(cshm_error err) {
	switch(err) {
	case ACH_OK: printf("Ok\n"); break;
	case ACH_OVERFLOW: printf("Error: Overflow\n"); break;
	case ACH_INVALID_NAME: printf("Error: Invalid Name\n"); break;
	case ACH_INVALID_BUFFER: printf("Error: Invalid Buffer\n"); break;
	case ACH_BAD_SHM_FILE: printf("Error: Bad File Name\n"); break;
	case ACH_FAILED_SYSCALL: printf("Error: Bad System Call\n"); break;
	case ACH_STALE_FRAMES: printf("Error: Old/Stale Frames\n"); break;
	case ACH_MISSED_FRAME: printf("Error: Missed Frames\n"); break;
	case ACH_TIMEOUT: printf("Error: Timeout\n"); break;
	case ACH_READER_SLOW: printf("Error: Reader too slow\n"); break;
	case ACH_READER_FAST: printf("Error: Reader very fast\n"); break;
	default: printf("Error: None\n"); break;
	}
}
