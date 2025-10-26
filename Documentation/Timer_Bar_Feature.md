# Timer Bar Feature - Implementation Guide

## Overview

The **Timer Bar** is a visual countdown timer that gives users a limited time to answer each quiz question. It's a critical UI component that combines visual feedback (color-changing progress bar), text display (seconds remaining), and game logic (triggering timeout events).

---

## Why Do We Need It?

In a quiz game, you need to:
1. **Set a time limit** - Players have a fixed duration (e.g., 30 seconds) to answer
2. **Show remaining time** - Players must see how much time is left
3. **Provide visual urgency** - Color changes from green → yellow → red as time runs out
4. **Trigger game actions** - When time expires, automatically move to the next question

---

## Architecture Overview

The timer bar implementation is split into two parts:

### 1. **`timer_bar.h`** - UI Creation & Visual Behavior
Contains the visual component that:
- Creates the progress bar widget
- Displays countdown numbers
- Changes colors based on remaining time
- Animates smoothly from full to empty

### 2. **`ESP32.ino`** - Game Logic Integration  
The `Game` class controls when the timer:
- **Starts** - When a new image/question appears
- **Pauses** - When user answers (correct or wrong)
- **Times out** - When countdown reaches zero

---

## Key Components

### 1. Creating the Timer Bar

```cpp
lv_obj_t *timer_bar = create_timer_bar(QUIZ_DURATION_SECONDS, main_cont);
```

**What happens:**
- Creates a horizontal bar widget
- Sets range from 0 to `QUIZ_DURATION_SECONDS` (e.g., 30 seconds)
- Starts an LVGL animation that counts down from 30 → 0
- The animation takes exactly 30,000 milliseconds (30 seconds)

**Example from our project:**
```cpp
#define QUIZ_DURATION_SECONDS 30

// In init_main_app():
lv_obj_t *timer_bar = create_timer_bar(QUIZ_DURATION_SECONDS, main_cont);
global_game = new Game(current_image_obj, timer_bar);
```

---

### 2. Dynamic Color Changes

The timer bar automatically changes color to show urgency:

```cpp
if (current_value > two_thirds)      // 66-100% remaining
    new_color = Green;                // "Plenty of time"
else if (current_value > one_third)  // 33-66% remaining  
    new_color = Yellow;               // "Hurry up!"
else                                 // 0-33% remaining
    new_color = Red;                  // "Almost out of time!"
```

**Visual progression for 30 seconds:**
- **30-20 sec:** Green bar (calm, relaxed)
- **20-10 sec:** Yellow bar (time pressure building)
- **10-0 sec:** Red bar (urgent!)

This happens automatically every frame via the `update_timer_color()` callback.

---

### 3. Displaying the Countdown Number

The timer shows the actual number inside/beside the bar:

**Smart positioning logic:**
- **If bar is wide enough:** Number appears **inside** the bar (white text)
- **If bar is too narrow:** Number appears **outside** to the right (black text)

This ensures the number is always readable, even when the bar shrinks to nearly empty.

```cpp
// Example: At 5 seconds, bar is narrow
// So text "5" appears outside the bar in black
```

---

### 4. Game Control Methods

The `Game` class provides three key methods:

#### **start_timer_animation()**
Starts/restarts the countdown animation:
```cpp
void Game::start_timer_animation() {
    lv_anim_del(timer_bar_obj_m, NULL);  // Clear any existing animation
    
    // Create new animation from max_value → 0
    lv_anim_set_values(&a, duration_seconds, 0);
    lv_anim_set_time(&a, duration_seconds * 1000);
    
    // Set callback when timer reaches 0
    lv_anim_set_ready_cb(&a, on_timer_timeout);
    
    lv_anim_start(&a);
}
```

**When it's called:**
- Game constructor (first question)
- After answering correctly/incorrectly (next question)
- When clicking "Play Again" (restart game)

---

#### **pause_timer_animation()**
Freezes the timer at current value:
```cpp
void Game::pause_timer_animation() {
    lv_anim_del(timer_bar_obj_m, NULL);  // Delete animation = freeze
}
```

**When it's called:**
- User clicks correct/wrong answer
- Need to show feedback popup without timer running

**Why freeze?** If timer kept running during the "YES/NO" popup, it would be unfair to the player.

---

