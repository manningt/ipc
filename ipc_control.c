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
#include "faults.h"

#ifdef IPC_CONTROL_TESTING
#include "globals_for_test.h"
#endif

uint8_t ipc_control_initialized = 0;

ipc_transport_class_t ipc_transport_desc[2] = {0};

char * my_name = "Base";
char * other_name = "Ctrl";

char plog_string[256];

uint8_t out_message[MAX_MESSAGE_SIZE] ;
uint8_t out_message_len;
int out_response_int;
int out_buffer_len;

uint8_t buffer[MAX_MESSAGE_SIZE] = {0};
int byte_count_rcvd;
int byte_count_sent;

void ipc_control_init()
{
	if (ipc_control_initialized == 0)
	{
		mode_settings.mode = GAME_MODE_E;
		ipc_control_statistics[0].num_bad_msgs = 0;
		ipc_control_statistics[0].num_read_msgs = 0;
		ipc_control_statistics[0].num_write_msgs = 0;
	}
	if (ipc_transport_init(&ipc_transport_desc[0], my_name, other_name) < 0)
	{
		LOG_ERROR( "control ipc init failed!");
	}
	else
		LOG_DEBUG( "control ipc init OK.");
	ipc_control_initialized = 1;
}

int encode_status() {
	return sprintf((char *)out_message, "active:%d,softFlt:%d,hardFlt:%d", BASE_ACTIVE, soft_fault, hard_fault); 
}

int encode_mode() {
	return sprintf((char *)out_message, "mode:%d,id:%d,step:%d,doubles:%d,tiebreaker:%d", \
		mode_settings.mode, mode_settings.drill_workout_id, mode_settings.drill_step, \
		doubles_mode, FAKE_tiebreak_mode );
}

int decode_mode() {
	// LOG_DEBUG("decoding: %s\n", buffer);
	char * item_ptr;
	char * end_item_ptr;
	char * buff_ptr = (char * ) buffer;
	buff_ptr += HEADER_LENGTH+1;  //skip over header
	item_ptr = strtok_r ( buff_ptr,",", &end_item_ptr);
	while (item_ptr != NULL)
	{
		char * kv_end;
		char *key_ptr = strtok_r(item_ptr, ":", &kv_end);
		char *value_ptr = strtok_r(NULL, ":", &kv_end);
		// LOG_DEBUG("key: %s   Value: %s   case: %c\n", key_ptr, value_ptr, key_ptr[0]);
		switch(key_ptr[0]) {
			case 'm':
				mode_settings.mode = atoi(value_ptr);
				//LOG_DEBUG("mode = %d\n", mode_settings.mode);
				break;
			case 'i':
				mode_settings.drill_workout_id = atoi(value_ptr);
				//LOG_DEBUG("drill id = %d\n", mode_settings.drill_workout_id );
				break;
			case 's':
				mode_settings.drill_step = atoi(value_ptr);
				//LOG_DEBUG("step id = %d\n",mode_settings.drill_step);
				break;
			case 'd':
				doubles_mode = atoi(value_ptr);
				//LOG_DEBUG("doubles = %d\n", doubles_mode);
				break;
			case 't':
				FAKE_tiebreak_mode = atoi(value_ptr);
				//LOG_DEBUG("tiebreaker = %d\n", FAKE_tiebreak_mode);
				break;
			default:
			LOG_DEBUG("Unrecognized key: %s in: %s\n", item_ptr, buffer);
			break;
		}
		item_ptr = strtok_r (NULL, "," , &end_item_ptr);
	}
	return RESP_OK;
}
int encode_parms() {
	return sprintf((char *)out_message, "level:%d,delay:%d,height:%d,speed:%d", \
		boomer_level, delay_mod, height_mod, speed_mod);
}

int decode_parms() {
	// LOG_DEBUG("decoding: %s\n", buffer);
	char * item_ptr;
	char * end_item_ptr;
	char * buff_ptr = (char * ) buffer;
	buff_ptr += HEADER_LENGTH+1;  //skip over header
	item_ptr = strtok_r ( buff_ptr,",", &end_item_ptr);
	while (item_ptr != NULL)
	{
		char * kv_end;
		char *key_ptr = strtok_r(item_ptr, ":", &kv_end);
		char *value_ptr = strtok_r(NULL, ":", &kv_end);
		// LOG_DEBUG("key: %s   Value: %s   case: %c\n", key_ptr, value_ptr, key_ptr[0]);
		switch(key_ptr[0]) {
			case 'l':
				boomer_level = atoi(value_ptr);
				// LOG_DEBUG("level = %d\n",boomer_level);
				break;
			case 's':
				speed_mod = (uint8_t) atoi(value_ptr);
				// LOG_DEBUG("speed = %d\n", speed_mod );
				break;
			case 'h':
				height_mod = (int8_t) atoi(value_ptr);
				// LOG_DEBUG("height = %d\n",height_mod);
				break;
			case 'd':
				delay_mod = (int16_t) atoi(value_ptr);
				// LOG_DEBUG("delay = %d\n", delay_mod);
				break;
			default:
			LOG_DEBUG("Unrecognized key: %s in: %s\n", item_ptr, buffer);
			break;
		}
		item_ptr = strtok_r (NULL, "," , &end_item_ptr);
	}
	return RESP_OK;
}

