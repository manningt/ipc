/*
   Control message handler:
    - recieves and decodes messages
    - updates base control structures for PUT operations
    - reads base control structures to provide config and operational values for GET operations
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // needed for file descriptor read/write
#include <stdlib.h> // for atoi

#include "ipc_fifo_transport.h"
#include "ipc_control.h"
#include "logging.h"

#ifdef IPC_CONTROL_TESTING
#include "ipc_test_support.h"
#endif

extern uint8_t game_state, drill_state;
#define BASE_ACTIVE ((game_state != IDLE_GS) || (drill_state != IDLE_DS))

extern bool simulation_mode;
extern bool tie_breaker_only_opt;
extern bool all_player_serve_opt;
extern bool all_boomer_serve_opt;
extern bool run_reduce_opt;
extern int8_t game_point_delay_opt;
extern bool grunts_opt;

extern uint8_t boomer_level, speed_mod;
extern int8_t height_mod;
extern uint16_t delay_mod;

extern uint32_t soft_fault;
extern uint32_t hard_fault;

extern double cam_calib_pts[NUM_CAMERAS][NUM_CAM_CALIB_POINTS][2];

// game statistics
extern bool player_serve;
extern uint8_t player_sets;
extern uint8_t boomer_sets;
extern uint8_t player_games;
extern uint8_t boomer_games;
extern uint8_t player_points;
extern uint8_t boomer_points;
extern uint8_t player_tie_points;
extern uint8_t boomer_tie_points;
extern uint64_t game_start_time;


static uint8_t ipc_control_initialized = 0;
static ipc_transport_class_t ipc_transport_desc[2] = {0};

void ipc_control_init()
{
	char * my_name = "Base";
	char * ui_name = "Ui";
	char * ctrl_name = "Ctrl";

	if (ipc_control_initialized == 0)
	{
		mode_settings.mode = GAME_MODE_E;
		ipc_control_statistics[0].num_bad_msgs = 0;
		ipc_control_statistics[0].num_read_msgs = 0;
		ipc_control_statistics[0].num_write_msgs = 0;
	}
	if (ipc_transport_init(&ipc_transport_desc[0], my_name, ctrl_name) < 0)
	{
		LOG_ERROR( "control ipc init failed!");
	}
	else
		LOG_DEBUG( "control ipc init OK.");
	ipc_control_initialized = 1;
}

int encode_status(char *out_msg_params) {
	return sprintf(out_msg_params, "%s:%d,%s:%d,%s:%d", \
	// ACTIVE_PARAM, BASE_ACTIVE, SOFT_FAULT_PARAM, soft_fault, HARD_FAULT_PARAM, hard_fault); 
	ACTIVE_PARAM, BASE_ACTIVE, SOFT_FAULT_PARAM, 0, HARD_FAULT_PARAM, 0); 
}

int encode_mode(char *out_msg_params) {
	return sprintf(out_msg_params, "%s:%d,%s:%d,%s:%d,%s:%d,%s:%d", \
		MODE_PARAM, mode_settings.mode, ID_PARAM, mode_settings.drill_workout_id, STEP_PARAM, mode_settings.drill_step, \
		TIEBREAKER_PARAM, tie_breaker_only_opt, SIM_MODE_PARAM, simulation_mode);
}

int decode_mode(char * buffer) {
	// LOG_DEBUG("decoding: %s\n", buffer);
	char * item_ptr;
	char * end_item_ptr;
	// skip header when doing token processing:
	item_ptr = strtok_r ( (buffer + HEADER_LENGTH+1),",", &end_item_ptr);
	while (item_ptr != NULL)
	{
		char * kv_end;
		char *key_ptr = strtok_r(item_ptr, ":", &kv_end);
		char *value_ptr = strtok_r(NULL, ":", &kv_end);
		// LOG_DEBUG("key: %s   Value: %s   case: %c\n", key_ptr, value_ptr, key_ptr[0]);
		if (key_ptr[0] == MODE_PARAM[0])
			mode_settings.mode = atoi(value_ptr);
		else if (key_ptr[0] == ID_PARAM[0])
			mode_settings.drill_workout_id = atoi(value_ptr);
		else if (key_ptr[0] == STEP_PARAM[0])
			mode_settings.drill_step = atoi(value_ptr);
		else if (key_ptr[0] == TIEBREAKER_PARAM[0])
			tie_breaker_only_opt = atoi(value_ptr);
		else if (key_ptr[0] == SIM_MODE_PARAM[0])
			simulation_mode = atoi(value_ptr);
		else LOG_DEBUG("Unrecognized key: %s in: %s\n", item_ptr, buffer);
		item_ptr = strtok_r (NULL, "," , &end_item_ptr);
	}
	return RESP_OK;
}
int encode_parms(char *out_msg_params) {
	int serve_mode_enum = ALTERNATE_SERVES_E;
	if (all_player_serve_opt)
		serve_mode_enum = PLAYER_ALL_SERVES_E;
	if (all_boomer_serve_opt)
		serve_mode_enum = BOOMER_ALL_SERVES_E;
	return sprintf(out_msg_params, "%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d", \
		LEVEL_PARAM, boomer_level, DELAY_PARAM, delay_mod, HEIGHT_PARAM, height_mod, SPEED_PARAM, speed_mod,\
		SERVE_MODE_PARAM, serve_mode_enum, RUN_REDUCE_PARAM, run_reduce_opt, \
		POINTS_DELAY_PARAM, game_point_delay_opt, GRUNTS_PARAM, grunts_opt);
}

int decode_parms(char * buffer) {
	// LOG_DEBUG("decoding: %s\n", buffer);
	char * item_ptr;
	char * end_item_ptr;
	int rc = RESP_OK;
	item_ptr = strtok_r ( (buffer + HEADER_LENGTH+1),",", &end_item_ptr);
	while (item_ptr != NULL)
	{
		char * kv_end;
		char *key_ptr = strtok_r(item_ptr, ":", &kv_end);
		char *value_ptr = strtok_r(NULL, ":", &kv_end);
		// LOG_DEBUG("key: %s   Value: %s   case: %c\n", key_ptr, value_ptr, key_ptr[0]);
		if (key_ptr[0] == LEVEL_PARAM[0])
			boomer_level = (uint8_t) atoi(value_ptr);
		else if (key_ptr[0] == SPEED_PARAM[0])
			speed_mod = (uint8_t) atoi(value_ptr);
		else if (key_ptr[0] == HEIGHT_PARAM[0])
			height_mod = (int8_t) atoi(value_ptr);
		else if (key_ptr[0] == DELAY_PARAM[0])
			delay_mod = (int16_t) atoi(value_ptr);
		else if (key_ptr[0] == SERVE_MODE_PARAM[0]) {
			int serve_enum = atoi(value_ptr);
			all_boomer_serve_opt = false;
			all_player_serve_opt = false;
			if (serve_enum == PLAYER_ALL_SERVES_E)
				all_player_serve_opt = true;
			if (serve_enum == BOOMER_ALL_SERVES_E)
				all_boomer_serve_opt = true;
		}
		else if (key_ptr[0] == RUN_REDUCE_PARAM[0])
			run_reduce_opt = (bool) atoi(value_ptr);
		else if (key_ptr[0] == POINTS_DELAY_PARAM[0])
			game_point_delay_opt = (uint8_t) atoi(value_ptr);			
		else if (key_ptr[0] == GRUNTS_PARAM[0])
			grunts_opt = (bool) atoi(value_ptr);			
		else
			LOG_DEBUG("Unrecognized key: %s in: %s\n", item_ptr, buffer);
		item_ptr = strtok_r (NULL, "," , &end_item_ptr);
	}
	return rc;
}

int decode_cam_calibration(char * buffer, uint8_t camera_id) {
	// LOG_DEBUG("decoding: %s\n", buffer);
	// camera_id doesn't have to be checked for bounds, since it's generated in the RSRC decode
	char * item_ptr;
	char * end_item_ptr;
	item_ptr = strtok_r ( (buffer + HEADER_LENGTH+1),",", &end_item_ptr);
	uint8_t pt_index;
	while (item_ptr != NULL)
	{
		char * kv_end;
		char *key_ptr = strtok_r(item_ptr, ":", &kv_end);
		char *value_ptr = strtok_r(NULL, ":", &kv_end);
		// LOG_DEBUG("key: %s   Value: %s\n", key_ptr, value_ptr);
		if ((key_ptr[0] < POINT_START_CHAR) || (key_ptr[0] > (POINT_START_CHAR+(NUM_CAM_CALIB_POINTS*2)-1)))
		{
			LOG_DEBUG("Unrecognized key: %s in: %s\n", item_ptr, buffer);
			return BAD_REQUEST;
		}
		pt_index = key_ptr[0] - POINT_START_CHAR; //make 'a-z' an integer
		cam_calib_pts[camera_id][pt_index/2][pt_index & 1] = atof(value_ptr);
		item_ptr = strtok_r (NULL, "," , &end_item_ptr);
	}
	return RESP_OK;
}

int encode_cam_calibration(char *out_msg_params, uint8_t camera_id) {
	char point_id = POINT_START_CHAR;
	char item[16];
	for (int i = 0; i < NUM_CAM_CALIB_POINTS*2; i++) {
		sprintf(item,"%c:%.1f", point_id, cam_calib_pts[camera_id][i/2][i & 1]);
		if (i < ((NUM_CAM_CALIB_POINTS*2)-1))
			strcat(item, ",");
		strcat(out_msg_params, item);
		// printf(" calib: %s\n", out_msg_params);
		point_id++;
		// kludge: put out as many points as possible, but stop if buffer size will be exceeded.
		if (strlen(out_msg_params) > (MAX_MESSAGE_SIZE-10))
			break;
	}
	return strlen(out_msg_params);
}

int encode_game_stats(char *out_msg_params) {
	return sprintf(out_msg_params, "%s:%llu,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d", \
	GAME_START_TIME, game_start_time, SERVER, player_serve, BOOMER_SETS_PARAM, boomer_sets, PLAYER_SETS_PARAM, player_sets, \
	BOOMER_GAMES_PARAM, boomer_games, PLAYER_GAMES_PARAM, player_games, BOOMER_POINTS_PARAM, boomer_points, \
	PLAYER_POINTS_PARAM, player_points, BOOMER_TIEPOINTS_PARAM, boomer_tie_points, PLAYER_TIEPOINTS_PARAM, player_tie_points); 
}

void ipc_control_update() {
	if (ipc_msg_poll(&ipc_transport_desc[0]))
	{
		// buffer is used for receiving messages and for concat the outbound message
		uint8_t buffer[MAX_MESSAGE_SIZE] = {0};

		int byte_count_rcvd;
		int byte_count_sent;
		//the message parameters are carried in a json-like string
		uint8_t out_msg_params[MAX_MESSAGE_SIZE] = {0};
		int out_msg_params_len = 0;
		int out_response_code = RESP_OK;
		int out_buffer_len;
		
		byte_count_rcvd = ipc_read(&ipc_transport_desc[0], buffer, sizeof(buffer));
		if (byte_count_rcvd > 0) ipc_control_statistics[0].num_read_msgs++;
		if (byte_count_rcvd < 1)
		{
		// TODO: how to handle receive errors
			LOG_DEBUG("read pipe closed.");
		}
		switch(buffer[0]) {
			case 'G':
				//handle GET
				if (!memcmp(buffer+RSRC_OFFSET, STAT_RSRC, RSRC_STRING_LENGTH)) {
					out_msg_params_len = encode_status((char *)out_msg_params);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, MODE_RSRC, RSRC_STRING_LENGTH)) {
					out_msg_params_len = encode_mode((char *)out_msg_params);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, OPTS_RSRC, RSRC_STRING_LENGTH)) {
					out_msg_params_len = encode_parms((char *)out_msg_params);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, LCAM_RSRC, RSRC_STRING_LENGTH)) {
					out_msg_params_len = encode_cam_calibration((char *)out_msg_params, LEFT_CAM);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, RCAM_RSRC, RSRC_STRING_LENGTH)) {
					out_msg_params_len = encode_cam_calibration((char *)out_msg_params, RIGHT_CAM);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, GAME_RSRC, RSRC_STRING_LENGTH)) {
					out_msg_params_len = encode_game_stats((char *)out_msg_params);
				}
				else {
					LOG_WARNING("Unrecognized GET resource: %s", buffer);
					out_response_code = BAD_REQUEST;
				}
				break;
			case 'P':
				//handle PUT
				if (!memcmp(buffer+RSRC_OFFSET, MODE_RSRC, RSRC_STRING_LENGTH)) {
					// can only change the mode config params if boomer is not active
					if (BASE_ACTIVE == 0)
						out_response_code = decode_mode((char *) buffer);
					else
						out_response_code = LOCKED;
				}
				else if (!memcmp(buffer+RSRC_OFFSET, OPTS_RSRC, RSRC_STRING_LENGTH)) {
					out_response_code = decode_parms((char *) buffer);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, LCAM_RSRC, RSRC_STRING_LENGTH)) {
					out_response_code = decode_cam_calibration((char *) buffer, LEFT_CAM);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, RCAM_RSRC, RSRC_STRING_LENGTH)) {
					out_response_code = decode_cam_calibration((char *) buffer, RIGHT_CAM);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, STRT_RSRC, RSRC_STRING_LENGTH))
				{
					if (mode_settings.mode == GAME_MODE_E) start_game();
					else if (mode_settings.mode == DRILL_MODE_E)
					{
						load_drill( (int16_t) mode_settings.drill_workout_id);
						start_drill();
					}
					else LOG_ERROR("Invalid mode on start: %d", mode_settings.mode);
				}
				else if (!memcmp(buffer+RSRC_OFFSET, STOP_RSRC, RSRC_STRING_LENGTH))
				{
					if (game_state != IDLE_GS) end_game();
					else if (drill_state != IDLE_DS) end_drill();
				}
				else {
					LOG_WARNING("Unrecognized PUT resource: %s", buffer);
					out_response_code = BAD_REQUEST;
				}

				break;
			/*
			handle responses when requests are made.
			case 'R':
				break;
			*/
			default:
				LOG_DEBUG("Unrecognized request: %s", buffer);
				out_response_code = BAD_REQUEST;
		}

		// send response
		out_buffer_len = sprintf((char *)buffer, "%s  %3d", RSP_METHOD, out_response_code);
		if (out_msg_params_len > 0)
		{
			strcat((char *) buffer, (char *)out_msg_params);
			out_buffer_len += out_msg_params_len;
		}
		byte_count_sent = ipc_write(&ipc_transport_desc[0], buffer, out_buffer_len);

		if (byte_count_sent < out_buffer_len)
		{
			sprintf(plog_string, "Send Status response failed: ipc_send_code: %d", byte_count_sent);
			LOG_ERROR( plog_string);
			// TODO: handle IPC send errors
		}
		else ipc_control_statistics->num_write_msgs++;
	}
}
