# Score Tracking Feature - Implementation Guide

## Overview

The **Score Tracking System** tracks player performance, manages game completion, and displays results. It uses four counters to monitor correct/wrong answers and determine when the game is complete.

---

## Core Data Structures

The scoring system uses **four key counters** stored in the `Game` class:

```cpp
class Game {
private:
    int correct_answers_m;      // Increments when user answers correctly
    int wrong_answers_m;        // Increments when user answers wrong OR times out
    int images_completed_m;     // Tracks how many images have been shown
    bool game_loop_complete_m;  // Flag when all images completed
};
```

**Initialization:**
```cpp
Game(lv_obj_t *img, lv_obj_t *timer) : 
    correct_answers_m(0), wrong_answers_m(0), 
    images_completed_m(0), game_loop_complete_m(false) {
    // Game starts fresh with all counters at 0
}
```

---

## Score Tracking Events

Scores are updated at three moments:

### 1. Correct Answer ✅
```cpp
void Game::handle_success() {
    correct_answers_m++;  // Line 673
}
```

### 2. Incorrect Answer ❌
```cpp
void Game::handle_failure() {
    wrong_answers_m++;  // Line 709
}
```

### 3. Timeout ⏰
```cpp
static void on_timer_timeout(lv_anim_t * a) {
    increment_wrong_answers();  // Line 820 - counts as wrong
}
```

**Important:** Timeouts count as wrong answers to maintain game pressure.

---

## Game Completion Detection

System detects when all images have been shown once:

```cpp
void iterate_image() {
    images_completed_m++;  // Line 358
    
    if (images_completed_m >= image_name_list_m.size()) {
        game_loop_complete_m = true;  // Game is done!
    }
}
```

After every answer (correct, wrong, or timeout), we check:

```cpp
if (is_game_complete()) {
    show_results_screen();  // End game!
    return;
}
// Otherwise, continue with next question
```

---

## Results Screen Display

Shows player's final score when game completes:

```cpp
void show_results_screen() {
    int correct = global_game->get_correct_answers();  // e.g., 3
    int wrong = global_game->get_wrong_answers();       // e.g., 1
    int total = global_game->get_total_images();       // e.g., 4
    
    // Display: "Score: 3/4" (Line 1236-1243)
    char score_text[50];
    snprintf(score_text, sizeof(score_text), "%s: %d/%d", 
             get_text_score(), correct, total);
}
```

**UI Elements:**
- Title: "Game Complete!" / "!המשחק הסתיים"
- Correct count (green) + Wrong count (red) + Total score (white)
- Play Again button

---

## Score Flow Example (4 images)

```
Q1 "cat" → click 'c' → correct_answers_m = 1
Q2 "dog" → timeout  → wrong_answers_m = 1
Q3 "bird" → click 'b' → correct_answers_m = 2
Q4 "tree" → click 't' → correct_answers_m = 3
            → images_completed_m = 4 ✓ complete!
            
Results: Correct: 3, Wrong: 1, Score: 3/4
```

## Accessor Methods

Read-only methods to get scores:

```cpp
int get_correct_answers() const;   // Lines 371-373
int get_wrong_answers() const;     // Lines 375-377
int get_total_images() const;      // Lines 379-381
bool is_game_complete() const;     // Lines 367-369
```

---

## Game Reset

Resets all counters and returns to first image:

```cpp
void play_again_callback(lv_event_t *e) {
    global_game->reset_game();  // Zeros all counters (Line 1161)
}

void reset_game() {
    correct_answers_m = 0;
    wrong_answers_m = 0;
    images_completed_m = 0;
    game_loop_complete_m = false;
    image_pos_m = 0;  // Back to first image
}
```


