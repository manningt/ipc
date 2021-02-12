
#include <stdbool.h>
#include <stdint.h>
#include "ipc_test_support.h"
#include "ipc_control.h"

uint8_t boomer_level = 2;
int16_t delay_mod = 0; 
int8_t height_mod = 0; 
uint8_t speed_mod = 100; 

uint8_t game_state = IDLE_GS;
uint8_t drill_state = IDLE_DS;

bool simulation_mode = false;
bool tie_breaker_only_opt = false;
bool all_player_serve_opt = false;
bool all_boomer_serve_opt = true;
bool run_reduce_opt;
uint8_t game_point_delay_opt;
bool grunts_opt = true;

// game statistics
bool player_serve;
uint8_t player_sets;
uint8_t boomer_sets;
uint8_t player_games;
uint8_t boomer_games;
uint8_t player_points;
uint8_t boomer_points;
uint8_t player_tie_points;
uint8_t boomer_tie_points;
uint64_t game_start_time;

double cam_calib_pts[NUM_CAMERAS][NUM_CAM_CALIB_POINTS][2] = {0};

uint32_t soft_fault = 0;
uint32_t hard_fault = 0;


void start_game()
{
	extern uint8_t boomer_level;
	extern uint8_t game_state;
	extern bool doubles_mode;

	game_state = NON_IDLE_GS;
	printf("Starting Game - TieBreaker: %d, Level: %d, AllPlayerServe: %d, AllBoomerServe: %d, ReduceRun: %d, DelayAfterPoint: %d, Grunts: %d\n", \
		tie_breaker_only_opt, boomer_level, all_player_serve_opt, all_boomer_serve_opt, run_reduce_opt, game_point_delay_opt, grunts_opt);

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