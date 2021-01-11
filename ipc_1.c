#include "ipc_1.h"
#include <stdlib.h>   //for malloc
#include <string.h>   //for zlog category string operations
#include <assert.h>   //for zlog category
#include <stdio.h>    //for printf
#include <errno.h>
#include <ctype.h>  // for isdigit

#include <pthread.h>
#include <signal.h>
#include <sys/stat.h> //for mkfifo
#include <fcntl.h> // for NONBLOCK
#include <unistd.h> // for close
#include <poll.h>

#include "mylog.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/*
static zlog_category_t* category() {
  static zlog_category_t *module_category = NULL;
  if (module_category== NULL) {
    char *module_category_name = (char*)malloc(strlen(__FILENAME__) * sizeof(char));
    strcpy(module_category_name, __FILENAME__);
     //terminate string at . to get rid of the .c extension
    *(module_category_name+(strlen(__FILENAME__)-2)) = '\0';
    module_category = LOG_GET_CATEGORY(module_category_name); assert(module_category);
    free(module_category_name);
  }
  return module_category;
}
*/

void *openFifosThread(void *this_ipc) {
  ipc_class_t * my_ipc = (ipc_class_t *) this_ipc;
  LOG_DEBUG(module_category, "Fifo open thread started");
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
        LOG_DEBUG(module_category, my_ipc->plog_string);
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
        LOG_DEBUG(module_category, my_ipc->plog_string);
      }
    }
    sleep(1);
  }
}