int decode_cam_calibration(uint8_t camera_id) {
	// LOG_DEBUG("decoding: %s\n", buffer);
	// camera_id doesn't have to be checked for bounds, since it's generated in the RSRC decode
	char * item_ptr;
	char * end_item_ptr;
	char * buff_ptr = (char * ) buffer;
	buff_ptr += HEADER_LENGTH+1;  //skip over header
	item_ptr = strtok_r ( buff_ptr,",", &end_item_ptr);
	uint8_t pt_index;
	bool is_y_point;
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
		is_y_point = pt_index & 1; //even index are X, odds are Y in the point structure
		pt_index = pt_index/2;
		if (is_y_point)
			cam_calib_pts[camera_id].pt[pt_index].y = atoi(value_ptr);
		else
			cam_calib_pts[camera_id].pt[pt_index].x = atoi(value_ptr);
		item_ptr = strtok_r (NULL, "," , &end_item_ptr);
	}
	return RESP_OK;
}

int encode_cam_calibration(uint8_t camera_id) {
	char point_id = POINT_START_CHAR;
	bool is_y_point;
	char item[16];
	memset(out_message,0,4); //clear the beginning out message so strcat works
	for (int i = 0; i < NUM_CAM_CALIB_POINTS*2; i++) {
		is_y_point = i & 1; //even indexes are X, odds are Y in the point structure
		if (is_y_point)
			sprintf(item,"%c:%d", point_id, cam_calib_pts[camera_id].pt[i/2].y);
		else
			sprintf(item,"%c:%d", point_id, cam_calib_pts[camera_id].pt[i/2].x);
		if (i < ((NUM_CAM_CALIB_POINTS*2)-1))
			strcat(item, ",");
		strcat((char *)out_message, item);
		point_id++;
	}
	return strlen((char *)out_message);
}


void ipc_control_update() {
	if (ipc_msg_poll(&ipc_transport_desc[0]))
	{
		// out_pbbuffer_len = 0;
		out_message_len = 0;
		out_response_int = RESP_OK;
		memset(buffer,0,sizeof(buffer)); 
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
					out_message_len = encode_status();
				}
				else if (!memcmp(buffer+RSRC_OFFSET, MODE_RSRC, RSRC_STRING_LENGTH)) {
					out_message_len = encode_mode();
				}
				else if (!memcmp(buffer+RSRC_OFFSET, LDSH_RSRC, RSRC_STRING_LENGTH)) {
					out_message_len = encode_parms();
				}
				else if (!memcmp(buffer+RSRC_OFFSET, LCAM_RSRC, RSRC_STRING_LENGTH)) {
					out_message_len = encode_cam_calibration(LEFT_CAMERA_ID);
				}
				break;
			case 'P':
				//handle PUT
				if (!memcmp(buffer+RSRC_OFFSET, MODE_RSRC, RSRC_STRING_LENGTH)) {
					// can only change the mode config params if boomer is not active
					if (BASE_ACTIVE == 0)
						out_response_int = decode_mode();
					else
						out_response_int = LOCKED;
				}
				else if (!memcmp(buffer+RSRC_OFFSET, LDSH_RSRC, RSRC_STRING_LENGTH)) {
					out_response_int = decode_parms();
				}
				else if (!memcmp(buffer+RSRC_OFFSET, LCAM_RSRC, RSRC_STRING_LENGTH)) {
					out_response_int = decode_cam_calibration(LEFT_CAMERA_ID);
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
				break;
			/*
			handle responses when requests are made.
			case 'R':
				break;
			*/
			default:
				LOG_DEBUG("Unrecognized request: %s", buffer);
		}

		// send response
		out_buffer_len = sprintf((char *)buffer, "%s  %3d", RSP_METHOD, out_response_int);
		if (out_message_len > 0)
		{
			strcat((char *) buffer, (char *)out_message);
			out_buffer_len += out_message_len;
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
