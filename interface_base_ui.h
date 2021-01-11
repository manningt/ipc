#ifdef __cplusplus
extern "C" {
#endif

#ifndef INTERFACE_BASE_UI_H
#define IINTERFACE_BASE_UI_H

#include <stdbool.h>
#include <stdint.h>

typedef struct b_status {
  bool active;
  bool done;
  uint32_t soft_fault;
  uint32_t hard_fault;
  bool ack;
} b_status_t;

typedef struct b_control {
  bool start;
  bool stop;
} b_control_t;

typedef struct b_mode_settings {
  uint32_t mode;
  uint32_t drill_workout_id;
  uint32_t drill_step;
  uint32_t iterations;
} b_mode_settings_t;

typedef struct b_param_settings {
  uint32_t level;
  uint32_t speed;
  uint32_t elevation;
  uint32_t frequency;
} b_param_settings_t;

#endif

#ifdef __cplusplus
}
#endif