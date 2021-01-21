#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <stdbool.h>
#include <stdint.h>

// 'emulated' base variables
uint8_t boomer_level = 2;
int8_t level_mod = 0;  //Level Modifier, units are half levels
uint16_t frequency_mod = 0; //Modifies how often balls are thrown 200 = 200% speed 50 = 50% speed
int8_t height_mod = 0;  //Height Modifier degrees
uint8_t speed_mod = 100;  //Speed Modifier 10 = 10% 200=200%

enum {IDLE_GS, NON_IDLE_GS};
uint8_t game_state = IDLE_GS;
enum {IDLE_DS, NON_IDLE_DS};
uint8_t drill_state = IDLE_DS;


uint32_t soft_fault = 0;
uint32_t hard_fault = 0;

void start_game()
{
  game_state = NON_IDLE_GS;
  printf("Starting Game\n");
}
void end_game()
{
  game_state = IDLE_GS;
  printf("Stopping Game\n");
}
void start_drill()
{
  drill_state = NON_IDLE_DS;
  printf("Starting Drill\n");
}
void end_drill()
{
  drill_state = IDLE_DS;
  printf("Stopping Drill\n");
}

#ifdef __cplusplus
}
#endif