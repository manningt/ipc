#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include "ipc_fifo_transport.h"
#include <pthread.h>

//-=-=- start of exported defines
#define MAX_MESSAGE_SIZE 128

#define RSRC_STRING_LENGTH 4
#define RSRC_OFFSET 4
#define HEADER_LENGTH 8

#define GET_METHOD "GET"
#define PUT_METHOD "PUT"
#define RSP_METHOD "RSP"

#define STAT_RSRC "STAT"
#define STOP_RSRC "STOP"
#define STRT_RSRC "STRT"
#define PAUS_RSRC "PAUS"
#define RESU_RSRC "RESU"
#define MODE_RSRC "MODE"
#define LDSH_RSRC "LDSH"  //level, delay, speed, height - can be changed on the fly
#define LCAM_RSRC "LCAM"  //left cam
#define RCAM_RSRC "RCAM"  //right cam

// #define MODE_E_UNKNOWN 0
#define GAME_MODE_E 1
#define DRILL_MODE_E 2
#define WORKOUT_MODE_E 3

// status parameters
#define ACTIVE_PARAM "active"
#define SOFT_FAULT_PARAM "sFault"
#define HARD_FAULT_PARAM "hFault"
// mode parameters
#define MODE_PARAM "mode"
#define ID_PARAM "id" //drill or workout ID
#define STEP_PARAM "step" //drill step to execture
#define DOUBLES_PARAM "doubles" //single or double mode
#define TIEBREAKER_PARAM "tiebreaker"
#define SIM_MODE_PARAM "z_sim"  //used to set/clear simulation mode
// drill & game parameters
#define LEVEL_PARAM "level"
#define SPEED_PARAM "speed"
#define DELAY_PARAM "delay"
#define HEIGHT_PARAM "height"
// game statistics
#define BOOMER_POINTS_PARAM "1b_pts"
#define PLAYER_POINTS_PARAM "2p_pts"
#define BOOMER_GAMES_PARAM "3b_games"
#define PLAYER_GAMES_PARAM "4p_games"
#define BOOMER_SETS_PARAM "5b_sets"
#define PLAYER_SETS_PARAM "6p_sets"

#define NUM_CAM_CALIB_POINTS 7
#define POINT_START_CHAR 'a'
#define LEFT_CAMERA_ID 0
#define RIGHT_CAMERA_ID 1

#define RESP_OK 200
#define BAD_REQUEST 400  //used if the message decode fails
#define FORBIDDEN 403
#define NOT_FOUND 404  // unknown resource
#define METHOD_NOT_ALLOWED 405
#define LOCKED 423    // used for PUT mode if the boomer_base is Active
#define UNPROCESSABLE_ENTITY 422  //used for encode errors - there was not a great error response for this

//-=-=- end of exported defines

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