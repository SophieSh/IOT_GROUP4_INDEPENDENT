#ifndef TIMER_UI_H
#define TIMER_UI_H

#include <lvgl.h>

// This block tells the C++ compiler to use C naming conventions for the functions inside.
#ifdef __cplusplus
extern "C" {
#endif

// Function prototype: tells the main file this function is defined in timer_ui.c
void create_timer_bar(int duration_seconds); 

#ifdef __cplusplus
}
#endif

#endif // TIMER_UI_H