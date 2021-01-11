#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPC_1_H
#define IPC_1_H

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include "mylog.h"

typedef struct b_status {
  bool active;
  bool done;
  uint32_t soft_fault;
  uint32_t hard_fault;
  bool ack;
} b_status_t;

typedef struct b_control {
  bool start;
  bool stop;
} b_control_t;

typedef struct b_mode_settings {
  uint32_t mode;
  uint32_t drill_workout_id;
  uint32_t drill_step;
  uint32_t iterations;
} b_mode_settings_t;

#define FIFO_NAME_LENGTH 14
#define MAX_MESSAGE_SIZE 128

#define PROTOCOL_ID "BIPC"

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define RSRC_STRING_LENGTH 5
#define RSRC_OFFSET (sizeof(PROTOCOL_ID) + METHOD_STRING_LENGTH + 1)

#define FOREACH_RESOURCE(RSRC) \
        RSRC(INVALID_RESOURCE)  \
        RSRC(STATU)  \
        RSRC(START)  \
        RSRC(STOP_) \
        RSRC(MODE_) \
        RSRC(PARMS) 

typedef enum RESOURCE_ENUM {
    FOREACH_RESOURCE(GENERATE_ENUM)
} resource_t;

static const char *RESOURCE_STRING[] = {
    FOREACH_RESOURCE(GENERATE_STRING)
};

#define METHOD_STRING_LENGTH 3
#define FOREACH_METHOD(MTHD) \
        MTHD(INVALID_METHOD)  \
        MTHD(GET)  \
        MTHD(PUT)

typedef enum METHOD_ENUM {
    FOREACH_METHOD(GENERATE_ENUM)
} method_t;

static const char *METHOD_STRING[] = {
    FOREACH_METHOD(GENERATE_STRING)
};

#define BIPC_HEADER_LENGTH 5 + METHOD_STRING_LENGTH + 1 + RSRC_STRING_LENGTH + 1 //"BIPC PUT START "

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
  char fifo_fname_read[(2*FIFO_NAME_LENGTH)+12];
  char fifo_fname_write[(2*FIFO_NAME_LENGTH)+12];
  uint32_t num_write_msgs;
  uint32_t num_read_msgs;
  uint32_t num_bad_msgs;

  pthread_t open_fifo_thread;
  // zlog_category_t *module_category;
  char plog_string[80];
} ipc_class_t;

int ipc_init(ipc_class_t *ipc_id, char * from, char * to);
int ipc_msg_poll(ipc_class_t *ipc_id);

int ipc_send(ipc_class_t *this_ipc, method_t method, resource_t resource, int response_status, \
  uint8_t pbbuffer_len, uint8_t * pbbuffer);
int ipc_recv(ipc_class_t *this_ipc, method_t *method_e, resource_t *rsrc_e, int * response_int, \
  int * pbbuffer_len, uint8_t pbbuffer[]);

#endif

#ifdef __cplusplus
}
#endif