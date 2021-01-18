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

#include "ipc_ui_msg_handler.h"
#include "boomer_log.h"

#define FIFO_NAME_LENGTH 14

// the following macro strips the path, leaving just the filename does not strip the .c suffix
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

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

  // globals:
  b_status_t base_status = {0};  //written by the base; read by the UI
  // b_control_t base_control = {0}; 
  b_mode_settings_t base_mode = {0};  //read by the base; write/read by the UI
  b_param_settings_t base_params = {0}; //read by the base; write/read by the UI

  ui_2_desc_t ui_2_desc = {0};
  ui_2_desc.status_ptr = &base_status;
  ui_2_desc.mode_ptr = &base_mode;
  ui_2_desc.params_ptr = &base_params;

  ui_2_init(&ui_2_desc);

  while(1)
  {
    ui_2_update(&ui_2_desc);
    sleep(1);
  }

  LOG_INFO(module_category, "ipc test completed.");
  LOG_FINI();

  exit(0);
}