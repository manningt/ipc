#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <stdbool.h>
#include <stdint.h>

// 'emulated' base variables
uint8_t boomer_level = 2;
// int8_t level_mod = 0;
int16_t delay_mod = 0; 
int8_t height_mod = 0; 
uint8_t speed_mod = 100; 

enum {IDLE_GS, NON_IDLE_GS};
uint8_t game_state = IDLE_GS;
enum {IDLE_DS, NON_IDLE_DS};
uint8_t drill_state = IDLE_DS;

bool doubles_mode = false;
bool FAKE_tiebreak_mode = false;

void start_game()
{
	game_state = NON_IDLE_GS;
	printf("Starting Game - Doubles: %d, TieBreaker: %d, Level: %d\n", \
		doubles_mode, FAKE_tiebreak_mode, boomer_level);
}
void end_game()
{
	game_state = IDLE_GS;
	printf("Stopping Game\n");
}
void start_drill()
{
	drill_state = NON_IDLE_DS;
	printf("Starting Drill - Level: %d, Speed: %d, Height: %d, Delay: %d\n", \
		boomer_level, speed_mod, height_mod, delay_mod);
}
void end_drill()
{
	drill_state = IDLE_DS;
	printf("Stopping Drill\n");
}
void load_drill(int16_t drill_id)
{
	printf("Loading Drill: %d\n", drill_id);
}

#ifdef __cplusplus
}
#endif