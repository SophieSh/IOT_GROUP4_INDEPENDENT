# Popup Messages Feature - Implementation Guide

## Overview

The quiz game uses popup messages (MessageBox) to give user feedback on game events, errors, and system status. All popups support multilingual text and auto-close after a timeout.

---

## Implementation

Popup creation using LVGL's MessageBox:

```cpp
lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_BELL, "Time's Up", NULL, false);
lv_obj_center(pop_up);
lv_obj_set_width(pop_up, 150);

// Auto-close after 1 second
lv_timer_t *timer = lv_timer_create(close_msgbox_timer_cb, 1000, pop_up);
lv_timer_set_repeat_count(timer, 1);
```

**Parameters:**
- `parent` - NULL (spawns on active screen)
- `title` - Icon symbol (LV_SYMBOL_OK, LV_SYMBOL_CLOSE, etc.)
- `txt` - Message text (language-specific)
- `btns` - NULL (no buttons)
- `add_close_btn` - false (auto-close via timer)

---

## Popup Types

### 1. Game Feedback Popups

**Correct Answer:**
```cpp
lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_OK, get_text_yes(), NULL, false);
// Shows: ✓ "YES" / "כן" for 1 second
```

**Wrong Answer:**
```cpp
lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_CLOSE, get_text_no(), NULL, false);
// Shows: ✗ "NO" / "לא" for 1 second
```

**Time Expired:**
```cpp
lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_BELL, get_text_times_up(), NULL, false);
// Shows: ⏰ "Time's Up" / "נגמר הזמן" for 1 second
```

### 2. System Status Popups

**SD Card:**
```cpp
// Success: "SD Card successfully mounted!" / "Success"
// Error: "Error: SD Card mounting fail..." / "שגיאה"
lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_SD_CARD, message, NULL, false);
```

**WiFi:**
```cpp
// Success: "WiFi connected successfully!" / "Success"
// Error: "Error: WiFi connection failed..." / "שגיאה"
lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_WIFI, message, NULL, false);
```

---

## Auto-Close Mechanism

All popups auto-close using LVGL timers:

```cpp
static void close_msgbox_timer_cb(lv_timer_t *timer) {
    lv_obj_t *msgbox = (lv_obj_t*)timer->user_data;
    lv_msgbox_close(msgbox);
}

// Create 1-second timer
lv_timer_t *timer = lv_timer_create(close_msgbox_timer_cb, 1000, pop_up);
lv_timer_set_repeat_count(timer, 1);  // Run once
```

**How it works:**
1. Create popup message
2. Create timer with 1000ms delay
3. Pass popup pointer as timer data
4. Timer callback calls `lv_msgbox_close()`
5. Popup automatically disappears

---

## Challenges Solved

### Challenge: Non-Blocking Feedback

**Problem:** Popups with buttons block game flow - user must tap to dismiss

**Solution:**
- Auto-close timers instead of user interaction
- 1 second timeout for all game popups
- Non-blocking user experience
- Timer callback automatically closes popup

---

## Summary

The **Popup System** provides user feedback using LVGL MessageBox widgets. All popups:
1. Auto-close after 1 second via timers
2. Support multilingual text (English/Hebrew)
3. Use appropriate icons for message type
4. Are sized based on message length

**Popup Events:**
- Correct/wrong answers (YES/NO popup)
- Timer expiration (Time's Up popup)
- System status (WiFi/SD card messages)
- Game errors (No images, connection failures)

