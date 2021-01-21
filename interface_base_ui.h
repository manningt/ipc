#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct b_status {
  bool active;
  bool done;
  uint32_t soft_fault;
  uint32_t hard_fault;
  bool ack;
} b_status_t;

typedef struct b_mode_settings {
  uint32_t mode;
  uint32_t drill_workout_id;
  uint32_t drill_step;
  uint32_t iterations;
} b_mode_settings_t;

typedef struct b_param_settings {
  uint32_t level;
  uint32_t speed;
  uint32_t height;
  uint32_t delay;
} b_param_settings_t;

#ifdef __cplusplus
}
#endif