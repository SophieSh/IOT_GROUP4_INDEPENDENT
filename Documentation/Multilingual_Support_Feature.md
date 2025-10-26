# Adding Hebrew Language Support

## Overview

To add Hebrew support, we needed to handle multi-byte UTF-8 characters, create a custom keyboard, configure LVGL fonts, and manage separate image directories.

---

## Challenges Solved

### Challenge 1: Hebrew Text Not Displaying

**Problem:** Hebrew letters show as `□□□` or empty boxes

**Solution:**
1. Enable `LV_USE_UTF8 1`, `LV_USE_BIDI 1` in lv_conf.h
2. Enable `LV_FONT_DEJAVU_16_PERSIAN_HEBREW 1`
3. Apply `lv_font_dejavu_16_persian_hebrew` to all Hebrew UI elements

### Challenge 2: Standard Keyboard Doesn't Support Hebrew

**Problem:** Can't type Hebrew letters - LVGL keyboard only supports Latin layouts

**Solution:** Custom Hebrew keyboard using button matrix

lv_obj_t* create_hebrew_keyboard(lv_obj_t* parent) {
    static const char * btnm_map[] = {
        "א", "ב", "ג", "ד", "ה", "ו", "ז", "\n",
        "ח", "ט", "י", "כ", "ל", "מ", "נ", "\n",
        "ס", "ע", "פ", "צ", "ק", "ר", "ש", "ת", "\n",
        LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
    };
}
```

**Lines 911-944:** Implementation in ESP32.ino

### Challenge 3: UTF-8 Character Extraction

**Problem:** Getting first letter of "חתול" breaks - Hebrew uses multi-byte UTF-8

**Solution:**
- `get_first_letter_str()` detects UTF-8 character length (1-4 bytes)
- Correctly extracts complete multi-byte Hebrew characters
- Handles both single-byte (English) and multi-byte (Hebrew)

### Challenge 4: Image Filenames Don't Match Language

**Problem:** Need `cat.bin` for English but `חתול.bin` for Hebrew

**Solution:**
- Separate SD directories: `S:/images/english/` and `S:/images/hebrew/`
- `get_image_directory()` returns correct path based on language
- Quiz first character depends on filename's language

---

## Multilingual Text System

Helper functions return language-specific text:

```cpp
const char* get_text_times_up() {
    return selected_language == LANG_HEBREW ? "נגמר הזמן" : "Time's Up";
}

const char* get_text_yes() {
    return selected_language == LANG_HEBREW ? "כן" : "YES";
}



## Summary

The **Multilingual Support** system enables:
1. Language selection at startup
2. Custom Hebrew keyboard (button matrix - LVGL limitation workaround)
3. UTF-8 Hebrew character handling
4. Separate image directories by language
5. Dynamic UI text switching




