#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPC_1_MSG_HANDLER_H
#define IPC_1_MSG_HANDLER_H

#include "interface_base_ui.h"
#include <pthread.h>

typedef struct msg_handler_thread_args {
  char * my_name_ptr;
  char * other_name_ptr;
  b_status_t * status_ptr;
  b_control_t * control_ptr;
  b_mode_settings_t * mode_ptr;
  b_param_settings_t * params_ptr;
  pthread_t msg_handler_thread;
} msg_handler_thread_args_t;

#define FIFO_NAME_LENGTH 14

void *messageHandlerThread(void *msg_handler_thread_args_struct);

#endif

#ifdef __cplusplus
}
#endif