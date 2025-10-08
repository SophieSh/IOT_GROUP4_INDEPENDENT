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
    Game(lv_obj_t *img, lv_obj_t *timer) : current_image_obj_m(img), image_pos_m(0), timer_bar_m(timer) {
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

    void start_timer_animation(lv_obj_t* timer_bar); 

    bool process(char c) {
        if(c != get_first_letter()) {
            Serial.println("No");
            return false;
        }
        Serial.println("Yes");
        image_pos_m = (image_pos_m + 1) % image_name_list_m.size();
       
        if (timer_bar_m) {
            start_timer_animation(timer_bar_m); 
        }

        return true;
    }

   void skip() {
        image_pos_m = (image_pos_m + 1) % image_name_list_m.size();
   }

private:
    lv_obj_t *current_image_obj_m;
    lv_obj_t *timer_bar_m;
    int image_pos_m;
    std::vector<std::string> image_name_list_m;
};

Game* game; 

    /* Configure initial image tab */
lv_obj_t* init_image_display(lv_obj_t* parent, const char* path) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    lv_obj_set_size(cont, 300, 155);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(cont, 10, 0);
    lv_obj_set_flex_align( cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER  );

    lv_obj_set_style_bg_color(cont, lv_color_hex(0x003773), 0); // Dark Blue Gray
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0); 

        
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

    // Update the global pointer to the NEW image object
    current_image_obj = img;

    // IMPORTANT: Tell the parent container to re-layout its children 
    // (critical for Flex layouts)
    lv_obj_update_layout(parent_cont);
}

// -----------------------------------------------------------------------------
// TIMER UI LOGIC START 
// -----------------------------------------------------------------------------


/**
 * @brief Event handler to change the bar's indicator color based on the remaining time.
 * Logic: Green ( > 2/3), Yellow ( > 1/3), Red ( <= 1/3)
 */

 static void on_timer_timeout(lv_anim_t * a) {
    Serial.println("!!! TIME IS UP! Moving to next image. !!!");
    
    game->skip();
    change_image(current_image_obj, game->get_path().c_str());

    // 3. Restart the timer for the new challenge
    // 'a->var' holds the pointer to the lv_bar_t object
    game->start_timer_animation((lv_obj_t*)a->var); 
}


static void update_timer_color(lv_obj_t * bar) {
    int32_t current_value = lv_bar_get_value(bar);
    int32_t max_value = lv_bar_get_max_value(bar);
    
    int32_t one_third = max_value / 3;
    int32_t two_thirds = (max_value * 2) / 3;
    
    lv_color_t new_color;

    if (current_value > two_thirds)
        new_color = lv_color_make(0, 180, 0); // Green
    else if (current_value > one_third)
        new_color = lv_color_make(255, 255, 0); // Yellow
    else
        new_color = lv_color_make(255, 0, 0); // Red


    lv_obj_set_style_bg_color(bar, new_color, LV_PART_INDICATOR);
}

static void set_timer_value(void *bar, int32_t v)
{
    // 1. Explicitly cast the void* to the object pointer (lv_obj_t*).
    lv_obj_t* bar_obj = (lv_obj_t *)bar;
    
    // 2. Set the bar value using the cast pointer.
    lv_bar_set_value(bar_obj, v, LV_ANIM_OFF); 
    
    // 3. Call the color update function using the cast pointer.
    update_timer_color(bar_obj);
}


/**
 * @brief Custom drawing handler to display the bar's current value (seconds) on the indicator.
 */
