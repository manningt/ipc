#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include "ipc_fifo_transport.h"
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

#define BAD_REQUEST 400  //used if the message decode fails
#define FORBIDDEN 403
#define NOT_FOUND 404  // unknown resource
#define METHOD_NOT_ALLOWED 405
#define LOCKED 423    // used for PUT mode if the boomer_base is Active
#define UNPROCESSABLE_ENTITY 422  //used for encode errors - there was not a great error response for this

#define BIPC_HEADER_LENGTH 5 + METHOD_STRING_LENGTH + 1 + RSRC_STRING_LENGTH + 1 //"BIPC PUT START "

typedef struct b_mode_settings {
  uint32_t mode;
  uint32_t drill_workout_id;
  uint32_t drill_step;
} b_mode_settings_t;

typedef struct ipc_control_desc {
    int (*init)(struct ipc_control_desc *self);
    bool initialized;
    ipc_transport_class_t * ipc_desc;
    b_mode_settings_t mode_settings;
} ipc_control_desc_t;

#define BASE_ACTIVE ((game_state != IDLE_GS) || (drill_state != IDLE_DS))

void ipc_control_init( ipc_control_desc_t *descriptor);

void ipc_control_update(ipc_control_desc_t *descriptor);


#ifdef __cplusplus
}
#endif