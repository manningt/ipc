/*
  IPC test:
    -- provides the globals for the control IPC to read/write
    -- instantiates the ipc_control message handler 
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // needed for sleep

#include "ipc_control.h"
#include "logging.h"

#define FIFO_NAME_LENGTH 14

#define IPC_CONTROL_TEST

int main(int argc, char *argv[])
{
  char my_name[FIFO_NAME_LENGTH] = "Base";
  char other_name[FIFO_NAME_LENGTH] = "Ui";
  char *my_name_ptr = my_name;
  char *other_name_ptr = other_name;

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

  LOG_DEBUG( "ipc test start.");
 
  ipc_control_desc_t ipc_control_desc = {0};
  ipc_control_init(&ipc_control_desc);

  while(1)
  {
    ipc_control_update(&ipc_control_desc);
    sleep(1);
  }

  LOG_DEBUG( "ipc test completed.");
}
