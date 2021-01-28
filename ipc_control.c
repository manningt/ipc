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

#include "ipc_fifo_transport.h"
#include "ipc_control.h"
#include "logging.h"
#include "faults.h"

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h"

#ifdef IPC_CONTROL_TESTING
#include "globals_for_test.h"
#endif

uint8_t ipc_control_initialized = 0;
b_mode_settings_t mode_settings;

ipc_transport_class_t ipc_transport_desc[2] = {0};

char * my_name = "Base";
char * other_name = "Ctrl";

char plog_string[256];

uint8_t pbbuffer[MAX_MESSAGE_SIZE] = {0};
int in_pbbuffer_len, out_pbbuffer_len;

int out_response_int;

uint8_t buffer[MAX_MESSAGE_SIZE] = {0};
int out_buffer_len;
int byte_count_rcvd = 0;
int byte_count_sent = 0;

void ipc_control_init()
{
	if (ipc_control_initialized == 0)
	{
		mode_settings.mode = mode_e_GAME;
	}
	if (ipc_transport_init(&ipc_transport_desc[0], my_name, other_name) < 0)
	{
		LOG_ERROR( "control ipc init failed!");
	}
	else
		LOG_DEBUG( "control ipc init OK.");

	ipc_control_statistics[0].num_bad_msgs = 0;
	ipc_control_statistics[0].num_read_msgs = 0;
	ipc_control_statistics[0].num_write_msgs = 0;
	ipc_control_initialized = 1;
}

int encode_status(uint8_t * buffer) {
	return sprintf((char *)buffer, "active:%d,softFlt:%d,hardFlt:%d", BASE_ACTIVE, soft_fault, hard_fault); 
}

