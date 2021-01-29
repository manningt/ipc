#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include "ipc_fifo_transport.h"
#include <pthread.h>

//--start of shared defines --//

#define RSRC_STRING_LENGTH 4
#define RSRC_OFFSET 4
#define MESSAGE_BODY_OFFSET 9

#define NUM_CAM_CALIB_POINTS 7
#define POINT_START_CHAR 'a'
#define LEFT_CAMERA_ID 0
#define RIGHT_CAMERA_ID 1

#define MAX_MESSAGE_SIZE 128

#define PROTOCOL_ID ""

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,


#define FOREACH_RESOURCE(RSRC) \
			RSRC(INVALID_RESOURCE)  \
			RSRC(STAT)  \
			RSRC(STRT)  \
			RSRC(STOP) \
			RSRC(MODE) \
			RSRC(PARM) \
			RSRC(LCAM)

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
			MTHD(PUT)  \
			MTHD(RSP)

typedef enum METHOD_ENUM {
	FOREACH_METHOD(GENERATE_ENUM)
} method_t;

static const char *METHOD_STRING[] = {
	FOREACH_METHOD(GENERATE_STRING)
};

#define RESP_OK 200
#define BAD_REQUEST 400  //used if the message decode fails
#define FORBIDDEN 403
#define NOT_FOUND 404  // unknown resource
#define METHOD_NOT_ALLOWED 405
#define LOCKED 423    // used for PUT mode if the boomer_base is Active
#define UNPROCESSABLE_ENTITY 422  //used for encode errors - there was not a great error response for this

#define BIPC_HEADER_LENGTH 5 + METHOD_STRING_LENGTH + 1 + RSRC_STRING_LENGTH + 1 //"BIPC PUT START "

#define BASE_ACTIVE ((game_state != IDLE_GS) || (drill_state != IDLE_DS))

// the following is maintained by the interface - there is no boomer_base equivalent variables.
typedef struct b_mode_settings {
	uint32_t mode;
	uint32_t drill_workout_id;
	uint32_t drill_step;
} b_mode_settings_t;

typedef struct ipc_control_statistics {
	uint32_t num_write_msgs;
	uint32_t num_read_msgs;
	uint32_t num_bad_msgs;
} ipc_control_statistics_t;

// the following are placeholders for the camera calibration
typedef struct point {
	uint32_t x;
	uint32_t y;
} point_t;
typedef struct cam_calib_pts {
	point_t pt[7];
	// point_t near_base_left; // Near baseline, left singles sideline (bottom left corner of singles court)
	// point_t near_base_right; 
	// point_t near_service_left; // Near service line, left singles sideline
	// point_t near_service_center; //Near service line, center line (typically called the "T")
	// point_t near_service_right;
	// point_t far_base_left; // Far baseline, left singles sideline (Upper left corner of singles court)
	// point_t far_base_right;
} cam_calib_pts_t;
cam_calib_pts_t cam_calib_pts[2];


// the following is global so they can be read by any of the sessions (ipc channels)
b_mode_settings_t mode_settings;
ipc_control_statistics_t ipc_control_statistics[2];

void ipc_control_init();
void ipc_control_update();

#ifdef __cplusplus
}
#endif