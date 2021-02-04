#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// 'emulated' base variables
// uint8_t boomer_level = 2;
// int16_t delay_mod = 0; 
// int8_t height_mod = 0; 
// uint8_t speed_mod = 100; 

enum {IDLE_GS, NON_IDLE_GS};
enum {IDLE_DS, NON_IDLE_DS};
// uint8_t game_state = IDLE_GS;
// uint8_t drill_state = IDLE_DS;

void start_game(void);
void end_game(void);
void pause_game(void);
void resume_game(void);
void load_drill(int16_t drill_id);
void start_drill(void);
void end_drill(void);

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

#ifdef __cplusplus
}
#endif