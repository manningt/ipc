
#include <stdbool.h>
#include <stdint.h>
#include "ipc_test_support.h"

uint8_t boomer_level = 2;
int16_t delay_mod = 0; 
int8_t height_mod = 0; 
uint8_t speed_mod = 100; 

uint8_t game_state = IDLE_GS;
uint8_t drill_state = IDLE_DS;

bool doubles_mode = false;
bool FAKE_tiebreak_mode = false;
bool simulation_mode = false;


uint32_t soft_fault = 0;
uint32_t hard_fault = 0;


void start_game()
{
	extern uint8_t boomer_level;
	extern uint8_t game_state;
	extern bool doubles_mode;
	extern bool FAKE_tiebreak_mode;

	game_state = NON_IDLE_GS;
	printf("Starting Game - Doubles: %d, TieBreaker: %d, Level: %d\n", \
		doubles_mode, FAKE_tiebreak_mode, boomer_level);
}
void end_game()
{
	extern uint8_t game_state;
	game_state = IDLE_GS;
	printf("Stopping Game\n");
}
void pause_game()
{
	extern uint8_t game_state;
	game_state = NON_IDLE_GS;
	printf("Pausing Game\n");
}
void resume_game()
{
	extern uint8_t game_state;
	game_state = IDLE_GS;
	printf("Resuming Game\n");
}
void start_drill()
{
	extern uint8_t boomer_level, speed_mod;
	extern int8_t height_mod;
	extern int16_t delay_mod;
	extern uint8_t drill_state;
	drill_state = NON_IDLE_DS;
	printf("Starting Drill - Level: %d, Speed: %d, Height: %d, Delay: %d\n", \
		boomer_level, speed_mod, height_mod, delay_mod);
}
void end_drill()
{
	extern uint8_t drill_state;
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