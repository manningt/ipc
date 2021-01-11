#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // needed for sleep

#include "ipc_1_transport.h"
#include "mylog.h"

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h"

// globals for testing - these are shared data structures between the Base code and IPC code
b_status_t base_status = {0};
b_control_t base_control = {0};
b_mode_settings_t base_mode = {0};

// the following macro strips the path, leaving just the filename does not strip the .c suffix
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

int main(int argc, char *argv[])
{
  char my_name[FIFO_NAME_LENGTH] = "Base";
  char other_name[FIFO_NAME_LENGTH] = "Ui";
  char *my_name_ptr = my_name;
  char *other_name_ptr = other_name;
  int rc;

  int opt;
  while ((opt = getopt(argc, argv, "rm:o:")) != -1)
  {
    switch (opt)
    {
    case 'r':
      // reverse: swap my and other names
      my_name_ptr = other_name;
      other_name_ptr = my_name;
      break;
    case 'm':
      strncpy(my_name, optarg, FIFO_NAME_LENGTH - 1);
      break;
    case 'o':
      strncpy(other_name, optarg, FIFO_NAME_LENGTH - 1);
      break;
    case ':':
      printf("option '%c' needs a value\n,", optopt);
      break;
    case '?':
      printf("unknown option: %c\n", optopt);
      break;
    }
  }

/*   
  int rc = LOG_INIT(LOG_CONFIG_PATH);
  if (rc) {
      printf("zlog init failed\n");
      return -1;
  }
  LOG_INIT_CATEGORY(__FILENAME__);
*/
  char plog_string[96];
  ipc_class_t ipc_desc;
  ipc_desc.initialized = 0;
  if (ipc_init(&ipc_desc, my_name_ptr, other_name_ptr) < 0)
  {
    LOG_FATAL(module_category, "ipc init failed!");
    return -4;
  }
  else
    LOG_DEBUG(module_category, "ipc init OK.");

  while (ipc_desc.fd_write < 1)
  {
    sleep(1);
    // printf("waiting for write FIFO\n");
  }
  LOG_DEBUG(module_category, "ipc FIFOs connected.");

  uint8_t pbbuffer[MAX_MESSAGE_SIZE] = {0}; //,  * pbbuffer_ptr = pbbuffer;
  int in_pbbuffer_len, out_pbbuffer_len;

  method_t in_method_e, out_method_e;
  resource_t in_resource_e, out_resource_e;
  int in_response_int, out_response_int;
            

  bool do_read_respond = true;
  // Read message then respond
  while (do_read_respond)
  {
    if (ipc_msg_poll(&ipc_desc))
    {
      // memset(pbbuffer, 0, sizeof(pbbuffer));
      out_pbbuffer_len = 0;
      out_method_e = INVALID_METHOD;
      out_resource_e = INVALID_RESOURCE;
      out_response_int = 200;
      rc = ipc_recv(&ipc_desc, &in_method_e, &in_resource_e, &in_response_int, &in_pbbuffer_len, pbbuffer);
      if (rc > 0) 
      {
        switch(in_method_e)
        {
          case GET:
          {
            switch(in_resource_e)
            {
              case STATU:
              {
                LOG_DEBUG(module_category,"GET STATU - sending status");
                b_status_msg msg_status = b_status_msg_init_zero;
                msg_status.active = base_status.active;
                msg_status.done = base_status.done;
                msg_status.soft_fault = base_status.soft_fault;
                msg_status.hard_fault = base_status.hard_fault;
                pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
                if (!pb_encode(&stream, b_status_msg_fields, &msg_status))
                {
                  sprintf(plog_string, "Encoding failed on STATUS message: %s\n", PB_GET_ERROR(&stream));
                  LOG_ERROR(module_category, plog_string);
                  // TODO: handle IPC recieve errors
                }
                out_pbbuffer_len = stream.bytes_written;
                out_response_int = 200;
              }
              break;
              case MODE_:
              {
                LOG_DEBUG(module_category,"GET MODE - sending mode");
                b_mode_msg msg_mode = b_mode_msg_init_zero;
                msg_mode.mode = base_mode.mode;
                msg_mode.drill_workout_id = base_mode.drill_workout_id;
                msg_mode.drill_step = base_mode.drill_step;
                msg_mode.iterations = base_mode.iterations;
                pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));
                if (!pb_encode(&stream, b_mode_msg_fields, &msg_mode))
                {
                  sprintf(plog_string, "Encoding failed on MODE message: %s\n", PB_GET_ERROR(&stream));
                  LOG_ERROR(module_category, plog_string);
                  // TODO: handle IPC recieve errors
                }
                out_pbbuffer_len = stream.bytes_written;
                out_response_int = 200;
              }
              break;
              case PARMS:
              {
                LOG_DEBUG(module_category,"GET PARAMETERS - should send parms");
              }
              break;
              default:
                sprintf(plog_string, "Illegal resource for GET: %s", RESOURCE_STRING[in_resource_e]);
                LOG_WARNING(module_category, plog_string);
            }
          }
          break;
          case PUT:
          {
            switch(in_resource_e)
            {
              case MODE_:
              {
                LOG_DEBUG(module_category,"PUT MODE - sending response");
                if (in_pbbuffer_len > 0)
                {
                  // printf("in_pbbuffer_len: %d\n", in_pbbuffer_len);
                  b_mode_msg in_message = b_mode_msg_init_zero;
                  pb_istream_t stream = pb_istream_from_buffer(pbbuffer, in_pbbuffer_len);
                  if (!pb_decode(&stream, b_mode_msg_fields, &in_message))
                  {
                    sprintf(plog_string, "Decode of PUT MODE parameters failed: %s", PB_GET_ERROR(&stream));
                    LOG_ERROR(module_category, plog_string);
                    out_response_int = 200;
                  } else 
                  { 
                    // sprintf(plog_string, "PUT MODE: mode: %d  drill_id: %d", in_message.mode, in_message.drill_workout_id);
                    // LOG_DEBUG(module_category, plog_string);
                    base_mode.mode = in_message.mode;
                    base_mode.drill_workout_id = in_message.drill_workout_id;
                    base_mode.drill_step = in_message.drill_step;
                    base_mode.iterations = in_message.iterations;
                  }
                } else {
                  LOG_ERROR(module_category,"PUT MODE did not have a message component");
                }
              }
              break;
              case PARMS:
              {
                LOG_DEBUG(module_category,"PUT PARAMETERS - should send response");
              }
              break;
              case START:
              {
                LOG_DEBUG(module_category,"PUT START - should send response");
              }
              break;
              case STOP_:
              {
                LOG_DEBUG(module_category,"PUT STOP - should send response");
              }
              break;

              default:
                sprintf(plog_string, "Illegal resource for PUT: %s", RESOURCE_STRING[in_resource_e]);
                LOG_WARNING(module_category, plog_string);
            }
          }
          break;
          case INVALID_METHOD:
          {
            if (in_response_int != 0)
            //process response
            {
              printf("Received response: %d\n", in_response_int);
            }
            else
            {
              LOG_WARNING(module_category,"Bad receive: invalid method and response code was zero");
            }
          }
          break;
          default:
            LOG_FATAL(module_category,"received unrecognized method in message - should not get here.");
            exit(1);
        }
        // send response
        rc = ipc_send(&ipc_desc, out_method_e, out_resource_e, out_response_int, out_pbbuffer_len, pbbuffer);
        if (rc < 1)
        {
            sprintf(plog_string, "Send Status response failed: ipc_send_code: %d", rc);
            LOG_ERROR(module_category, plog_string);
            // TODO: handle IPC send errors
        }
      }
    }
    sleep(1);
  }
  LOG_INFO(module_category, "ipc test completed.");
  LOG_FINI();
}
