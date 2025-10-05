/*******************************************************************************
 * LVGL Widgets
 * This is a widgets demo for LVGL - Light and Versatile Graphics Library
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>

#define TFT_BL 27
#define GFX_BL DF_GFX_BL // default backlight pin

/* Display configuration */
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 15 /* CS */, 14 /* SCK */, 13 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, -1 /* RST */, 3 /* rotation */, true /* IPS */);

/* Touch include */
#include "touch.h"

/* Change to your screen resolution */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
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
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touch_has_signal())
    {
        if (touch_touched())
        {
            data->state = LV_INDEV_STATE_PR;
            /*Set the coordinates*/
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
        }
        else if (touch_released())
        {
            data->state = LV_INDEV_STATE_REL;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

// -----------------------------------------------------------------------------
// TIMER UI LOGIC START 
// -----------------------------------------------------------------------------

static void set_timer_value(void *bar, int32_t v)
{
    lv_bar_set_value(bar, v, LV_ANIM_OFF); 
}


/**
 * @brief Event handler to change the bar's indicator color based on the remaining time.
 * Logic: Green ( > 2/3), Yellow ( > 1/3), Red ( <= 1/3)
 */
static void timer_color_event_cb(lv_event_t * e) {
    // 1. Get the bar object that triggered the event
    lv_obj_t * bar = lv_event_get_target(e); 
    
    // 2. Get the current and maximum values (e.g., current value 30 down to 0, max value 30)
    int32_t current_value = lv_bar_get_value(bar);
    int32_t max_value = lv_bar_get_max_value(bar);
    
    // Calculate the thresholds dynamically (using integer division)
    int32_t one_third = max_value / 3;
    int32_t two_thirds = (max_value * 2) / 3;
    
    lv_color_t new_color;

    if (current_value > two_thirds) {
        // Green zone: Time remaining is high (e.g., 21-30 seconds left)
        new_color = lv_color_make(0, 180, 0); // Darker Green
    } else if (current_value > one_third) {
        // Yellow zone: Time remaining is medium (e.g., 11-20 seconds left)
        new_color = lv_color_make(255, 255, 0); // Yellow
    } else {
        // Red zone: Time remaining is low (e.g., 0-10 seconds left)
        new_color = lv_color_make(255, 0, 0); // Red
    }

    // 3. Apply the new color style to the indicator part of the bar
    // LV_PART_INDICATOR ensures we only color the filled part
    lv_obj_set_style_bg_color(bar, new_color, LV_PART_INDICATOR);
}

/**
 * @brief Custom drawing handler to display the bar's current value (seconds) on the indicator.
 */
static void timer_display_event_cb(lv_event_t * e)
{
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

    // 5. Vertically center the text within the bar's height
    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    // 6. Execute the drawing command
    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

void create_timer_bar(int duration_seconds) {
    
    // --- PART 1: OBJECT CREATION & LAYOUT ---
    lv_obj_t* timer_bar = lv_bar_create(lv_scr_act());  // Create the bar object on the active screen (child of the Screen)
    lv_bar_set_range(timer_bar, 0, duration_seconds);
    lv_bar_set_value(timer_bar, duration_seconds, LV_ANIM_OFF); //// Set the initial value to the maximum (full bar)
    lv_obj_set_size(timer_bar, 250, 30); // size in pixels
    
    // Position it in the center of the screen
    lv_obj_center(timer_bar); 
    
    // --- PART 2: ANIMATION SETUP ---

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
    lv_obj_add_event_cb(timer_bar, timer_color_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(timer_bar, timer_display_event_cb, LV_EVENT_DRAW_PART_END, NULL);
}

// -----------------------------------------------------------------------------
// TIMER UI LOGIC END
// -----------------------------------------------------------------------------



void setup()
{
    Serial.begin(115200);
    Serial.println("LVGL Tabview Demo");

    // Init Display
    gfx->begin(80000000);
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    ledcSetup(0, 2000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 255); /* Screen brightness can be modified by adjusting this parameter. (0-255) */
#endif
    gfx->fillScreen(BLACK);

    lv_init();
    
    touch_init();
    
    screenWidth = gfx->width();
    screenHeight = gfx->height();

#ifdef ESP32
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight/2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight/2);
#endif
    if (!disp_draw_buf)
    {
        Serial.println("LVGL disp_draw_buf allocate failed!");
    }
    else
    {
        lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight/2);

        /* Initialize the display */
        lv_disp_drv_init(&disp_drv);
        /* Change the following line to your display resolution */
        disp_drv.hor_res = screenWidth;
        disp_drv.ver_res = screenHeight;
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        /* Initialize the (dummy) input device driver */
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);
        
        create_timer_bar(30); 
        
        Serial.println("Setup done");
    }
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}