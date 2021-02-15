#ifdef __cplusplus
extern "C" {
#endif

// #pragma once
#ifndef ipc_test_support_h
#define ipc_test_support_h

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "global_parameters.h"

enum {IDLE_GS, NON_IDLE_GS};
enum {IDLE_DS, NON_IDLE_DS};

uint8_t set_boomer_level(uint8_t setting);
void start_game(void);
void end_game(void);
void pause_game(void);
void resume_game(void);

void load_drill(int16_t drill_id);
void start_drill(void);
void end_drill(void);
void pause_drill(); //pauses the drill but retains progress
void unpause_drill();
int8_t set_drill_delay (int16_t setting);
int8_t set_drill_speed (uint8_t setting);
int8_t set_drill_height (int8_t setting);


typedef enum _soft_fault_e {
	soft_fault_e_NONE = 0,
	soft_fault_e_OUT_OF_BALLS = 1,
} soft_fault_e;

typedef enum _hard_fault_e {
	hard_fault_e_NONE = 0,
	hard_fault_e_ADC = 1,
	hard_fault_e_DAC = 2,
	hard_fault_e_TACH = 3,
	hard_fault_e_GPIO = 4,
} hard_fault_e;

#endif //ifndef ipc_test_support_h

#ifdef __cplusplus
}
#endif