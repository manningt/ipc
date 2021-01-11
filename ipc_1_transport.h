#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPC_FIFO_TRANSPORT_H
#define IPC_FIFO_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define FIFO_PATH_LENGTH 64

#define FIFO_ERROR_FILE_NOT_OPEN -2
#define FIFO_READ_ERROR_NO_BIPC -3
#define FIFO_READ_ERROR_UNRECOGNIZED_METHOD -4
#define FIFO_READ_ERROR_UNRECOGNIZED_RESOURCE -5

#define IPC_INIT_ERROR_MKFIFO_FAILED -1
#define IPC_INIT_ERROR_FIFO_OPEN_THREAD_FAILED -2


typedef struct ipc_class {
  int (*init)(struct ipc_class *self,  char * my_proc_name, char * other_proc_name);
  bool initialized;
  int fd_read; // file descriptor to read from the FIFO
  int fd_write;
  char fifo_fname_read[FIFO_PATH_LENGTH];
  char fifo_fname_write[FIFO_PATH_LENGTH];
  uint32_t num_write_msgs;
  uint32_t num_read_msgs;
  uint32_t num_bad_msgs;

  pthread_t open_fifo_thread;
  // zlog_category_t *module_category;
  char plog_string[80];
} ipc_class_t;

int ipc_init(ipc_class_t *ipc_id, char * from, char * to);
int ipc_msg_poll(ipc_class_t *ipc_id);

#endif

#ifdef __cplusplus
}
#endif