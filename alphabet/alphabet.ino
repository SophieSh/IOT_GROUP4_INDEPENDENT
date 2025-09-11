/*******************************************************************************
 * LVGL Widgets
 * Hebrew Alphabet Tabs Example
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

/* Screen and LVGL buffers */
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

void setup()
{
    Serial.begin(115200);
    Serial.println("LVGL Hebrew Alphabet Tabs");

    // Init Display
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

#ifdef ESP32
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(
        sizeof(lv_color_t) * screenWidth * screenHeight / 2,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    disp_draw_buf = (lv_color_t *)malloc(
        sizeof(lv_color_t) * screenWidth * screenHeight / 2);
#endif
    if (!disp_draw_buf)
    {
        Serial.println("LVGL disp_draw_buf allocate failed!");
    }
    else
    {
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

        // === Create TabView ===
        lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);

        // Add 3 tabs
        lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "א–ח");
        lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "ט–ע");
        lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "פ–ת");

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
                },
                                    LV_EVENT_CLICKED, NULL);
            }
        };

        // Hebrew letters for each tab
        const char *letters1[] = {"א", "ב", "ג", "ד", "ה", "ו", "ז", "ח"};
        const char *letters2[] = {"ט", "י", "כ", "ל", "מ", "נ", "ס", "ע"};
        const char *letters3[] = {"פ", "צ", "ק", "ר", "ש", "ת"};

        // Add them
        add_letter_buttons(tab1, letters1, 8);
        add_letter_buttons(tab2, letters2, 8);
        add_letter_buttons(tab3, letters3, 6);

        Serial.println("Setup done");
    }
}

void loop()
{
    lv_timer_handler(); // let LVGL run
    delay(5);
}