static void timer_display_event_cb(lv_event_t * e) {
    // Get parameters related to the drawing process
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    
    // Check if the current drawing event is for the INDICATOR part of the bar
    if(dsc->part != LV_PART_INDICATOR) return;

    // Get the object (the bar itself)
    lv_obj_t * obj= lv_event_get_target(e);

    // 1. Setup text drawing styles (font, color)
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT; // Use the default LVGL font

    // 2. Convert the bar's current value (seconds remaining) to a string
    char buf[8];
    // lv_bar_get_value(obj) reads the seconds remaining (e.g., 30 down to 0)
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));


    // 3. Measure the dimensions of the text string
    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);

    // 4. Determine the text position (inside or outside the filled area)
    lv_area_t txt_area;
    
    /* If the indicator is wide enough, put the text inside on the right */
    if(lv_area_get_width(dsc->draw_area) > txt_size.x + 20) {
        // Position inside the indicator area, 5 pixels from the right edge
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white(); // Text color is white for contrast inside the bar
    }
    /* Otherwise, put the text outside to the right of the bar */
    else {
        // Position outside the indicator area, 5 pixels from the bar's right edge
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black(); // Text color is black for contrast outside the bar
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

void Game::start_timer_animation(lv_obj_t* timer_bar) {
    // Stop any existing animation on this object first to prevent conflicts
    lv_anim_del(timer_bar, NULL);
    
    int duration_seconds = lv_bar_get_max_value(timer_bar);
    
    // Set bar to max value (full) before starting the animation
    lv_bar_set_value(timer_bar, duration_seconds, LV_ANIM_OFF); 

    lv_anim_t a; 
    lv_anim_init(&a); 
    lv_anim_set_var(&a, timer_bar); 
    
    lv_anim_set_values(&a, duration_seconds, 0); 
    lv_anim_set_time(&a, (uint32_t)duration_seconds * 1000); 
    
    lv_anim_set_exec_cb(&a, set_timer_value); 
    lv_anim_set_playback_time(&a, 0); 
    lv_anim_set_repeat_count(&a, 0); 

    // --- NEW: Set the callback for when the animation is finished (time is up) ---
    lv_anim_set_ready_cb(&a, on_timer_timeout); 

    lv_anim_start(&a);
}

lv_obj_t *create_timer_bar(int duration_seconds, lv_obj_t *parent) {
    lv_obj_t* timer_bar_cont = lv_obj_create(parent);
    lv_obj_set_size(timer_bar_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_clear_flag(timer_bar_cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
    
    // Make the container look invisible (no background, border, padding gap)
    lv_obj_set_style_pad_gap(timer_bar_cont, 0, 0);
    lv_obj_set_style_radius(timer_bar_cont, 0, 0); 
    lv_obj_set_style_border_width(timer_bar_cont, 0, 0); 
    lv_obj_set_style_bg_opa(timer_bar_cont, LV_OPA_TRANSP, 0); 
    
    // APPLY the desired 10px PADDING *only* to the sides and bottom of this container
    // This creates the margin around the bar without affecting the tabview.
    lv_obj_set_style_pad_left(timer_bar_cont, 7, 0);
    lv_obj_set_style_pad_right(timer_bar_cont, 7, 0);
    lv_obj_set_style_pad_bottom(timer_bar_cont, 7, 0);
    lv_obj_set_style_pad_top(timer_bar_cont, 7, 0); // Use parent gap for top spacing

    lv_obj_set_style_bg_color(timer_bar_cont, lv_color_hex(0x504DB3), 0); // Dark Blue Gray
    lv_obj_set_style_bg_opa(timer_bar_cont, LV_OPA_COVER, 0);

    // --- PART 1: OBJECT CREATION & LAYOUT ---
    lv_obj_t* timer_bar = lv_bar_create(timer_bar_cont);  
    lv_bar_set_range(timer_bar, 0, duration_seconds);
    lv_obj_set_size(timer_bar, LV_PCT(100), 20); // size in pixels
    

    // Position it in the center of the screen
    lv_obj_center(timer_bar); 
    
    lv_obj_set_style_pad_all(timer_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(timer_bar, 0, LV_PART_MAIN);

    

    lv_anim_t a; //Creates a structure (a) to hold all the animation settings.
    lv_anim_init(&a); 
    lv_anim_set_var(&a, timer_bar); // Set the target object for the animation
    
    // 💡 VALUE RANGE: Counts from duration_seconds down to 0
    lv_anim_set_values(&a, duration_seconds, 0); 
    
    // 💡 TIME: Duration in milliseconds (e.g., 30 * 1000 = 30000 ms)
    lv_anim_set_time(&a, (uint32_t)duration_seconds * 1000); 
    
    lv_anim_set_exec_cb(&a, set_timer_value); // Set the callback function to be called at every animation step
    lv_anim_set_playback_time(&a, 0); // No reverse playback
    lv_anim_set_repeat_count(&a, 0);  // Run only once  

    lv_anim_start(&a);


    // --- PART 3: DYNAMIC COLOR HANDLING ---
    lv_obj_add_event_cb(timer_bar, timer_display_event_cb, LV_EVENT_DRAW_PART_END, NULL);

    return timer_bar;
}

// -----------------------------------------------------------------------------
// TIMER UI LOGIC END
// -----------------------------------------------------------------------------



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
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

        lv_obj_t *main_cont = lv_obj_create(lv_scr_act());
        lv_obj_set_size(main_cont, screenWidth, screenHeight);
        lv_obj_set_style_pad_all(main_cont, 0, 0);
        lv_obj_set_style_pad_gap(main_cont, 0, 0); // Gap between tabview and timer bar
        lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_width(main_cont, 0, 0); 

        // Create TabView
        lv_obj_t *tabview = lv_tabview_create(main_cont, LV_DIR_TOP, 30);
        lv_obj_t *content = lv_tabview_get_content(tabview);
        // Clear the scrollable flag on the content area to prevent swiping
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
    
        lv_obj_set_size(tabview, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(tabview, 1); 

        // Add 4 tabs
        lv_obj_t *tab0 = lv_tabview_add_tab(tabview, "img");
        lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "a-h");
        lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "i-p");
        lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "q-x");

        auto setup_tab_layout = [](lv_obj_t* tab) {
            lv_obj_set_style_bg_color(tab, lv_color_hex(0x504DB3), 0); // Dark Blue Gray
            lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0); // Ensure the color is visible
            
            lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
        };

        setup_tab_layout(tab0);
        setup_tab_layout(tab1);
        setup_tab_layout(tab2);
        setup_tab_layout(tab3);



        // Helper: add Hebrew letter buttons
        auto add_letter_buttons = [](lv_obj_t *parent, const char *letters[], int count) {
            lv_obj_t *cont = lv_obj_create(parent);
            lv_obj_set_size(cont, 300, 150);
            lv_obj_center(cont);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_style_pad_gap(cont, 10, 0);

            lv_obj_set_style_bg_color(cont, lv_color_hex(0x003773), 0); // Dark Blue Gray
            lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(cont, 0, 0); 



            static lv_style_t button_style;
            lv_style_init(&button_style);
            
            lv_style_set_bg_color(&button_style, lv_color_hex(0x59BDB8));
            lv_style_set_bg_opa(&button_style, LV_OPA_COVER);
            
            // Set the color for the pressed state (LV_STATE_PRESSED)
            static lv_style_t pressed_style;
            lv_style_init(&pressed_style);
            lv_style_set_bg_color(&pressed_style, lv_color_hex(0x449490));
    


            static lv_style_t button_label_style;
            lv_style_init(&button_label_style);
            lv_style_set_text_font(&button_label_style, &lv_font_montserrat_32); 
            lv_style_set_text_color(&button_label_style, lv_color_hex(0x000000));

            for (int i = 0; i < count; i++) {
                lv_obj_t *btn = lv_btn_create(cont);
                lv_obj_set_size(btn, 60, 55);

                lv_obj_add_style(btn, &button_style, LV_PART_MAIN);
                lv_obj_add_style(btn, &pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);

                lv_obj_t *label = lv_label_create(btn);
                lv_label_set_text(label, letters[i]);
                lv_obj_center(label);
                lv_obj_add_style(label, &button_label_style, 0); 

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


    lv_obj_t *timer_bar = create_timer_bar(30, main_cont);
    game = new Game(current_image_obj, timer_bar);
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