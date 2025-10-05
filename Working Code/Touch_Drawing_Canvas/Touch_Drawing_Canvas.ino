/*******************************************************************************
 * LVGL Canvas Widgit with drawing blue on touch
 * need to fix left-right tab movment
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <stdlib.h> 
#include <stdbool.h>
#include "FS.h"
#include "SD.h"



#define LVGL_SD_DRIVE_LETTER 'S'
#define TFT_BL 27
#define GFX_BL DF_GFX_BL // default backlight pin


/* Display configuration */
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 15 /* CS */, 14 /* SCK */, 13 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, -1 /* RST */, 3 /* rotation */, true /* IPS */);

#include "touch.h"

/* Screen and LVGL buffers */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* Current image challange */
static lv_obj_t *current_image_obj = NULL;


/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    #if (LV_COLOR_16_SWAP != 0)
        gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    #else
        gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    #endif

    lv_disp_flush_ready(disp);
}

/* Read touch points */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch_has_signal()) {
        if (touch_touched()) {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
        } else if (touch_released()) {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/* fs driver handlers */
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
    if (mode != LV_FS_MODE_RD) {
        return nullptr; // Only read mode
    }

    // Use the Arduino SD library to open the file
    File *file = new File(SD.open(path, FILE_READ));

    if (file == nullptr || !file->available()) {
        delete file;
        return nullptr;
    }

    return (void*)file;
}

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p) {
    File *file = (File *)file_p;
    
    if (file) {
        file->close();
        delete file; // Memory cleanup
        return LV_FS_RES_OK; // Indicate success
    }

    return LV_FS_RES_UNKNOWN; // Indicate failure
}

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br) {
    File *file = (File *)file_p;
    *br = file->read((uint8_t *)buf, btr);
    return (*br > 0) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) {
    File *file = (File *)file_p;
    *pos_p = file->position();
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence) {

    File *file = (File *)file_p;

    if (whence != LV_FS_SEEK_SET)
        return LV_FS_RES_NOT_IMP;
    if (file->seek(pos))
        return LV_FS_RES_OK;
    else return LV_FS_RES_UNKNOWN;
    
}

/* Changes img_obj to .bin image in new_path (from Micro-SD Card) */
void change_image(lv_obj_t *img_obj, const char *new_path) {

    // --- LVGL Object Replacement Strategy ---

    // 1. Get the parent container (to put the new image in the same spot)
    lv_obj_t *parent_cont = lv_obj_get_parent(img_obj);

    // 2. Remove the OLD image object and its resources.
    // This removes the object from the screen and frees its internal LVGL resources.
    // Since LVGL manages the object, it does not hold the file handle open after reading.
    lv_obj_del(img_obj);
            
    lv_obj_t *img = lv_img_create(parent_cont);

    lv_img_set_src(img, new_path);


    if (lv_img_get_src(img) == NULL) {
                // If loading failed, show a fallback label
        lv_obj_clean(img); // Remove the image object
                
        lv_obj_t *label = lv_label_create(parent_cont);
        lv_label_set_text(label, "IMG NOT FOUND: S:/images/tree.bin");
        lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0x00, 0x00), 0);
        lv_obj_center(label);
        Serial.println("Error: Failed to load S:/images/tree.bin");
        return;
    }

    lv_obj_set_size(img, 170, 170);
    lv_obj_set_style_pad_left(img, -15, 0);
    lv_obj_set_style_pad_top(img, -15, 0);

    // Update the global pointer to the NEW image object
    current_image_obj = img;

    // IMPORTANT: Tell the parent container to re-layout its children 
    // (critical for Flex layouts)
    lv_obj_update_layout(parent_cont);

            // [ ... Insert your new image loading error check logic here ... ]
}

/* Configure initial image tab */
lv_obj_t* init_image_display(lv_obj_t *parent, const char* path) {
        lv_obj_t *cont = lv_obj_create(parent);

        lv_obj_set_size(cont, 300, 180);
        lv_obj_center(cont);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_gap(cont, 10, 0);

        lv_obj_set_flex_align(
            cont,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER, 
            LV_FLEX_ALIGN_CENTER  
        );

       
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_size(btn, 60, 60);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, ">");
        lv_obj_center(label);


        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
                lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
                const char *text = lv_label_get_text(lbl);
                Serial.printf("Pressed: %s\n", text);
                 change_image(current_image_obj, "S:/images/tree.bin");
        }, LV_EVENT_CLICKED, NULL);

         lv_obj_t *img = lv_img_create(cont);

        lv_img_set_src(img, path);

        if (lv_img_get_src(img) == NULL) {
            lv_obj_clean(img);
                
            lv_obj_t *label = lv_label_create(cont);
            lv_label_set_text(label, "IMG NOT FOUND: S:/images/tree.bin");
            lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0x00, 0x00), 0);
            lv_obj_center(label);
            Serial.println("Error: Failed to load S:/images/tree.bin");
            return nullptr;
        }

        lv_obj_set_size(img, 170, 170);
        lv_obj_set_style_pad_left(img, -15, 0);
        lv_obj_set_style_pad_top(img, -15, 0);



          current_image_obj = img;  
    return img;
}

