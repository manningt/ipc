#pragma once

#include <stdint.h>

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


uint32_t soft_fault = 0;
uint32_t hard_fault = 0;

