# C-based IPC using named pipes and nano protobufs.

The 4 files to be used to include the control IPC are:
- ipc_control.c & h
- ipc_fifo_transport.c & h

A file _faults.h_ is included so that the UI can know if/why it has stopped (the hard & soft_fault variables).  This is a placeholder for fault reporting - it's implementation should be discussed.

NOTE: To get this to compile with the existing code in infrastructure:
* _line 12 in games.c: bool doubles_mode; should be moved to games.h_
* this is so that it can be read/written by control code

The file _ipc_control_test.c_ is used to make an executable for unit testing. It should _not_ be included in the boomer build.
It is the equivalent of what is done in top.c, e.g.:
- Call to ipc_control_init before the while loop
- call ipc_control_update in the while loop - this does a non-blocking poll for messages and processes a message if there is one.

There should be no behavior change with the inclusion of ipc_control - the current screens should all work.  I have python scripts to send requests and process replies, and do unit/regression tests.  I will be putting those in a seperate repository, and will make an easy to use set of scripts to issue start/stop game/drill from the shell.

The shell can be used to send and recieve messages that don't have parameters:
* `echo "BIPC PUT START" >/dev/shm/CtrlToBase.fifo`
* `tail -f /dev/shm/BaseToCtrl.fifo` (in a seperate window)

The IPC messages are encoding and decoded using c-based [nanopb](https://github.com/nanopb/nanopb).

3 files in the nanopb directory are included in _ipc_control.c_.  Ideally the build process would have `../nanopb` as an included directory. (My makefile does - I have yet to get the settings to have VS Code build it)

To install nanopb, do:`git clone https://github.com/nanopb/nanopb`

messages.pb.h and messages.pb.c also need to be included in the build.  These are currently committed files, but they are files output by the protoc compiler when it compiles _messages.proto_ which defines the messages & enums.  Ideally/eventually the build process would include this line: `../nanopb/generator/protoc  --nanopb_out=. message.proto`

The messages.proto file can be compiled for other languages, e.g. the unit tests and the UI generator which are written in python and uses compiled message definitions.

To run protoc with the nanopb option. do the following installs:
* `python3 -m pip install -U pip`
* `python3 -m pip install python3-protobuf`
* `python3 -m pip install --upgrade protobuf`
