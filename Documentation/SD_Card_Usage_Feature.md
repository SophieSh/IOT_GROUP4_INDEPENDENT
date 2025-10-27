# SD Card Usage and Image Display Feature

## Overview

The application loads quiz images from a MicroSD card mounted on the ESP32. Images must be converted to LVGL-compatible `.bin` format and stored in language-specific directories (`S:/images/english` and `S:/images/hebrew`). Displaying these images involves a complex pipeline from SD card reading to TFT rendering.

## Architecture

### Bridge Between Arduino SD and LVGL

**Challenge:** LVGL expects standard POSIX file operations, but Arduino SD library provides different APIs.

**Solution:** Custom file system driver in `driver_handlers.h` that bridges the gap.

```cpp
#define LVGL_SD_DRIVE_LETTER 'S'

bool init_sd_card() {
    SD.begin();  // Initialize Arduino SD library
    
    lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);
    fs_drv.letter = 'S';
    
    // Connect Arduino SD functions to LVGL callbacks
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    
    lv_fs_drv_register(&fs_drv);
}
```

The Arduino `File` object is wrapped and passed to LVGL as a generic `void*` pointer.

### Reading Directory Contents

**Process:**
1. `read_directory_file_list("S:/images/english")` opens directory via LVGL
2. `lv_fs_dir_read()` calls Arduino SD `openNextFile()` for each entry
3. Only `.bin` files are collected using `checkName()` validation
4. Filenames stored in `std::vector<std::string>` for game logic

**File Filtering:**
```cpp
bool checkName(char* str) {
    // Must have exactly one dot
    // Must end with .bin
    // Must be at least 5 characters
}
```

### Image Conversion Process

**Challenge:** LVGL uses a custom binary format, not standard JPEG/PNG.

**Solution:** Convert images using the [LVGL Image Converter](https://lvgl.io/tools/imageconverter):
1. Upload source image (PNG/JPG)
2. Select output format: **CF_TRUE_COLOR** (16-bit RGB565)
3. Download `.bin` file
4. Copy to SD card: `S:/images/english/cat.bin`

The converter:
- Parses image header (width, height, color format)
- Converts to LVGL-compatible binary layout
- Optimizes for embedded display (reduces file size vs raw pixels)

### Image Loading Flow

1. **Game Constructor** loads directory:
   ```cpp
   const char* image_dir = get_image_directory();  // "S:/images/english" or "S:/images/hebrew"
   image_name_list_m = read_directory_file_list(image_dir);
   ```

2. **Get Image Path**:
   ```cpp
   std::string path = "S:/images/english/" + filename;  // e.g., "S:/images/english/cat.bin"
   ```

3. **Display image** (simple API, complex backend):
   ```cpp
   lv_img_set_src(img, path);  // LVGL loads via custom SD driver
   ```
   
   Behind this one line:
   - SD card read (via SPI)
   - Parse LVGL binary header
   - Decode pixel data into display buffer
   - Allocate memory for image chunks
   - Render to TFT via GFX library

### Memory Management

**Challenge:** Arduino SD File objects must be closed and deleted to prevent memory leaks.

**Solution:**
- `fs_close()`: Calls `file->close()` then `delete file`
- `fs_dir_close()`: Calls `dir->close()` then `delete dir`
- Each temporary `File entry` in directory iteration is explicitly closed

## Challenges Solved

### Challenge 1: LVGL Expects POSIX, Arduino Provides Different API

**Problem:** LVGL file system expects `open()`, `read()`, `seek()` functions, but Arduino SD uses `SD.open()`, `file.read()`, `file.seek()`.

**Solution:** Bridge wrapper functions map LVGL callbacks to Arduino SD methods. The File object is passed as a void pointer.

### Challenge 2: Directory Entries Are Temporary

**Problem:** Arduino SD `openNextFile()` returns a temporary File object that must be closed before reading the next entry.

**Solution:** Each directory entry is immediately closed in `fs_dir_read()`:
```cpp
File entry = dir->openNextFile();
// Copy filename immediately
strncpy(fn, entry.name(), ...);
entry.close();  // Must close before next iteration
```

### Challenge 3: Filtering Only .bin Files

**Problem:** Directory contains metadata files, system files, or other formats.

**Solution:** `checkName()` validates:
- Exactly one dot separator
- Extension is `.bin`
- Minimum 5 characters (`xxxx.bin`)

This ensures only converted LVGL images are loaded into the game.

## Summary

The SD card and image display feature encompasses multiple interconnected challenges:

1. **Image conversion**: Standard images must be converted to LVGL `.bin` format using the [online converter](https://lvgl.io/tools/imageconverter)
2. **File system bridge**: Custom driver maps Arduino SD library to LVGL POSIX-style file operations
3. **Directory management**: Reading SD directories while filtering only `.bin` files and handling temporary File objects
4. **Complex display pipeline**: The simple `lv_img_set_src()` call triggers SD reading, binary parsing, memory allocation, and hardware rendering

These components work together to enable multilingual quiz content loaded dynamically from removable SD storage.