static lv_coord_t last_x = -1;
static lv_coord_t last_y = -1;

// --- Drawing Descriptor ---
// Define how the line should look (color, width).
static lv_draw_line_dsc_t line_dsc;

/**
 * @brief Event handler responsible for drawing lines on the canvas based on touch input.
 * * @param e Pointer to the LVGL event structure.
 */
static void canvas_draw_event_cb(lv_event_t * e) {
    lv_obj_t* canvas = lv_event_get_target(e);
    lv_indev_t* indev = lv_indev_get_act();
    lv_event_code_t code = lv_event_get_code(e);

    if (indev == NULL) return;

    if (code == LV_EVENT_PRESSING) {
        lv_point_t current_point;
        
        // 1. Get the current touch coordinates relative to the screen
        lv_indev_get_point(indev, &current_point);
        
        // 2. Translate screen coordinates to canvas-local coordinates
        lv_area_t canvas_area;
        lv_obj_get_coords(canvas, &canvas_area);

        // Calculate the coordinates inside the canvas
        lv_coord_t current_x = current_point.x - canvas_area.x1;
        lv_coord_t current_y = current_point.y - canvas_area.y1;

        if (last_x == -1) {
            // First press point: initialize the last point trackers
            last_x = current_x;
            last_y = current_y;

            // FIX: Declare a local array variable instead of using a compound literal.
            lv_point_t start_point_array[] = {
                {last_x, last_y}, 
                {last_x + 1, last_y} // Draw a short segment (or two points) to make a dot
            };

            lv_canvas_draw_line(
                canvas, 
                start_point_array, // Pass the array (which decays to a pointer)
                2, 
                &line_dsc
            );
        } else {
            // Dragging: Draw a line segment from the last point to the current point
            lv_point_t points[] = {{last_x, last_y}, {current_x, current_y}};

            lv_canvas_draw_line(canvas, points, 2, &line_dsc);

            // Update the last point for the next segment
            last_x = current_x;
            last_y = current_y;
        }

        // Inform LVGL that the canvas area needs to be redrawn
        lv_obj_invalidate(canvas);

    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_LEAVE) {
        // Touch released or cursor left the area: reset the last point to stop drawing
        last_x = -1;
        last_y = -1;
    }
}

/**
 * Initializes and places an LVGL canvas on a parent object, making it ready for drawing.
 *
 * @param parent_obj Pointer to the parent container.
 * @param width The desired width of the canvas in pixels.
 * @param height The desired height of the canvas in pixels.
 * @return A pointer to the newly created canvas object, or NULL on failure.
 */
