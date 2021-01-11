/*
  IPC test:
    -- creates the msg_handler thread, which creates the FIFO open thread
    -- TODO: create a boomer_base emulator that react to changes in the control structure
             and updates the status structure accordingly it will create a 
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // needed for sleep

#include "ipc_1_msg_handler.h"
#include "mylog.h"

#define FIFO_NAME_LENGTH 14

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

  // globals for testing - these are shared data structures between the Base code and IPC code
  b_status_t base_status = {0};
  b_control_t base_control = {0};
  b_mode_settings_t base_mode = {0};
  b_param_settings_t base_params = {0};

  msg_handler_thread_args_t thread_args;
  thread_args.my_name_ptr = my_name_ptr;
  thread_args.other_name_ptr = other_name_ptr;
  thread_args.status_ptr = &base_status;
  thread_args.control_ptr = &base_control;
  thread_args.mode_ptr = &base_mode;
  thread_args.params_ptr = &base_params;
  
  rc = pthread_create(&thread_args.msg_handler_thread, NULL, messageHandlerThread, (void *)&thread_args);
  if (rc)
  {
      perror("Unable to create message handler thread: %d\n");
      exit(1);
  }

  puts("Press any key to TERMINATE ... ");
  getchar();

  LOG_INFO(module_category, "ipc test completed.");
  LOG_FINI();

  exit(0);
}