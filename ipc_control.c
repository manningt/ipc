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

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h"

#ifndef LINUX
#include "globals_for_test.h"
#endif

ipc_transport_class_t ipc_desc = {0};
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


void ipc_control_init(ipc_control_desc_t *descriptor)
{
	ipc_control_desc_t *this_cntrl = (ipc_control_desc_t *)descriptor;
	if (this_cntrl->initialized == 0)
		this_cntrl->ipc_desc = &ipc_desc;
	if (ipc_init(&ipc_desc, my_name, other_name) < 0)
	{
		LOG_ERROR( "control ipc init failed!");
	}
	else
		LOG_DEBUG( "control ipc init OK.");

	this_cntrl->initialized = 1;
}

void ipc_control_update(ipc_control_desc_t *descriptor) {
	ipc_control_desc_t *this_cntrl = (ipc_control_desc_t *)descriptor;
	if (ipc_msg_poll(&ipc_desc))
	{
		out_pbbuffer_len = 0;
		out_response_int = 200;
		
		byte_count_rcvd = read(ipc_desc.fd_read, buffer, sizeof(buffer));

		if (byte_count_rcvd < 1)
		{
		// TODO: how to handle receive errors
		perror("IPC Read error: ");
		close(ipc_desc.fd_read);
		ipc_desc.fd_read = 0;
		}
		else if (memcmp(buffer, PROTOCOL_ID, sizeof(PROTOCOL_ID)-1) != 0)
      {
        LOG_ERROR( "BIPC not in header.");
        out_response_int = BAD_REQUEST;
        ipc_desc.num_bad_msgs++;
      }
      else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[GET], METHOD_STRING_LENGTH))
      {
			// Process GETs
			if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STATU], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("GET STATU - sending status");
				b_status_msg msg_status = b_status_msg_init_zero;
				printf("Game State: %d  --  Drill State: %d\n", game_state, drill_state);
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
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE_], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("GET MODE");
				b_mode_msg msg_mode = b_mode_msg_init_zero;
				msg_mode.mode = this_cntrl->mode_settings.mode;
				msg_mode.drill_workout_id = this_cntrl->mode_settings.drill_workout_id;
				msg_mode.drill_step = this_cntrl->mode_settings.drill_step;
				// msg_mode.iterations = this_cntrl->mode_settings.iterations;
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
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARMS], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("GET PARMS");
				b_params_msg params = b_params_msg_init_zero;
				if (this_cntrl->mode_settings.mode == mode_e_GAME)
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
				ipc_desc.num_bad_msgs++;
			}
		} else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[PUT], METHOD_STRING_LENGTH))
		{
			// Process PUT messages
			if (byte_count_rcvd > BIPC_HEADER_LENGTH)
			{
				//copy out message portion for decoding
				in_pbbuffer_len = byte_count_rcvd - 15; // why does the define not work?? BIPC_HEADER_LENGTH;
				memcpy(pbbuffer, buffer+BIPC_HEADER_LENGTH, in_pbbuffer_len);
			}
			else in_pbbuffer_len = 0;

			if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[START], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT START");
				if (this_cntrl->mode_settings.mode == mode_e_GAME) start_game();
				else if (this_cntrl->mode_settings.mode == mode_e_DRILL) start_drill();
				else LOG_ERROR("Invalid mode on start: %d", this_cntrl->mode_settings.mode);
			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STOP_], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT STOP");
				if (game_state != IDLE_GS) end_game();
				else if (drill_state != IDLE_DS) end_drill();

			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE_], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT MODE");
				if (BASE_ACTIVE == 0)
				{
					// can only change the mode config params if boomer is not active
					if (in_pbbuffer_len < 1)
					{
						sprintf(plog_string, "No message component for: %s", buffer);
						LOG_ERROR( plog_string);
						ipc_desc.num_bad_msgs++;
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
							ipc_desc.num_bad_msgs++;
						} else 
						{ 
							this_cntrl->mode_settings.mode = in_message.mode;
							this_cntrl->mode_settings.drill_workout_id = in_message.drill_workout_id;
							this_cntrl->mode_settings.drill_step = in_message.drill_step;
							// this_cntrl->mode_settings.iterations = in_message.iterations;
						}
					}
				}
				else
				{
					LOG_WARNING("PUT MODE rejected because boomer_base is Active\n");
					out_response_int = LOCKED;
				}
			}
			else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARMS], RSRC_STRING_LENGTH))
			{
				LOG_DEBUG("PUT PARMS");
				if (in_pbbuffer_len < 1)
				{
					sprintf(plog_string, "No message component for: %s", buffer);
					LOG_ERROR( plog_string);
					ipc_desc.num_bad_msgs++;
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
						ipc_desc.num_bad_msgs++;
					}
					else 
					{
						if (this_cntrl->mode_settings.mode == mode_e_GAME)
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
				ipc_desc.num_bad_msgs++;
			}
		}
		else
		{
			sprintf(plog_string, "Unrecognized method: %s", buffer);
			LOG_WARNING( plog_string);
			out_response_int = METHOD_NOT_ALLOWED;
			ipc_desc.num_bad_msgs++;
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
		byte_count_sent = write(ipc_desc.fd_write, buffer, out_buffer_len);

		if (byte_count_sent < out_buffer_len)
		{
			sprintf(plog_string, "Send Status response failed: ipc_send_code: %d", byte_count_sent);
			LOG_ERROR( plog_string);
			// TODO: handle IPC send errors
		}
	}
}
