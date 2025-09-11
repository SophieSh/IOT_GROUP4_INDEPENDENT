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

void create_controls_for_tab(lv_obj_t* parent, const char* btn1_text, const char* btn2_text) {
    // Calculate positions for side-by-side buttons
    const int button_width = 120;
    const int button_height = 50;
    const int button_spacing = 20;  // Space between buttons
    const int total_width = (button_width * 2) + button_spacing;
    const int start_x = -(total_width / 2) + (button_width / 2);  // Center the button group

    // Create first button
    lv_obj_t* btn1 = lv_btn_create(parent);
    lv_obj_set_size(btn1, button_width, button_height);
    lv_obj_align(btn1, LV_ALIGN_TOP_MID, start_x, 20);  // Position from center, offset left
    lv_obj_t* label1 = lv_label_create(btn1);
    lv_label_set_text(label1, btn1_text);
    lv_obj_center(label1);

    // Create second button
    lv_obj_t* btn2 = lv_btn_create(parent);
    lv_obj_set_size(btn2, button_width, button_height);
    lv_obj_align(btn2, LV_ALIGN_TOP_MID, start_x + button_width + button_spacing, 20);  // Position from center, offset right
    lv_obj_t* label2 = lv_label_create(btn2);
    lv_label_set_text(label2, btn2_text);
    lv_obj_center(label2);

    // Create first slider and its label
    lv_obj_t* slider1 = lv_slider_create(parent);
    lv_obj_set_size(slider1, 200, 10);
    lv_obj_align(slider1, LV_ALIGN_TOP_MID, 0, 90);  // Adjusted Y position after buttons
    lv_obj_t* slider1_label = lv_label_create(parent);
    lv_label_set_text(slider1_label, "0");
    lv_obj_align_to(slider1_label, slider1, LV_ALIGN_OUT_TOP_MID, 0, -5);

    // Create second slider and its label
    lv_obj_t* slider2 = lv_slider_create(parent);
    lv_obj_set_size(slider2, 200, 10);
    lv_obj_align(slider2, LV_ALIGN_TOP_MID, 0, 140);  // Adjusted Y position
    lv_obj_t* slider2_label = lv_label_create(parent);
    lv_label_set_text(slider2_label, "0");
    lv_obj_align_to(slider2_label, slider2, LV_ALIGN_OUT_TOP_MID, 0, -5);

    // Event handlers remain the same
    lv_obj_add_event_cb(btn1, [](lv_event_t* e) {
        uint32_t tab_num = (uint32_t)lv_obj_get_index(lv_obj_get_parent(lv_event_get_target(e))) + 1;
        Serial.printf("Button 1 pressed in tab %d\n", tab_num);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn2, [](lv_event_t* e) {
        uint32_t tab_num = (uint32_t)lv_obj_get_index(lv_obj_get_parent(lv_event_get_target(e))) + 1;
        Serial.printf("Button 2 pressed in tab %d\n", tab_num);
    }, LV_EVENT_CLICKED, NULL);

    // Slider event handler
    auto slider_event_cb = [](lv_event_t* e) {
        lv_obj_t* slider = lv_event_get_target(e);
        lv_obj_t* label = (lv_obj_t*)lv_event_get_user_data(e);
        uint32_t tab_num = (uint32_t)lv_obj_get_index(lv_obj_get_parent(slider)) + 1;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", (int)lv_slider_get_value(slider));
        lv_label_set_text(label, buf);
        Serial.printf("Slider changed to %d in tab %d\n", 
            (int)lv_slider_get_value(slider),
            tab_num);
    };

    lv_obj_add_event_cb(slider1, slider_event_cb, LV_EVENT_VALUE_CHANGED, slider1_label);
    lv_obj_add_event_cb(slider2, slider_event_cb, LV_EVENT_VALUE_CHANGED, slider2_label);
}


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

        // Create a tab view object
        lv_obj_t* tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);

        // Add tabs
        lv_obj_t* tab1 = lv_tabview_add_tab(tabview, "Tab 1");
        lv_obj_t* tab2 = lv_tabview_add_tab(tabview, "Tab 2");

        // Create controls for both tabs
        create_controls_for_tab(tab1, "Tab1 Btn1", "Tab1 Btn2");
        create_controls_for_tab(tab2, "Tab2 Btn1", "Tab2 Btn2");

        Serial.println("Setup done");
    }
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}