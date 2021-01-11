/*
   UI message handler:
    - recieves and decodes messages
    - updates base control structures for PUT operations
    - reads base control structures to provide config and operational values for GET operations
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // needed for sleep

#include "ipc_fifo_transport.h"
#include "ipc_ui_msg_handler.h"
#include "mylog.h"

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h"

// the following macro strips the path, leaving just the filename does not strip the .c suffix
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

void *uiMessageHandlerThread(void *thread_args_struct)
{
  ui_msg_handler_thread_args_t * args = (ui_msg_handler_thread_args_t *) thread_args_struct;
  LOG_DEBUG(module_category, "Message handler thread started");
  
  ipc_class_t ipc_desc;
  ipc_desc.initialized = 0;
  if (ipc_init(&ipc_desc, args->my_name_ptr, args->other_name_ptr) < 0)
  {
    LOG_FATAL(module_category, "ipc init failed!");
    exit(1);
  }
  else
    LOG_DEBUG(module_category, "ipc init OK.");

  char plog_string[96];

  uint8_t pbbuffer[MAX_MESSAGE_SIZE] = {0};
  int in_pbbuffer_len, out_pbbuffer_len;

  int out_response_int;

  uint8_t buffer[MAX_MESSAGE_SIZE] = {0};
  int out_buffer_len;
  int byte_count_rcvd = 0;
  int byte_count_sent = 0;
            
  bool do_read_respond = true;
  
  while (do_read_respond)
  {
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
        LOG_ERROR(module_category, "BIPC not in header.");
        out_response_int = 400;
        ipc_desc.num_bad_msgs++;
      }
      else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[GET], METHOD_STRING_LENGTH))
      {
        if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STATU], RSRC_STRING_LENGTH))
        {
          LOG_DEBUG(module_category,"GET STATU - sending status");
          b_status_msg msg_status = b_status_msg_init_zero;
          msg_status.active = args->status_ptr->active;
          msg_status.done = args->status_ptr->done;
          msg_status.soft_fault = args->status_ptr->soft_fault;
          msg_status.hard_fault = args->status_ptr->hard_fault;
          pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
          if (!pb_encode(&stream, b_status_msg_fields, &msg_status))
          {
            sprintf(plog_string, "Encoding failed on STATUS message: %s\n", PB_GET_ERROR(&stream));
            LOG_ERROR(module_category, plog_string);
            out_response_int = 400;
            // TODO:  handle encode errors
          }
          out_pbbuffer_len = stream.bytes_written;
        } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE_], RSRC_STRING_LENGTH))
        {
          LOG_DEBUG(module_category,"GET MODE - sending mode");
          b_mode_msg msg_mode = b_mode_msg_init_zero;
          msg_mode.mode = args->mode_ptr->mode;
          msg_mode.drill_workout_id = args->mode_ptr->drill_workout_id;
          msg_mode.drill_step = args->mode_ptr->drill_step;
          msg_mode.iterations = args->mode_ptr->iterations;
          pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
          if (!pb_encode(&stream, b_mode_msg_fields, &msg_mode))
          {
            sprintf(plog_string, "Encoding failed on MODE message: %s\n", PB_GET_ERROR(&stream));
            LOG_ERROR(module_category, plog_string);
            // TODO: handle encode errors
            out_response_int = 400;
          }
          out_pbbuffer_len = stream.bytes_written;
        } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARMS], RSRC_STRING_LENGTH))
        {
          LOG_DEBUG(module_category,"GET PARAMETERS - should send parms");
        } else
        {
          sprintf(plog_string, "Unrecognized resource for GET: %s", buffer);
          LOG_ERROR(module_category, plog_string);
          ipc_desc.num_bad_msgs++;
        }
      } else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[PUT], METHOD_STRING_LENGTH))
      {
        // Process OUT messages
        if (byte_count_rcvd <= BIPC_HEADER_LENGTH)
        {
          sprintf(plog_string, "No message component for: %s", buffer);
          LOG_ERROR(module_category, plog_string);
          ipc_desc.num_bad_msgs++;
        }
        else
        {
          //copy out message portion for decoding
          in_pbbuffer_len = byte_count_rcvd - 15; // why does the define not work?? BIPC_HEADER_LENGTH;
          memcpy(pbbuffer, buffer+BIPC_HEADER_LENGTH, in_pbbuffer_len);

          if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[START], RSRC_STRING_LENGTH))
          {
            LOG_DEBUG(module_category,"PUT START - should send response");
          } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STOP_], RSRC_STRING_LENGTH))
          {
            LOG_DEBUG(module_category,"PUT STOP - should send response");
          } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE_], RSRC_STRING_LENGTH))
          {
            b_mode_msg in_message = b_mode_msg_init_zero;
            pb_istream_t stream = pb_istream_from_buffer(pbbuffer, in_pbbuffer_len);
            if (!pb_decode(&stream, b_mode_msg_fields, &in_message))
            {
              sprintf(plog_string, "Decode of PUT MODE parameters failed: %s", PB_GET_ERROR(&stream));
              LOG_ERROR(module_category, plog_string);
              // TODO: handle decode errors
              out_response_int = 400;
              ipc_desc.num_bad_msgs++;
            } else 
            { 
              args->mode_ptr->mode = in_message.mode;
              args->mode_ptr->drill_workout_id = in_message.drill_workout_id;
              args->mode_ptr->drill_step = in_message.drill_step;
              args->mode_ptr->iterations = in_message.iterations;
            }
          } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARMS], RSRC_STRING_LENGTH))
          {
            LOG_DEBUG(module_category,"PUT PARMS - should send response");
          } else
          {
            sprintf(plog_string, "Illegal resource for PUT: %s", buffer);
            LOG_WARNING(module_category, plog_string);
            out_response_int = 400;
            ipc_desc.num_bad_msgs++;
         }
        }
      } else
      {
        sprintf(plog_string, "Unrecognized method: %s", buffer);
        LOG_WARNING(module_category, plog_string);
        out_response_int = 400;
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
          LOG_ERROR(module_category, plog_string);
          // TODO: handle IPC send errors
      }
    }
    sleep(1);
  }
  // TODO: why is the following return necessary?  The thread has a void return
  return 0;
}
