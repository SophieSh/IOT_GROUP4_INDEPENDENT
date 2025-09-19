#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <TAMC_GT911.h>

#define CANVAS_WIDTH  100
#define CANVAS_HEIGHT 100

// ----------------- Display -----------------
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[CANVAS_WIDTH* 20]; // buffer

// ----------------- Touch (GT911) -----------------
#define SDA_PIN 21
#define SCL_PIN 22
#define RST_PIN 25
#define INT_PIN 26
TAMC_GT911 ts(SDA_PIN, SCL_PIN, RST_PIN, INT_PIN, CANVAS_WIDTH, CANVAS_HEIGHT); // width, height

// ----------------- LVGL Display Driver -----------------
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// ----------------- LVGL Touch Driver -----------------
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
  ts.read();   // refresh touch data from GT911

  if (ts.isTouched) {               // property, not function
    TP_Point p = ts.points[0];      // first touch point
    data->state = LV_INDEV_STATE_PR;
    data->point.x = p.x;
    data->point.y = p.y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}


// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);

  // Init display
  tft.begin();
  tft.setRotation(1);

  // Init touch
  Wire.begin(SDA_PIN, SCL_PIN);
  ts.begin();

  // Init LVGL
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, CANVAS_WIDTH * 20);

  // Register display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = CANVAS_HEIGHT;
  disp_drv.ver_res = CANVAS_WIDTH;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Register touch driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // ----------------- Create a canvas -----------------
  lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
  static lv_color_t buf[CANVAS_WIDTH * CANVAS_HEIGHT];
  lv_canvas_set_buffer(canvas, buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
  lv_obj_center(canvas);

  // Fill with white
  lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

  // Draw a red rectangle
  lv_draw_rect_dsc_t rect_dsc;
  lv_draw_rect_dsc_init(&rect_dsc);
  rect_dsc.bg_color = lv_palette_main(LV_PALETTE_RED);
  lv_canvas_draw_rect(canvas, 20, 20, 100, 50, &rect_dsc);

  // Draw text
  lv_draw_label_dsc_t label_dsc;
  lv_draw_label_dsc_init(&label_dsc);
  label_dsc.color = lv_color_black();
  lv_canvas_draw_text(canvas, 10, 90, 180, &label_dsc, "Hello Canvas!");
}

void loop() {
  lv_timer_handler(); 
  delay(5);
}
