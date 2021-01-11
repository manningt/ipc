#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPC_UI_MSG_HANDLER_H
#define IPC_UI_MSG_HANDLER_H

#include "interface_base_ui.h"
#include <pthread.h>

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

typedef struct ui_msg_handler_thread_args {
  char * my_name_ptr;
  char * other_name_ptr;
  b_status_t * status_ptr;
  b_control_t * control_ptr;
  b_mode_settings_t * mode_ptr;
  b_param_settings_t * params_ptr;
  pthread_t msg_handler_thread;
} ui_msg_handler_thread_args_t;

void *uiMessageHandlerThread(void *msg_handler_thread_args_struct);

#endif

#ifdef __cplusplus
}
#endif