int ipc_init(ipc_class_t *this_ipc, char * my_proc_name , char * other_proc_name) {
  int rc;

  if (this_ipc->initialized == 0)
  {
    sprintf(this_ipc->fifo_fname_write, "/tmp/%sTo%s.fifo", my_proc_name, other_proc_name);
    if (mkfifo( (char *) this_ipc->fifo_fname_write, 0666) != 0)
    {
      if (errno != EEXIST)
      { //ignore if file exists
        perror("Error on write mkfifo:");
        return(IPC_INIT_ERROR_MKFIFO_FAILED);
      }
    }

    sprintf(this_ipc->fifo_fname_read, "/tmp/%sTo%s.fifo", other_proc_name, my_proc_name);
    if (mkfifo( (char *) this_ipc->fifo_fname_read, 0666) != 0)
    {
      if (errno != EEXIST) {
        perror("Error on read mkfifo:");
        return(IPC_INIT_ERROR_MKFIFO_FAILED);
      }
    }
    sprintf(this_ipc->plog_string, "Created FIFOs: %s & %s", this_ipc->fifo_fname_write, \
        this_ipc->fifo_fname_read);
    LOG_DEBUG(module_category, this_ipc->plog_string);

    this_ipc->num_read_msgs = 0;
    this_ipc->num_write_msgs = 0;
    this_ipc->num_bad_msgs = 0;
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


int ipc_msg_poll(ipc_class_t *this_ipc) {
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


int ipc_send(ipc_class_t *this_ipc, method_t method, resource_t resource, int response_status, \
              uint8_t pbbuffer_len, uint8_t pbbuffer[]) {
  if (this_ipc->fd_write < 1)
  {
    return FIFO_ERROR_FILE_NOT_OPEN;
  }
  int byte_count_sent;
  int rc = -1;
  uint8_t payload[128] = {0};
  int payload_length = 0;
  if ((method == INVALID_METHOD) && (response_status > 0))
  {
    // printf("sending response\n");
    payload_length = BIPC_HEADER_LENGTH;
    // the +1 in the following length is to leave a byte for the null string terminator
    snprintf((char *)payload, BIPC_HEADER_LENGTH+1, "%s %3d       ", PROTOCOL_ID, response_status);

  }
  else if ((method != INVALID_METHOD) && (response_status == 0))
  {
    // send request header
    payload_length = BIPC_HEADER_LENGTH;
    snprintf((char *)payload, BIPC_HEADER_LENGTH+1, "%s %s %s ", PROTOCOL_ID, METHOD_STRING[method], RESOURCE_STRING[resource]);
  }
  else return -2;

  if (pbbuffer_len > 0)
  {
    memcpy(payload+BIPC_HEADER_LENGTH, pbbuffer, pbbuffer_len);
    payload_length += pbbuffer_len;
  }
  // printf("Before FIFO write; payload: %s -- payload len: %d\n", payload, payload_length);
  // fflush(stdout);
  byte_count_sent = write(this_ipc->fd_write, payload, payload_length);
  rc = byte_count_sent;
  if (byte_count_sent < 0)
  {
    if (errno == EPIPE)
    {
      close(this_ipc->fd_write);
      this_ipc->fd_write = 0;
    }
    else perror("Error on Pipe Write: ");
  }
  else if (byte_count_sent < payload_length)
  {
    sprintf(this_ipc->plog_string, "Wrote %d bytes of message: %s", byte_count_sent, payload);
    LOG_ERROR(module_category, this_ipc->plog_string);
  }
  else
  {
    this_ipc->num_write_msgs++;
  }
  return rc;
}


int ipc_recv(ipc_class_t *this_ipc, method_t *method_e, resource_t *resource_e, \
      int * response_int, int * pbbuffer_len, uint8_t pbbuffer[]) {

  if (this_ipc->fd_read < 1)
  {
    return FIFO_ERROR_FILE_NOT_OPEN;
  }
  int byte_count_rcvd;
  uint8_t buffer[MAX_MESSAGE_SIZE] = {0};
  *method_e = INVALID_METHOD;
  *resource_e = INVALID_RESOURCE;
  *pbbuffer_len = 0;
  char response_code[4] = {0};
  bool is_response = false;

  byte_count_rcvd = read(this_ipc->fd_read, buffer, sizeof(buffer));
  if (byte_count_rcvd < 1)
  {
    perror("IPC Read error: ");
    close(this_ipc->fd_read);
    this_ipc->fd_read = 0;
    return byte_count_rcvd;
  }

  // printf("Received from FIFO: %s\n", buffer);
  if (memcmp(buffer, PROTOCOL_ID, sizeof(PROTOCOL_ID)-1) != 0)
  {
    LOG_ERROR(module_category, "BIPC not in header.");
    this_ipc->num_bad_msgs++;
    return FIFO_READ_ERROR_NO_BIPC;
  }
  memcpy(response_code, buffer+sizeof(PROTOCOL_ID), 3);

  if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[GET], METHOD_STRING_LENGTH))
  {
    *method_e = GET;
  } else if (!memcmp(buffer+sizeof(PROTOCOL_ID), METHOD_STRING[PUT], METHOD_STRING_LENGTH))
  {
    *method_e = PUT;
  } else if (sscanf(response_code, "%d", response_int))
  {
    is_response = true;
    *method_e = INVALID_METHOD;
  } else
  {
    sprintf(this_ipc->plog_string, "Unrecognized method: %s", buffer);
    LOG_ERROR(module_category, this_ipc->plog_string);
    // printf("Unrecognized method: %s\n", buffer);
    this_ipc->num_bad_msgs++;
    return FIFO_READ_ERROR_UNRECOGNIZED_METHOD;
  }
  if (!is_response)
  {
    if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STATU], RSRC_STRING_LENGTH))
    {
      *resource_e = STATU;
    } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[START], RSRC_STRING_LENGTH))
    {
      *resource_e = START;
    } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[STOP_], RSRC_STRING_LENGTH))
    {
      *resource_e = STOP_;
    } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[MODE_], RSRC_STRING_LENGTH))
    {
      *resource_e = MODE_;
    } else if (!memcmp(buffer+RSRC_OFFSET, RESOURCE_STRING[PARMS], RSRC_STRING_LENGTH))
    {
      *resource_e = PARMS;
    } else
    {
      sprintf(this_ipc->plog_string, "Unrecognized resource: %s", buffer);
      LOG_ERROR(module_category, this_ipc->plog_string);
      this_ipc->num_bad_msgs++;
      return FIFO_READ_ERROR_UNRECOGNIZED_RESOURCE;
    }
  }
  if (byte_count_rcvd > BIPC_HEADER_LENGTH)
  {
    // printf("header byte count: %d\n", BIPC_HEADER_LENGTH);
    // printf("rcvd byte count: %d;  msg byte count: %d\n", byte_count_rcvd, (byte_count_rcvd - BIPC_HEADER_LENGTH));
    //!! subtracting the #define BIPC_HEADER_LENGTH gave: byte_count_rcvd+4  !Not -15
    *pbbuffer_len = byte_count_rcvd - 15; //BIPC_HEADER_LENGTH;
    memcpy(pbbuffer, buffer+BIPC_HEADER_LENGTH, *pbbuffer_len);
  }

  this_ipc->num_read_msgs++;
  return byte_count_rcvd;
}


/*
// plain text send/receive
int ipc_msg_send(ipc_class_t *this_ipc, char * message) {
  int rc;
  rc = write(this_ipc->fd_write, message, strlen(message)+1);
  return rc;
}

int ipc_msg_recv(ipc_class_t *this_ipc, char * message, int message_size) {
  int rc;
  rc = read(this_ipc->fd_read, message, message_size);
  return rc;
}
*/