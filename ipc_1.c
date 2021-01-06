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

  while (my_ipc->connected != 1) {
    sprintf(my_ipc->plog_string, "Opening %s for reading.", my_ipc->fifo_fname_read);
    LOG_DEBUG(module_category, my_ipc->plog_string);
    my_ipc->fd_read = 0;
    while (my_ipc->fd_read < 1) {
      my_ipc->fd_read = open(my_ipc->fifo_fname_read, O_RDONLY | O_NONBLOCK);
      if (my_ipc->fd_read < 1) {
        perror("Open read FIFO failed: ");
        pthread_exit(NULL);
      }
    }
    sprintf(my_ipc->plog_string, "Opening %s for writing.", my_ipc->fifo_fname_write);
    LOG_DEBUG(module_category, my_ipc->plog_string);
    // LOG_DEBUG(module_category, "Opened read FIFO, now opening write FIFO");
    // open fifo for write will block until read end is opened
    my_ipc->fd_write = 0;
    while (my_ipc->fd_write < 1) {
      my_ipc->fd_write = open(my_ipc->fifo_fname_write, O_WRONLY | O_NONBLOCK);
      if (my_ipc->fd_write < 1) {
        if (errno != 6) {
          perror("Open write FIFO failed: ");
          pthread_exit(NULL);
        }
      }
    }
    my_ipc->connected = 1;
  }
  pthread_exit(NULL);
}


int ipc_init(ipc_class_t *this_ipc, char * my_proc_name , char * other_proc_name) {
  int rc;

  if (this_ipc->initialized == 0)
  {
    this_ipc->connected = 0;
  
    sprintf(this_ipc->fifo_fname_write, "/tmp/%sTo%s.fifo", my_proc_name, other_proc_name);
    sprintf(this_ipc->plog_string, "Creating FIFO: %s", this_ipc->fifo_fname_write);
    LOG_DEBUG(module_category, this_ipc->plog_string);

    if (mkfifo( (char *) this_ipc->fifo_fname_write, 0666) != 0)
    {
      if (errno != EEXIST)
      { //ignore if file exists
        perror("Error on mkfifo:");
        return(-1);
      }
    }

    sprintf(this_ipc->fifo_fname_read, "/tmp/%sTo%s.fifo", other_proc_name, my_proc_name);
    if (mkfifo( (char *) this_ipc->fifo_fname_read, 0666) != 0)
    {
      if (errno != EEXIST) {
        perror("Error on read mkfifo:");
        return(-1);
      }
    }
  this_ipc->num_read_msgs = 0;
  this_ipc->num_write_msgs = 0;
  this_ipc->num_bad_msgs = 0;
  this_ipc->initialized = 1;
  }
  if (this_ipc->connected == 0)
  {
    rc = pthread_create(&this_ipc->open_fifo_thread, NULL, openFifosThread, (void *)this_ipc);
    if (rc)
    {
        perror("Unable to create open fifo thread: %d\n");
        return(-2);
    }
  }
  return 0;
}


int ipc_msg_poll(ipc_class_t *this_ipc) {
  int rc = 0;
  ipc_class_t * my_ipc = (ipc_class_t *) this_ipc;

  if (!my_ipc->connected)
    return rc;

  struct pollfd poll_fd_read;
  poll_fd_read.fd = my_ipc->fd_read;
  poll_fd_read.events = POLLIN;

  poll( &poll_fd_read, 1, 0);
  if (poll_fd_read.revents & POLLIN) {
    rc = 1;
  }
  return rc;
}


int ipc_send(ipc_class_t *this_ipc, method_t method, resource_t resource, int response_status, \
              uint8_t pbbuffer_len, uint8_t pbbuffer[]) {
  ipc_class_t * my_ipc = (ipc_class_t *) this_ipc;
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
  byte_count_sent = write(my_ipc->fd_write, payload, payload_length);
  if (byte_count_sent < 0)
  {
    if (errno == EPIPE)
    {
      my_ipc->fd_write = 0;
      my_ipc->connected = false;
    }
    else perror("Error on Pipe Write: ");
  }
  else if (byte_count_sent < payload_length)
  {
    sprintf(my_ipc->plog_string, "Wrote %d bytes of message: %s", byte_count_sent, payload);
    LOG_ERROR(module_category, my_ipc->plog_string);
  }
  else
  {
    this_ipc->num_write_msgs++;
    rc = 0;
  }
  return rc;
}


int ipc_recv(ipc_class_t *this_ipc, method_t *method_e, resource_t *resource_e, \
      int * response_int, int * pbbuffer_len, uint8_t pbbuffer[]) {
  ipc_class_t * my_ipc = (ipc_class_t *) this_ipc;
  int byte_count_rcvd;
  uint8_t buffer[MAX_MESSAGE_SIZE] = {0};
  *method_e = INVALID_METHOD;
  *resource_e = INVALID_RESOURCE;
  *pbbuffer_len = 0;
  char response_code[4] = {0};
  bool is_response = false;

  byte_count_rcvd = read(my_ipc->fd_read, buffer, sizeof(buffer));
  if (byte_count_rcvd < 1) return byte_count_rcvd;

  // printf("Received from FIFO: %s\n", buffer);
  if (memcmp(buffer, PROTOCOL_ID, sizeof(PROTOCOL_ID)-1) != 0)
  {
    LOG_ERROR(module_category, "BIPC not in header.");
    this_ipc->num_bad_msgs++;
    return -1;
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
    return -2;
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
    } else
    {
      sprintf(this_ipc->plog_string, "Unrecognized resource: %s", buffer);
      LOG_ERROR(module_category, this_ipc->plog_string);
      this_ipc->num_bad_msgs++;
      return -3;
    }
  }
  if (byte_count_rcvd > BIPC_HEADER_LENGTH)
  {
    *pbbuffer_len = byte_count_rcvd - BIPC_HEADER_LENGTH;
    memcpy(pbbuffer, buffer+BIPC_HEADER_LENGTH, *pbbuffer_len);
  }

  this_ipc->num_read_msgs++;
  return byte_count_rcvd;
}


/*
// plain text send/receive
int ipc_msg_send(ipc_class_t *this_ipc, char * message) {
  int rc;
  ipc_class_t * my_ipc = (ipc_class_t *) this_ipc;
  rc = write(my_ipc->fd_write, message, strlen(message)+1);
  return rc;
}

int ipc_msg_recv(ipc_class_t *this_ipc, char * message, int message_size) {
  int rc;
  ipc_class_t * my_ipc = (ipc_class_t *) this_ipc;
  rc = read(my_ipc->fd_read, message, message_size);
  return rc;
}
*/