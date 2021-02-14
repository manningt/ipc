#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include "ipc_fifo_transport.h"
#include <pthread.h>

//-=-=- start of exported defines
#define MAX_MESSAGE_SIZE 252

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
#define OPTS_RSRC "OPTS"  //drill & game options: level, delay, speed, height can be changed on the fly
#define LCAM_RSRC "LCAM"  //left cam
#define RCAM_RSRC "RCAM"  //right cam
#define SCOR_RSRC "SCOR"  //player scores
#define IPCS_RSRC "IPCS"  //game IPC statistics

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
// drill & game options 
#define LEVEL_PARAM "level"
#define SPEED_PARAM "speed"
#define DELAY_PARAM "delay"
#define HEIGHT_PARAM "height"
#define GRUNTS_PARAM "grunts"			// when enabled, Boomer grunts almost every time it throws a ball, in game or drill or workout mode.
        // It doesnt grunt in Game mode if it is just tossing a ball for the player to serve.
				// If grunt is enabled, it grunts louder if it throws the ball faster.
// game options
#define POINTS_DELAY_PARAM "ptDelay"	 	//increase/decrease time between points in seconds
#define RUN_REDUCE_PARAM "reduceRun"		//reduce running
#define SERVE_MODE_PARAM "wServes"			//server for game: No Serves, All Serves, Alternative Serves
#define BOOMER_ALL_SERVES_E 2
#define PLAYER_ALL_SERVES_E 1
#define ALTERNATE_SERVES_E 0
// game statistics (read-only - never decoded)
#define GAME_START_TIME "time"
#define SERVER "server"
#define BOOMER_SETS_PARAM "b_sets"
#define PLAYER_SETS_PARAM "p_sets"
#define BOOMER_GAMES_PARAM "b_games"
#define PLAYER_GAMES_PARAM "p_games"
#define BOOMER_POINTS_PARAM "b_pts"
#define PLAYER_POINTS_PARAM "p_pts"
#define BOOMER_TIEPOINTS_PARAM "b_t_pts"
#define PLAYER_TIEPOINTS_PARAM "p_t_pts"
// IPC statistics:
#define IPC_0_NUM_READS_PARAM "0_rd"
#define IPC_0_NUM_WRITES_PARAM "0_wr"
#define IPC_0_NUM_BAD_PARAM "0_bad"
#define IPC_1_NUM_READS_PARAM "1_rd"
#define IPC_1_NUM_WRITES_PARAM "1_wr"
#define IPC_1_NUM_BAD_PARAM "1_bad"


#define NUM_CAM_CALIB_POINTS 13
#define POINT_START_CHAR 'a'
#define NUM_CAMERAS 2
#define LEFT_CAM 0
#define RIGHT_CAM 1

#define RESP_OK 200
#define BAD_REQUEST 400  //used if the message decode fails
#define FORBIDDEN 403
#define NOT_FOUND 404  // unknown resource
#define METHOD_NOT_ALLOWED 405
#define LOCKED 423    // used for PUT mode if the boomer_base is Active
#define UNPROCESSABLE_ENTITY 422  //used for encode errors - there was not a great error response for this

#define CTRL_TRANSPRT 0
#define UI_TRANSPRT 1
#define BASE_NAME "Base"
#define CTRL_NAME "Ctrl"
#define UI_NAME "Ui"

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


#define NEAR_BASE_LEFT 0			// Near baseline, left singles sideline (bottom left corner of singles court)
#define NEAR_BASE_RIGHT 1 
#define NEAR_SERVICE_LEFT 2		// Near service line, left singles sideline
#define NEAR_SERVICE_CENTER 3		//Near service line, center line (typically called the "T")
#define NEAR_SERVICE_RIGHT 4
#define FAR_BASE_LEFT 5				// Far baseline, left singles sideline (Upper left corner of singles court)
#define FAR_BASE_RIGHT 6
//===
#define NET_LEFT     7
#define NET_CENTER    8
#define NET_RIGHT    9
#define FAR_SERVICE_LEFT     10
#define FAR_SERVICE_CENTER    11
#define FAR_SERVICE_RIGHT    12

void ipc_control_init();
void ipc_control_update();

#ifdef __cplusplus
}
#endif