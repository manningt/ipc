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
	LOG_DEBUG( "ipc test start.");

	ipc_control_init();

	while(1)
	{
		ipc_control_update();
		sleep(1);
	}

	LOG_DEBUG( "ipc test completed.");
}