void ipc_control_update() {
	if (ipc_msg_poll(&ipc_transport_desc[0]))
	{
		out_pbbuffer_len = 0;
		out_response_int = 200;
		
		byte_count_rcvd = ipc_read(&ipc_transport_desc[0], buffer, sizeof(buffer));

		if (byte_count_rcvd > 0) ipc_control_statistics[0].num_read_msgs++;

		if (byte_count_rcvd < 1)
		{
		// TODO: how to handle receive errors
			LOG_DEBUG("read pipe closed.");
		}
		else if (memcmp(buffer, PROTOCOL_ID, sizeof(PROTOCOL_ID)-1) != 0)
      {
        LOG_ERROR( "BIPC not in header.");
        out_response_int = BAD_REQUEST;
        ipc_control_statistics[0].num_bad_msgs++;
      }
      else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[GET], METHOD_STRING_LENGTH))
      {
			// Process GETs
			if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STAT], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("GET STATU");
				b_status_msg msg_status = b_status_msg_init_zero;
				msg_status.active = BASE_ACTIVE;
				msg_status.soft_fault = soft_fault;
				msg_status.hard_fault = hard_fault;
				pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
				if (!pb_encode(&stream, b_status_msg_fields, &msg_status))
				{
					sprintf(plog_string, "Encoding failed on STATUS message: %s\n", PB_GET_ERROR(&stream));
					LOG_ERROR( plog_string);
					out_response_int = UNPROCESSABLE_ENTITY;
					// TODO:  handle encode errors
				}
				out_pbbuffer_len = stream.bytes_written;
			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("GET MODE");
				b_mode_msg msg_mode = b_mode_msg_init_zero;
				msg_mode.mode = mode_settings.mode;
				msg_mode.drill_workout_id = mode_settings.drill_workout_id;
				msg_mode.drill_step = mode_settings.drill_step;
				msg_mode.doubles = doubles_mode;
				// msg_mode.tie_breaker = tiebreak_mode;
				msg_mode.tie_breaker = mode_settings.temporary_tie_breaker_mode;
				// msg_mode.iterations = mode_settings.iterations;
				pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
				if (!pb_encode(&stream, b_mode_msg_fields, &msg_mode))
				{
					sprintf(plog_string, "Encoding failed on MODE message: %s\n", PB_GET_ERROR(&stream));
					LOG_ERROR( plog_string);
					// TODO: handle encode errors
					out_response_int = UNPROCESSABLE_ENTITY;
				}
				out_pbbuffer_len = stream.bytes_written;
			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARM], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("GET PARMS");
				b_params_msg params = b_params_msg_init_zero;
				if (mode_settings.mode == mode_e_GAME)
				{
					params.level = boomer_level;
				}
				else
				{
					params.level = level_mod;
				}
				params.speed = speed_mod;
				params.height = height_mod;
				params.delay = frequency_mod;
				pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
				if (!pb_encode(&stream, b_params_msg_fields, &params))
				{
					sprintf(plog_string, "Encoding failed on PARMS message: %s\n", PB_GET_ERROR(&stream));
					LOG_ERROR( plog_string);
					// TODO: handle encode errors
					out_response_int = 400;
				}
				out_pbbuffer_len = stream.bytes_written;
			}
			else
			{
				sprintf(plog_string, "Unrecognized resource for GET: %s", buffer);
				LOG_ERROR( plog_string);
				ipc_control_statistics[0].num_bad_msgs++;
			}
		} else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[PUT], METHOD_STRING_LENGTH))
		{
			// Process PUT messages
			if (byte_count_rcvd > BIPC_HEADER_LENGTH)
			{
				//copy out message portion for decoding
				in_pbbuffer_len = byte_count_rcvd - 14; // why does the define not work?? BIPC_HEADER_LENGTH;
				memcpy(pbbuffer, buffer+BIPC_HEADER_LENGTH, in_pbbuffer_len);
			}
			else in_pbbuffer_len = 0;

			if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STRT], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT START");
				if (mode_settings.mode == mode_e_GAME) start_game();
				else if (mode_settings.mode == mode_e_DRILL)
				{
					load_drill( (int16_t) mode_settings.drill_workout_id);
					start_drill();
				}
				else LOG_ERROR("Invalid mode on start: %d", mode_settings.mode);
			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STOP], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT STOP");
				if (game_state != IDLE_GS) end_game();
				else if (drill_state != IDLE_DS) end_drill();

			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT MODE");
				if (BASE_ACTIVE == 0)
				{
					// can only change the mode config params if boomer is not active
					if (in_pbbuffer_len < 1)
					{
						sprintf(plog_string, "No message component for: %s", buffer);
						LOG_ERROR( plog_string);
						ipc_control_statistics[0].num_bad_msgs++;
						out_response_int = BAD_REQUEST;
					}
					else
					{
						b_mode_msg in_message = b_mode_msg_init_zero;
						pb_istream_t stream = pb_istream_from_buffer(pbbuffer, in_pbbuffer_len);
						if (!pb_decode(&stream, b_mode_msg_fields, &in_message))
						{
							sprintf(plog_string, "Decode of PUT MODE parameters failed: %s", PB_GET_ERROR(&stream));
							LOG_ERROR( plog_string);
							// TODO: handle decode errors
							out_response_int = BAD_REQUEST;
							ipc_control_statistics[0].num_bad_msgs++;
						} else 
						{
							// if a parameter is not populated in the message, it's value will be 0 in the decoded message
							if (in_message.mode != mode_e_MODE_UNKNOWN) mode_settings.mode = in_message.mode;
							mode_settings.drill_workout_id = in_message.drill_workout_id;
							mode_settings.drill_step = in_message.drill_step;
							doubles_mode =	in_message.doubles;
							// tiebreak_mode = in_message.tie_breaker;
							mode_settings.temporary_tie_breaker_mode = in_message.tie_breaker;
							// mode_settings.iterations = in_message.iterations;
						}
					}
				}
				else
				{
					LOG_WARNING("PUT MODE rejected because boomer_base is Active\n");
					out_response_int = LOCKED;
				}
			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARM], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT PARMS");
				if (in_pbbuffer_len < 1)
				{
					sprintf(plog_string, "No message component for: %s", buffer);
					LOG_ERROR( plog_string);
					ipc_control_statistics[0].num_bad_msgs++;
					out_response_int = BAD_REQUEST;
				}
				else
				{
					b_params_msg in_message = b_params_msg_init_zero;
					pb_istream_t stream = pb_istream_from_buffer(pbbuffer, in_pbbuffer_len);
					if (!pb_decode(&stream, b_params_msg_fields, &in_message))
					{
						sprintf(plog_string, "Decode of PUT PARMS parameters failed: %s", PB_GET_ERROR(&stream));
						LOG_ERROR( plog_string);
						// TODO: handle decode errors
						out_response_int = BAD_REQUEST;
						ipc_control_statistics[0].num_bad_msgs++;
					}
					else 
					{
						if (mode_settings.mode == mode_e_GAME)
						{
							boomer_level = (uint8_t) in_message.level;
						}
						else
						{
							level_mod = (int8_t) in_message.level;
						}
						speed_mod = (uint8_t) in_message.speed;
						height_mod = (int8_t) in_message.height;
						frequency_mod = (uint16_t) in_message.delay;
					}
				}
			}
			else
			{
				sprintf(plog_string, "Illegal resource for PUT: %s", buffer);
				LOG_WARNING( plog_string);
				out_response_int = NOT_FOUND;
				ipc_control_statistics[0].num_bad_msgs++;
			}
		}
		else
		{
			sprintf(plog_string, "Unrecognized method: %s", buffer);
			LOG_WARNING( plog_string);
			out_response_int = METHOD_NOT_ALLOWED;
			ipc_control_statistics[0].num_bad_msgs++;
		}

		// send response
		memset(buffer, 0, sizeof(buffer));
		out_buffer_len = BIPC_HEADER_LENGTH;
		// the +1 in the following length is to leave a byte for the null string terminator
		snprintf((char *)buffer, BIPC_HEADER_LENGTH+1, "%s %3d       ", PROTOCOL_ID, out_response_int);

		if (out_pbbuffer_len > 0)
		{
			memcpy(buffer+BIPC_HEADER_LENGTH, pbbuffer, out_pbbuffer_len);
			out_buffer_len += out_pbbuffer_len;
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
