# C-based IPC using named pipes (pipes to be converted to named sockets)

The 4 files to be used to include the control IPC are:
- ipc_control.c & h
- ipc_fifo_transport.c & h

There should be no behavior change with the inclusion of ipc_control - the current screens should all work.  I have python scripts to send requests and process replies, and do unit/regression tests.  I will be putting those in a seperate repository, and will make an easy to use set of scripts to issue start/stop game/drill from the shell.

The file _ipc_control_test.c_ is used to make an executable for unit testing. It is not be included in the boomer build; there is a conditional define that controls it: #ifdef IPC_CONTROL_TESTING
It is the equivalent of what is done in top.c, e.g.:
- Call to ipc_control_init before the while loop
- call ipc_control_update in the while loop - this does a non-blocking poll for messages and processes a message if there is one.

ipc_test_support.<c,h> take the place of paramters and functions defined in boomer_base (currently infrastructure) and are used for unit testing of the IPC code, the python/shell scripts and the user-interface.

