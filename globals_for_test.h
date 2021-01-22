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

	// int32 level = 1;  //0-12 represents 1 to 7; default 2; externally is 1.0 to 7.0 in increments of 0.5
	// int32 speed = 2;  //80 to 120 step of 2; default 100  - percent speedup
	// int32 height = 3;    //-32 to +32 step of 2; default 0; 2 units = rough 1 degree.
	// int32 delay = 4;    //-2 to 2; step of .1; default 0; seconds of delay to add between ball throws in a drill

#define LEVEL_MIN 0
#define LEVEL_MAX 12
#define LEVEL_DEFAULT 2
#define LEVEL_STEP 1
int set_boomer_level(uint8_t level) {
	int rc = 1;
	if ((level >= LEVEL_MIN) && (level >= LEVEL_MAX))
	{
		boomer_level = level;
		rc = 0;
	}
	return rc;
}
void increase_boomer_level(){
	if(boomer_level <= LEVEL_MAX) boomer_level += LEVEL_STEP;
}
void decrease_boomer_level(){
	if(boomer_level >- LEVEL_MIN) boomer_level -= LEVEL_STEP;
}
void reset_boomer_level(){
	boomer_level = LEVEL_DEFAULT;
}


enum {IDLE_GS, NON_IDLE_GS};
uint8_t game_state = IDLE_GS;
enum {IDLE_DS, NON_IDLE_DS};
uint8_t drill_state = IDLE_DS;

// uint32_t soft_fault = 0;
// uint32_t hard_fault = 0;

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