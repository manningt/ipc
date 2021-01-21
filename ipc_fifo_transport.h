#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define FIFO_NAME_LENGTH 14
#define FIFO_PATH_LENGTH 64

#define FIFO_ERROR_FILE_NOT_OPEN -2
#define FIFO_READ_ERROR_NO_BIPC -3
#define FIFO_READ_ERROR_UNRECOGNIZED_METHOD -4
#define FIFO_READ_ERROR_UNRECOGNIZED_RESOURCE -5

#define IPC_INIT_ERROR_MKFIFO_FAILED -1
#define IPC_INIT_ERROR_FIFO_OPEN_THREAD_FAILED -2

#ifdef linux
#define FIFO_PATH_PREFIX "/dev/shm"
#else
#define FIFO_PATH_PREFIX "/tmp"
#endif


typedef struct ipc_transport_class {
	int (*init)(struct ipc_transport_class *self,  char * my_proc_name, char * other_proc_name);
	bool initialized;
	int fd_read; // file descriptor to read from the FIFO
	int fd_write;
	char fifo_fname_read[FIFO_PATH_LENGTH];
	char fifo_fname_write[FIFO_PATH_LENGTH];
	uint32_t num_write_msgs;
	uint32_t num_read_msgs;
	uint32_t num_bad_msgs;

	pthread_t open_fifo_thread;
	char plog_string[80];
} ipc_transport_class_t;

int ipc_init(ipc_transport_class_t *ipc_id, char * from, char * to);
int ipc_msg_poll(ipc_transport_class_t *ipc_id);

#ifdef __cplusplus
}
#endif