lv_obj_t* initialize_and_place_canvas(lv_obj_t* parent_obj, lv_coord_t width, lv_coord_t height) {
    // 1. Calculate the required buffer size in bytes
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height);

    // 2. Allocate the buffer dynamically on the heap
    lv_color_t* canvas_buffer = (lv_color_t*) malloc(buf_size);
    
    if (canvas_buffer == nullptr) {
        return nullptr; // Allocation failed
    }

    // 3. Create the canvas object
    lv_obj_t* canvas = lv_canvas_create(parent_obj);
    if (canvas == nullptr) {
        free(canvas_buffer); // Free the buffer if canvas creation failed
        return nullptr;
    }

    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);



    // 4. Set the canvas size and center it
    lv_obj_set_size(canvas, width, height);
    lv_obj_center(canvas);
    
    // Set a border and shadow to make it look like a drawing pad
    lv_obj_set_style_border_color(canvas, lv_color_make(0xDD, 0xDD, 0xDD), 0);
    lv_obj_set_style_border_width(canvas, 2, 0);
    lv_obj_set_style_shadow_width(canvas, 10, 0);
    lv_obj_set_style_shadow_opa(canvas, LV_OPA_30, 0);


    // 5. Initialize the canvas with the buffer and format
    lv_canvas_set_buffer(canvas, canvas_buffer, width, height, LV_IMG_CF_TRUE_COLOR);

    // 6. Set up the line drawing descriptor (e.g., thick blue line)
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_make(0x1E, 0x90, 0xFF); // Dodger Blue
    line_dsc.width = 4;
    line_dsc.round_end = 1;
    line_dsc.round_start = 1;

    // 7. Add the drawing event handler
    lv_obj_add_event_cb(canvas, canvas_draw_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(canvas, canvas_draw_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(canvas, canvas_draw_event_cb, LV_EVENT_LEAVE, NULL); // Stop drawing if cursor leaves area

    // 8. Set an event handler to free the buffer when the canvas is deleted (memory leak prevention)
    lv_obj_add_event_cb(canvas, [](lv_event_t * e) {
        if (lv_event_get_code(e) == LV_EVENT_DELETE) {
            lv_obj_t* target = lv_event_get_target(e);
            
            // Use lv_canvas_get_img to get the descriptor, then access its data field
            const lv_img_dsc_t* img_dsc = lv_canvas_get_img(target);
            
            // The 'data' field of the image descriptor holds the buffer pointer
            lv_color_t* buf_to_free = (lv_color_t*) img_dsc->data;
            
            if (buf_to_free) {
                free(buf_to_free);
                printf("Canvas buffer freed.\n"); // Debug message
            }
        }
    }, LV_EVENT_ALL, NULL);

    // 9. Clear the canvas to white background
    lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

    return canvas;
}


void setup() {
    Serial.begin(115200);
    Serial.println("LVGL Image Tab");

    gfx->begin(80000000);
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    ledcSetup(0, 2000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 255); // Screen brightness (0–255)
#endif
    gfx->fillScreen(BLACK);

    lv_init();
    touch_init();

    screenWidth = gfx->width();
    screenHeight = gfx->height();

    if (!SD.begin()) Serial.println("SD Card Mount Failed");
    else Serial.println("SD Card Mounted successfully!");

    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    fs_drv.letter = LVGL_SD_DRIVE_LETTER; // 'S'
    fs_drv.cache_size = 0; 
    
    // Register the wrapper functions as handlers
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;
    lv_fs_drv_register(&fs_drv);
    Serial.println("LVGL File System Driver Registered.");

#ifdef ESP32
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(
        sizeof(lv_color_t) * screenWidth * screenHeight / 2,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    disp_draw_buf = (lv_color_t *)malloc(
        sizeof(lv_color_t) * screenWidth * screenHeight / 2);
#endif
    if (!disp_draw_buf) {
        Serial.println("LVGL disp_draw_buf allocate failed!");
    } else {
        lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 2);

        // Register display driver
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = screenWidth;
        disp_drv.ver_res = screenHeight;
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        // Register touch input
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);

        // Create TabView
        lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);

        // Add 4 tabs
        lv_obj_t *tab0 = lv_tabview_add_tab(tabview, "canvas");
        lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "a-h");
        lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "i-p");
        lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "q-x");

        // Helper: add Hebrew letter buttons
        auto add_letter_buttons = [](lv_obj_t *parent, const char *letters[], int count) {
            lv_obj_t *cont = lv_obj_create(parent);
            lv_obj_set_size(cont, 300, 180);
            lv_obj_center(cont);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_style_pad_gap(cont, 10, 0);

            for (int i = 0; i < count; i++)
            {
                lv_obj_t *btn = lv_btn_create(cont);
                lv_obj_set_size(btn, 60, 60);

                lv_obj_t *label = lv_label_create(btn);
                lv_label_set_text(label, letters[i]);
                lv_obj_center(label);

                // Print pressed letter to Serial
                lv_obj_add_event_cb(btn, [](lv_event_t *e) {
                    lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
                    const char *text = lv_label_get_text(lbl);
                    Serial.printf("Pressed: %s\n", text);
                }, LV_EVENT_CLICKED, NULL);
            }
    };

    
    lv_obj_t* scroll_container = lv_obj_get_child(tab0, 0); 

    if (scroll_container != nullptr) {
        // 4. Clear the flag responsible for receiving input gestures (swiping)
        lv_obj_clear_flag(scroll_container, LV_OBJ_FLAG_GESTURE_BUBBLE);
        
        // 5. You can also explicitly disable the scroll direction on this container
        lv_obj_set_scroll_dir(scroll_container, LV_DIR_VER); // Only allow vertical scrolling if needed, or NONE
        
        // OR, even more strongly:
        lv_obj_clear_flag(scroll_container, LV_OBJ_FLAG_SCROLLABLE); 
    }

    // Letters for each tab
    const char *letters1[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    const char *letters2[] = {"i", "j", "k", "l", "m", "n", "o", "p"};
    const char *letters3[] = {"q", "r", "s", "t", "u", "v", "w", "x"};

    // Intialize tabs
    //init_image_display(tab0, "S:/images/bus.bin");

    // Create the canvas
    lv_obj_t* my_canvas = initialize_and_place_canvas(tab0, 120, 120);
    lv_obj_add_flag(my_canvas, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_CLICKABLE);

    add_letter_buttons(tab1, letters1, 8);
    add_letter_buttons(tab2, letters2, 8);
    add_letter_buttons(tab3, letters3, 8);

     Serial.println("Setup done"); 
    }
}

void loop() {
    lv_timer_handler(); // let LVGL run
    delay(5);
}