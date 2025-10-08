/*******************************************************************************
 * Intoducing the Game class through which eventually all mechanics will work
 * with the use of an instance of the class:      Game *game = new Game(...);
 * we controle such things as:
 * - what image is now shown?
 * - what's the first character of the image?
 * - processing letter presses to check if correct
 ******************************************************************************/

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <vector>
#include <string>
#include "FS.h"
#include "SD.h"
#include "driver_handlers.h"


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

#include <vector>
#include <string>
#include "lvgl.h" 


void init_sd_card() {
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

    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

std::vector<std::string> read_directory_file_list() {
    std::vector<std::string> file_list;
    lv_fs_dir_t dir;
    lv_fs_res_t res;
    
    // LV_FS_MAX_FN_LENGTH is usually defined in lv_conf.h
    // We use a safe local buffer size based on typical SD library path limits
    char fn[256]; 

    // 1. Open the directory
    res = lv_fs_dir_open(&dir, "S:/images");
    if (res != LV_FS_RES_OK) {
        Serial.print("Error opening directory");
        return file_list; // Return empty list on failure
    }

    Serial.print("Reading contents of S:/images/");

    // 2. Read directory entries until the end is reached
    while (true) {
        res = lv_fs_dir_read(&dir, fn);
        
        // Check for read error
        if (res != LV_FS_RES_OK) {
            Serial.print("Error reading directory entry.");
            break;
        }

        // Check for end of list (LVGL convention: fn[0] == '\0')
        if (fn[0] == '\0') {
            break; 
        }

        if (strcmp(fn, ".") != 0 && 
            strcmp(fn, "..") != 0 && 
            fn[0] != '/' && 
            strncmp(fn, "._", 2) != 0) { // <-- NEW FILTERING LOGIC
            
            file_list.push_back(std::string(fn));
            Serial.print("  Found: ");
            Serial.println(fn);
        }
    }

    // 4. Close the directory
    lv_fs_dir_close(&dir);

    Serial.print("Finished reading directory. Total files found: ");
    Serial.println(file_list.size());
    
    return file_list;
}

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


class Game {
public:
    Game(lv_obj_t *img) : current_image_obj_m(img), image_pos_m(-1) {
       //image_name_list_m = {"tree", "bus", "cat"};
       for (int i = 0; i < 10; ++i) {
            image_name_list_m = read_directory_file_list();
            if (!image_name_list_m.empty()) break;
       }
    }

    std::string get_path() {
        if (image_pos_m == -1) return "";
        return "S:/images/" + image_name_list_m[image_pos_m];// + ".bin";
    }

    char get_first_letter() {
        if(image_pos_m == -1) return 0;
        return image_name_list_m[image_pos_m][0];
    }

    lv_obj_t *get_current_image() {
        return current_image_obj_m;
    }

   bool process(char c) {
        if(c != get_first_letter()) {
            Serial.println("No");
            return false;
        }
        Serial.println("Yes");
        image_pos_m = (image_pos_m + 1) % image_name_list_m.size();
       
        return true;
   }

   void skip() {
        image_pos_m = (image_pos_m + 1) % image_name_list_m.size();
   }

    void set(int new_pos) {
        image_pos_m = new_pos;
    }

private:
    lv_obj_t *current_image_obj_m;
    int image_pos_m;
    std::vector<std::string> image_name_list_m;
};

Game* game; 

    /* Configure initial image tab */
lv_obj_t* init_image_display(lv_obj_t* parent, const char* path) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    lv_obj_set_size(cont, 300, 180);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(cont, 10, 0);

    lv_obj_set_flex_align( cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER  );

        
    lv_obj_t *btn = lv_btn_create(cont);
    lv_obj_set_size(btn, 60, 60);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, ">");
    lv_obj_center(label);


    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
            const char *text = lv_label_get_text(lbl);
            Serial.printf("Pressed: %s\n", text);
            game->skip();
            change_image(current_image_obj, game->get_path().c_str());
    }, LV_EVENT_CLICKED, NULL);

        lv_obj_t *img = lv_img_create(cont);

    lv_img_set_src(img, path);

    if (lv_img_get_src(img) == NULL) {
        lv_obj_clean(img);
                
        lv_obj_t *label = lv_label_create(cont);
        lv_label_set_text(label, "IMG NOT FOUND");
        lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0x00, 0x00), 0);
        lv_obj_center(label);
        Serial.println("Error: Failed to load");
        return nullptr;
    }
    /*
    lv_obj_set_size(img, 170, 170);
    lv_obj_set_style_pad_left(img, -15, 0);
    lv_obj_set_style_pad_top(img, -15, 0);
*/
    current_image_obj = img;  
    return img;
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
        lv_label_set_text(label, "IMG NOT FOUND");
        lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0x00, 0x00), 0);
        lv_obj_center(label);
        Serial.println("Error: Failed to load");
        return;
    }
/*
    lv_obj_set_size(img, 170, 170);
    lv_obj_set_style_pad_left(img, -15, 0);
    lv_obj_set_style_pad_top(img, -15, 0);
*/
    // Update the global pointer to the NEW image object
    current_image_obj = img;

    // IMPORTANT: Tell the parent container to re-layout its children 
    // (critical for Flex layouts)
    lv_obj_update_layout(parent_cont);

            // [ ... Insert your new image loading error check logic here ... ]
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

    Serial.println("beginning initialization\n");

    lv_init();
    touch_init();
    init_sd_card();


    screenWidth = gfx->width();
    screenHeight = gfx->height();

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
        lv_obj_t *content = lv_tabview_get_content(tabview);
        // Clear the scrollable flag on the content area to prevent swiping
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
    
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
        // Add 4 tabs
        lv_obj_t *tab0 = lv_tabview_add_tab(tabview, "img");
        lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "a-h");
        lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "i-p");
        lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "q-x");
        lv_obj_clear_flag(tab0, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
        lv_obj_clear_flag(tab1, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
        lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
        lv_obj_clear_flag(tab3, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

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
                    if(game->process(text[0])) {
                        change_image(current_image_obj, game->get_path().c_str());
                    }
                }, LV_EVENT_CLICKED, NULL);
            }
    };



    // Letters for each tab
    const char *letters1[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    const char *letters2[] = {"i", "j", "k", "l", "m", "n", "o", "p"};
    const char *letters3[] = {"q", "r", "s", "t", "u", "v", "w", "x"};

    // Intialize tabs


    //delay(5000);

    game = new Game(current_image_obj);
    game->set(0);
    init_image_display(tab0, game->get_path().c_str());
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