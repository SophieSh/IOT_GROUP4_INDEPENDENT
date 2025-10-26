# Tab System Feature - Implementation Guide

## Overview

The quiz game uses a 4-tab interface to organize different user interactions: displaying quiz images, letter button selection, keyboard input, and drawing canvas.

---

## Implementation

Create TabView container:

```cpp
lv_obj_t *tabview = lv_tabview_create(main_cont, LV_DIR_TOP, 30);
// LV_DIR_TOP = tabs at top, 30 = tab bar height
```

Add 4 tabs to the TabView:

```cpp
lv_obj_t *tab0 = lv_tabview_add_tab(tabview, LV_SYMBOL_IMAGE);     // 📷 Image
lv_obj_t *tab1 = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST);      // ≡ Letters  
lv_obj_t *tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_KEYBOARD);   // ⌨️ Keyboard
lv_obj_t *tab3 = lv_tabview_add_tab(tabview, LV_SYMBOL_EDIT);      // ✏️ Draw
```

---

## Tab Functions

### Tab 0: Image Display
Shows current quiz image from SD card (`S:/images/english/` or `S:/images/hebrew/`)
- Displays `.bin` image files
- Updates on each new question

### Tab 1: Letter Buttons  
8 random letter buttons (one correct answer)
- Language-specific (English or Hebrew)
- Correct answer always included

### Tab 2: Keyboard
Full keyboard for typing answers
- English: Standard LVGL keyboard
- Hebrew: Custom 22-letter button matrix

### Tab 3: Drawing Canvas
120x120 pixel drawing board
- Touch drawing enabled
- Can be cleared or sent to server

---

## Challenges and solutions: 

Sliding