#### **on_timer_timeout()** 
Called automatically when timer reaches 0:
```cpp
static void on_timer_timeout(lv_anim_t * a) {
    global_game->pause_timer_animation();
    global_game->increment_wrong_answers();  // Timeout = wrong answer
    
    // Show "Time's Up" popup
    lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_BELL, get_text_times_up(), NULL, false);
    
    global_game->iterate_image();  // Move to next question
    
    // Check if game completed
    if (global_game->is_game_complete()) {
        show_results_screen();
    } else {
        change_image(...);
        global_game->start_timer_animation();  // Start timer for next question
    }
}
```

**Flow when time runs out:**
1. Pause the timer
2. Count as wrong answer
3. Show "⏰ Time's Up" message
4. Move to next image
5. Either show results (if last image) OR restart timer (if more images)

---

## Complete Game Flow Example

```
1. User starts game
   → start_timer_animation() 
   → Bar: [████████████████████] 30 sec (green)

2. 15 seconds pass
   → Bar: [██████████          ] 15 sec (yellow)

3. User clicks correct answer
   → pause_timer_animation()
   → Bar: [██████████          ] 15 sec (frozen)
   → Show "✓ YES" popup
   → After 1 second:
      - Load next image
      - start_timer_animation() again
      - Bar: [████████████████████] 30 sec (green) - FRESH TIMER

4. User doesn't answer in time
   → on_timer_timeout() fires
   → pause_timer_animation()
   → Bar: [                    ] 0 sec (red)
   → Show "⏰ Time's Up" popup
   → increment_wrong_answers()
   → Load next image
   → start_timer_animation()
```

---

## Important Implementation Details

### **Animation System**
Uses LVGL's built-in animation (`lv_anim_t`):
- Runs smoothly on limited hardware (ESP32)
- Automatically interpolates values every frame
- Non-blocking (doesn't freeze your program)

### **Custom Drawing Callback**
```cpp
lv_obj_add_event_cb(timer_bar, timer_display_event_cb, LV_EVENT_DRAW_PART_END, NULL);
```
This is called **every frame** while LVGL redraws the bar, allowing us to:
- Draw custom text (the countdown number)
- Update colors dynamically
- Position text intelligently

### **Memory Management**
Always delete old animations before starting new ones:
```cpp
lv_anim_del(timer_bar_obj_m, NULL);  // Prevents memory leaks!
```

---

## How to Customize

Want to modify the timer? Here are common changes:

**Change duration:**
```cpp
#define QUIZ_DURATION_SECONDS 45  // 45 seconds instead of 30
```

**Change colors:**
```cpp
// In timer_bar.h, update_timer_color():
new_color = lv_color_make(0, 255, 0);   // Green (R, G, B)
new_color = lv_color_make(255, 165, 0); // Orange instead of yellow
new_color = lv_color_make(255, 0, 0);   // Red
```

**Change thresholds:**
```cpp
// Currently: green > 66%, yellow > 33%, red < 33%
// To make it more urgent (red sooner):
if (current_value > two_thirds)      // > 66% = green
else if (current_value > max_value/2) // 50-66% = yellow  
else                                 // < 50% = red
```

---

## Common Pitfalls

1. **Forgetting to pause:** If you don't call `pause_timer_animation()` when showing a popup, the timer continues running underneath!

2. **Not restarting:** After handling an answer, you MUST call `start_timer_animation()` for the next question, or the timer stays frozen.

3. **Multiple animations:** Always delete existing animations before starting a new one, otherwise they stack up and behavior becomes unpredictable.

---

## File Locations

- **Implementation:** `ESP32/timer_bar.h` (visual component)
- **Integration:** `ESP32/ESP32.ino` (Game class methods)
- **Configuration:** `#define QUIZ_DURATION_SECONDS 30` in ESP32.ino

---

## Summary

The Timer Bar is a **self-contained visual component** (`timer_bar.h`) that integrates with **game state management** (`Game` class) to create time-limited quiz gameplay. It demonstrates:

- **LVGL widget customization** (bar styling, custom drawing)
- **Animation system** (smooth countdown)
- **Event-driven programming** (callbacks when time expires)
- **UI/Logic separation** (visual in .h, behavior in Game class)

This pattern can be reused for any feature needing visual countdowns: cooking timers, workout intervals, parking meters, etc!

---

**Authors:** Yonatan, Gital, Sofiya  
**Created:** 08/10/2025  
**Last Updated:** 24/10/2025

