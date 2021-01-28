#include "ipc_fifo_transport.h"
#include <stdlib.h>   //for malloc
#include <string.h>   //for zlog category string operations
#include <assert.h>   //for zlog category
#include <stdio.h>    //for printf
#include <errno.h>

#include <pthread.h>
#include <signal.h>
#include <sys/stat.h> //for mkfifo
#include <fcntl.h> // for NONBLOCK
#include <unistd.h> // for close
#include <poll.h>

#include "logging.h"

void *openFifosThread(void *this_ipc) {
  ipc_transport_class_t * my_ipc = (ipc_transport_class_t *) this_ipc;
  LOG_DEBUG( "Fifo open thread started");
  while (true)
	{
		if (my_ipc->fd_read < 1)
		{
			// open for read does NOT block if the writer end is not open
			my_ipc->fd_read = open(my_ipc->fifo_fname_read, O_RDONLY | O_NONBLOCK);
			if (my_ipc->fd_read < 1) {
			perror("Open read FIFO failed: ");
			pthread_exit(NULL);
			}
			else
			{
			sprintf(my_ipc->plog_string, "Opened %s for reading.", my_ipc->fifo_fname_read);
			LOG_DEBUG( my_ipc->plog_string);
			}
		}
		if (my_ipc->fd_write < 1)
		{
			// open fifo for write DOES block until read end is opened
			my_ipc->fd_write = open(my_ipc->fifo_fname_write, O_WRONLY | O_NONBLOCK);
			if (my_ipc->fd_write < 1) {
			if (errno != 6) {
				perror("Open write FIFO failed: ");
				// pthread_exit(NULL);
			}
			} else
			{
			sprintf(my_ipc->plog_string, "Opened %s for writing.", my_ipc->fifo_fname_write);
			LOG_DEBUG( my_ipc->plog_string);
			}
		}
		sleep(1);
	}
}


int ipc_transport_init(ipc_transport_class_t * this_ipc, char * my_proc_name , char * other_proc_name) {
	int rc;

	if (this_ipc->initialized == 0)
	{
		sprintf(this_ipc->fifo_fname_write, "%s/%sTo%s.fifo", FIFO_PATH_PREFIX, my_proc_name, other_proc_name);
		if (mkfifo( (char *) this_ipc->fifo_fname_write, 0666) != 0)
		{
			if (errno != EEXIST)
			{ //ignore if file exists
			perror("Error on write mkfifo:");
			return(IPC_INIT_ERROR_MKFIFO_FAILED);
			}
		}

		sprintf(this_ipc->fifo_fname_read, "%s/%sTo%s.fifo", FIFO_PATH_PREFIX, other_proc_name, my_proc_name);
		if (mkfifo( (char *) this_ipc->fifo_fname_read, 0666) != 0)
		{
			if (errno != EEXIST) {
			perror("Error on read mkfifo:");
			return(IPC_INIT_ERROR_MKFIFO_FAILED);
			}
		}
		sprintf(this_ipc->plog_string, "Created FIFOs: %s & %s",  this_ipc->fifo_fname_write, \
			this_ipc->fifo_fname_read);
		LOG_DEBUG( this_ipc->plog_string);

		this_ipc->initialized = 1;
		this_ipc->fd_read = 0;
		this_ipc->fd_write = 0;

		rc = pthread_create(&this_ipc->open_fifo_thread, NULL, openFifosThread, (void *)this_ipc);
		if (rc)
		{
			perror("Unable to create open fifo thread: %d\n");
			return(IPC_INIT_ERROR_FIFO_OPEN_THREAD_FAILED);
		}
	}
	return 0;
}


int ipc_msg_poll(ipc_transport_class_t *this_ipc) {
	int rc = 0;
	if (this_ipc->fd_read > 0)
	{
		struct pollfd poll_fd_read;
		poll_fd_read.fd = this_ipc->fd_read;
		poll_fd_read.events = POLLIN;

		poll( &poll_fd_read, 1, 0);
		if (poll_fd_read.revents & POLLIN)
		{
			rc = 1;
		}
	}
	return rc;
}

int ipc_read(ipc_transport_class_t * this_ipc, uint8_t * buffer, uint8_t size) {
	uint32_t bytes_read;
	bytes_read = read(this_ipc->fd_read, buffer, size);
	if (bytes_read < 1)
	{
		perror("IPC Read error: ");
		close(this_ipc->fd_read);
		this_ipc->fd_read = 0;
	}		
	return bytes_read;
}

int ipc_write(ipc_transport_class_t * this_ipc, uint8_t * buffer, uint8_t size) {
	return write(this_ipc->fd_write, buffer, size);
}