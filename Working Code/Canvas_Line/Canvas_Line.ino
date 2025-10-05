/*******************************************************************************
 * LVGL Canvas Widget, drawing a thick red line
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <stdlib.h> 
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
    lv_obj_t *parent_cont = lv_obj_get_parent(img_obj);

    lv_obj_del(img_obj);
            
    lv_obj_t *img = lv_img_create(parent_cont);

    lv_img_set_src(img, new_path);


    if (lv_img_get_src(img) == NULL) {
        lv_obj_clean(img); 
                
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

    current_image_obj = img;


    lv_obj_update_layout(parent_cont);
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


lv_obj_t* initialize_and_place_canvas(lv_obj_t* parent_obj, lv_coord_t width, lv_coord_t height) {
     size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height);

    lv_color_t* canvas_buffer = (lv_color_t*) malloc(buf_size);
    
    if (canvas_buffer == NULL) {
        return NULL; // Allocation failed
    }

    lv_obj_t* canvas = lv_canvas_create(parent_obj);
    if (canvas == NULL) {
        free(canvas_buffer); // Free the buffer if canvas creation failed
        return NULL;
    }

    lv_obj_set_size(canvas, width, height);

    lv_canvas_set_buffer(canvas, canvas_buffer, width, height, LV_IMG_CF_TRUE_COLOR);

    lv_obj_add_event_cb(canvas, [](lv_event_t * e) {
        if (lv_event_get_code(e) == LV_EVENT_DELETE) {
            lv_obj_t* target = lv_event_get_target(e);
            
            const lv_img_dsc_t* img_dsc = lv_canvas_get_img(target);
            
            lv_color_t* buf_to_free = (lv_color_t*) img_dsc->data;
            
            if (buf_to_free) {
                free(buf_to_free);
            }
        }
    }, LV_EVENT_ALL, NULL); // Using LV_EVENT_ALL for simplicity

    // 7. Optional: Center and clear the canvas
    lv_obj_center(canvas);
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
        lv_obj_t *tab0 = lv_tabview_add_tab(tabview, "img");
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



    // Letters for each tab
    const char *letters1[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    const char *letters2[] = {"i", "j", "k", "l", "m", "n", "o", "p"};
    const char *letters3[] = {"q", "r", "s", "t", "u", "v", "w", "x"};

    // Create the canvas
    lv_obj_t* my_canvas = initialize_and_place_canvas(tab0, 170, 170);

    if (my_canvas != nullptr) {
        // Define the points
        lv_point_t p1 = {10, 10};
        lv_point_t p2 = {140, 90};

        // Create an array containing the points to connect (just two for a single line)
        lv_point_t line_points[] = {p1, p2}; 
        uint32_t point_count = 2; // We have 2 points in the array

        // Initialize the descriptor (assuming you did this correctly)
        lv_draw_line_dsc_t dsc;
        lv_draw_line_dsc_init(&dsc);
        dsc.color = lv_color_make(0xFF, 0x00, 0x00); // Red
        dsc.width = 5;

// Correct function call
lv_canvas_draw_line(my_canvas, line_points, point_count, &dsc);

        // Ensure the canvas is redrawn (though often automatic)
        lv_obj_invalidate(my_canvas);
    }

    // Intialize tabs
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