#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // needed for sleep

#include "ipc_1.h"
#include "mylog.h"

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h"


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

  // printf("mine: %s  -- other: %s", my_name_ptr, other_name_ptr);

/*
  uint8_t pb_buffer[128] = "abcdefghijklmnopqrstuvwxyz";
  uint8_t header[] = "BIPC GET statu ";
  uint32_t buff_length;

  printf("Before memcpy: %s\n", pb_buffer);
  buff_length = strlen( (const char *) pb_buffer);
  memcpy(pb_buffer+(sizeof(header)-1), pb_buffer, buff_length);
  pb_buffer[sizeof(header)-1 + buff_length] = 0;
  memcpy(pb_buffer, header, sizeof(header)-1);
  printf("After memcpy: %s\n", pb_buffer);
  exit(0);  
*/

/*   
  int rc = LOG_INIT(LOG_CONFIG_PATH);
  if (rc) {
      printf("zlog init failed\n");
      return -1;
  }
  LOG_INIT_CATEGORY(__FILENAME__);
*/
  ipc_class_t ipc_desc;
  if (ipc_init(&ipc_desc, my_name_ptr, other_name_ptr) < 0)
  {
    LOG_FATAL(module_category, "ipc init failed!");
    return -4;
  }
  else
    LOG_DEBUG(module_category, "ipc init OK.");

  while (ipc_desc.connected == 0)
    sleep(1);
  LOG_DEBUG(module_category, "ipc FIFOs connected.");

  uint8_t pbbuffer[MAX_MESSAGE_SIZE] = {0}; //,  * pbbuffer_ptr = pbbuffer;
  uint8_t pbbuffer_len = 0;
 
  // make message and encode
  b_status out_message = b_status_init_zero;
  // Create a stream that will write to the buffer. 
  pb_ostream_t stream = pb_ostream_from_buffer(pbbuffer, sizeof(pbbuffer));

  out_message.Active = 0;
  out_message.Status = 1;
  sprintf(out_message.Condition, "%s-to-%s", my_name, other_name);

  if (!pb_encode(&stream, b_status_fields, &out_message))
  {
      printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
      return -1;
  }
  pbbuffer_len = stream.bytes_written;

  rc = ipc_send(&ipc_desc, INVALID_METHOD, INVALID_RESOURCE, 200, pbbuffer_len, pbbuffer);
  if (rc != 0)
  {
    printf("Send_request failed\n");
    exit(-1);
  }
  //clear buffer & wait for message
  memset(pbbuffer, 0, sizeof(pbbuffer));
  for (int i = 0; i < 15; i++)
  {
    rc = ipc_msg_poll(&ipc_desc);
    // printf("%d.", i);
    if (rc)
      break;
    sleep(1);
  }
  method_t method_e;
  resource_t resource_e;
  int response_int = 0;
  rc = ipc_recv(&ipc_desc, &method_e, &resource_e, &response_int, pbbuffer_len, pbbuffer);
  if (rc > 0) 
  {
    if (method_e == INVALID_METHOD)
    {
      if (response_int != 0) printf("Received response: %d\n", response_int);
      else printf("Warning: bad receive: invalid method and response code was zero\n");
    } else printf("Received Method: %s -- Resource: %s\n", METHOD_STRING[method_e], RESOURCE_STRING[resource_e]);

    // decode message based on Resource, which tells the message type (Status, config)
    if (pbbuffer_len > 0)
    {
      b_status in_message = b_status_init_zero;
      pb_istream_t stream = pb_istream_from_buffer(pbbuffer, pbbuffer_len);
      if (!pb_decode(&stream, b_status_fields, &in_message))
      {
          printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
      } else 
      { 
        printf("Message condition: %s\n", in_message.Condition);
      }
    }
  }
  LOG_INFO(module_category, "ipc test completed.");
  LOG_FINI();
}
