/*******************************************************************************
 *        ~- Full Project V2 -~
 * All functionallity from V1 and more
 * Keyboard for access to all alphabet
 * Popup timeout/fail/success messages
 * Symbol icons - for illiterate users
 * Wifi, IP & server TCP communication
 * Draw-By-Touch canvas with clear_btn
 * Send canvas to a server - check_btn
 * Play buzzer melody for fail/success
 ******************************************************************************/

// Includes
#include <lvgl.h> 
#include <Arduino_GFX_Library.h> // For touch controls
#include <vector>                // For image_name_list
#include <string>                // Names in: S:/images
#include "FS.h"                  // For driver_handlers
#include "SD.h"                  // Micro-SD-Card reads
#include "driver_handlers.h"     // Drivers for SD card
#include "timer_bar.h"           // Timer progress bars
#include "sound.h"               // Victory/loss buzzes
#include <WiFi.h>                // For wifi connection
#include <WiFiClient.h>          // Server communication


#define TFT_BL 27       
#define GFX_BL DF_GFX_BL
#define QUIZ_DURATION_SECONDS 30 // Time to submit answer
#define BL_CHANNEL 0


// Display configuration
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 15 /* CS */, 14 /* SCK */, 13 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, -1 /* RST */, 3 /* rotation */, true /* IPS */);

#include "touch.h"

// Screen and LVGL buffers
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

// Current image challange
static lv_obj_t *current_image_obj = nullptr;
static lv_obj_t *global_tabview = nullptr;
static lv_obj_t *global_canvas = nullptr;

static lv_coord_t last_x = -1;
static lv_coord_t last_y = -1;

// --- Drawing Descriptor ---
// Define how the line should look (color, width).
static lv_draw_line_dsc_t line_dsc;

// --- TCP CLIENT / WIFI CONFIGURATION (NEW) ---
const char* WIFI_SSID = "ITSY";
const char* WIFI_PASSWORD = "0546652087";

// IMPORTANT: Replace this IP with your server's actual local IP address
IPAddress SERVER_IP(10,0,0,2); 
const int SERVER_PORT = 8080;          

WiFiClient client;
const char* DEVICE_HOSTNAME = "ESP32-LVGL-Client";


// Function to handle Wi-Fi connection
void connectToWiFi() {
    Serial.print("Connecting to: ");
    Serial.println(WIFI_SSID);
    WiFi.setHostname(DEVICE_HOSTNAME);
    
    // Begin connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected.");
        Serial.print("Client IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi Failed to Connect. Check credentials and try again.");
    }
}

// Display flushing
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

// Read touch points
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

void handle_sound_btn();

// Configure initial image tab
lv_obj_t* init_image_display(lv_obj_t* parent, const char* path) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    lv_obj_set_size(cont, 300, 150);
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
    lv_label_set_text(label, LV_SYMBOL_VOLUME_MAX);
    lv_obj_center(label);


    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            handle_sound_btn();
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

// Changes img_obj to .bin image in new_path (from Micro-SD Card)
void change_image(lv_obj_t *img_obj, const char *new_path) {
    lv_obj_t *parent_cont = lv_obj_get_parent(img_obj);

    lv_obj_del(img_obj);
            
    lv_obj_t *img = lv_img_create(parent_cont);
    lv_img_set_src(img, new_path);

    if (lv_img_get_src(img) == NULL) {
        lv_obj_clean(img);
                
        lv_obj_t *label = lv_label_create(parent_cont);
        lv_label_set_text(label, "IMG NOT FOUND");
        lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0x00, 0x00), 0);
        lv_obj_center(label);
        Serial.println("Error: Failed to load");
        return;
    }

    current_image_obj = img;
    lv_obj_update_layout(parent_cont);
}


class Game {
public:
    Game(lv_obj_t *img, lv_obj_t *timer) : current_image_obj_m(img), image_pos_m(0), timer_bar_obj_m(timer) {
        start_timer_animation();
        for (int i = 0; i < 10; ++i) {
            image_name_list_m = read_directory_file_list("S:/images");
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

    void start_timer_animation();
    void pause_timer_animation();

    void process(char c) {
        if(c != get_first_letter()) {
            Serial.println("No");
            handle_failure();
            return;
        }

        Serial.println("Yes");
        handle_success();
    }

    void iterate_image() {
        image_pos_m = (image_pos_m + 1) % image_name_list_m.size();
    }
private:
    lv_obj_t *current_image_obj_m;
    lv_obj_t *timer_bar_obj_m
    ;
    int image_pos_m;
    std::vector<std::string> image_name_list_m;

    void handle_failure();
    void handle_success();


};



Game* global_game; // Game instance


static void close_msgbox_timer_cb(lv_timer_t *timer) {
    lv_obj_t *msgbox = (lv_obj_t*)timer->user_data;
    lv_msgbox_close(msgbox); 
}

void Game::handle_failure() {
    pause_timer_animation();

    lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_CLOSE, "NO", NULL, false);
    lv_obj_center(pop_up);
    lv_obj_set_width(pop_up, 65);

    uint32_t close_delay_ms = 1000;
    lv_timer_t *timer = lv_timer_create(close_msgbox_timer_cb, close_delay_ms, pop_up);
    lv_timer_set_repeat_count(timer, 1);

    
    lv_tabview_set_act(global_tabview, 0, LV_ANIM_ON);

    iterate_image();
    start_timer_animation();
    change_image(current_image_obj, global_game->get_path().c_str());
    playLossSound();
}

void Game::handle_success() {
    pause_timer_animation();

    lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_OK, "YES", NULL, false);
    lv_obj_center(pop_up);
    lv_obj_set_width(pop_up, 65);

    uint32_t close_delay_ms = 1000;
    lv_timer_t *timer = lv_timer_create(close_msgbox_timer_cb, close_delay_ms, pop_up);
    lv_timer_set_repeat_count(timer, 1);

    
    lv_tabview_set_act(global_tabview, 0, LV_ANIM_ON);

    iterate_image();
    start_timer_animation();
    change_image(current_image_obj, global_game->get_path().c_str());
    playVictorySound();
}

void handle_clear_btn() {
    if(!global_canvas) return;
    lv_canvas_fill_bg(global_canvas, lv_color_white(), LV_OPA_COVER);
        
    last_x = -1; 
    last_y = -1;
}

void handle_check_btn() {
    if (global_canvas == nullptr || WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Canvas not ready or WiFi disconnected. Cannot send data.");
        return;
    }

    // 1. Get Canvas Data Descriptor
    const lv_img_dsc_t* img_dsc = lv_canvas_get_img(global_canvas);
    if (!img_dsc || !img_dsc->data) {
        Serial.println("Error: Canvas buffer data not found.");
        return;
    }
    
    // Calculate the total size of the pixel buffer (Width * Height * 2 bytes/color for 16-bit color)
    size_t buf_size = img_dsc->header.w * img_dsc->header.h * sizeof(lv_color_t);
    uint8_t* buffer = (uint8_t*)img_dsc->data;

    Serial.printf("\n*** Button Pressed! Sending Canvas Buffer (%dx%d, %d bytes) ***\n", 
                  img_dsc->header.w, img_dsc->header.h, buf_size);

    // 2. Attempt to connect to the server
    if (!client.connected()) {
        Serial.print("Attempting to connect to TCP Server...");
        if (client.connect(SERVER_IP, SERVER_PORT)) {
            Serial.println(" connected!");
        } else {
            Serial.println(" connection failed. Server might be offline or IP is wrong.");
            return;
        }
    }

    // 3. Send the raw data
    if (client.connected()) {
        // --- Protocol Header ---
        // Send a simple header so the Python server knows what to expect (Width,Height,Size)
        String header = "CANVAS:" + String(img_dsc->header.w) + "," + 
                        String(img_dsc->header.h) + "," + String(buf_size) + "\n";
        client.print(header);
        
        // Send the raw pixel buffer data
        size_t bytes_sent = client.write(buffer, buf_size);
        
        Serial.printf("Sent %d bytes of canvas data.\n", bytes_sent);

        // 4. Read server acknowledgment (ACK)
        delay(100); 
        if (client.available()) {
            Serial.print("<- Server ACK: ");
            while (client.available()) {
                String line = client.readStringUntil('\n');
                Serial.println(line);
            }
        } else {
            Serial.println("<- No acknowledgment received from server.");
        }

        // 5. Close the connection after sending/receiving
        client.stop();
        Serial.println("Connection closed after transfer.");
    }
}

static void on_timer_timeout(lv_anim_t * a) {
    global_game->pause_timer_animation();
    Serial.println("!!! TIME IS UP! Moving to next image. !!!");
    lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_WARNING, "time's up", NULL, false);
    lv_obj_center(pop_up);
    lv_obj_set_width(pop_up, 100);

    uint32_t close_delay_ms = 1000;
    lv_timer_t *timer = lv_timer_create(close_msgbox_timer_cb, close_delay_ms, pop_up);
    lv_timer_set_repeat_count(timer, 1);


    lv_tabview_set_act(global_tabview, 0, LV_ANIM_ON);

    global_game->iterate_image();
    change_image(current_image_obj, global_game->get_path().c_str());

    global_game->start_timer_animation();
}

void Game::start_timer_animation() {
    lv_anim_del(timer_bar_obj_m, NULL);
    
    int duration_seconds = lv_bar_get_max_value(timer_bar_obj_m);
    
    lv_bar_set_value(timer_bar_obj_m, duration_seconds, LV_ANIM_OFF); 

    lv_anim_t a; 
    lv_anim_init(&a); 
    lv_anim_set_var(&a, timer_bar_obj_m); 
    
    lv_anim_set_values(&a, duration_seconds, 0); 
    lv_anim_set_time(&a, (uint32_t)duration_seconds * 1000); 
    
    lv_anim_set_exec_cb(&a, set_timer_value); 
    lv_anim_set_playback_time(&a, 0); 
    lv_anim_set_repeat_count(&a, 0); 

    // --- NEW: Set the callback for when the animation is finished (time is up) ---
    lv_anim_set_ready_cb(&a, on_timer_timeout); 

    lv_anim_start(&a);
}

void Game::pause_timer_animation() {
    lv_anim_del(timer_bar_obj_m, NULL);
    Serial.println("Timer halted at current time.");
}

void handle_sound_btn() {
    // Speak
    return;
}

static void keyboard_event_cb(lv_event_t *e) {
    lv_obj_t *kb = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    // This gets the text of the button that was clicked
    const char *txt = lv_keyboard_get_btn_text(kb, lv_keyboard_get_selected_btn(kb));

    if (code == LV_EVENT_VALUE_CHANGED && txt && strlen(txt) == 1) {
        // Only process single-character buttons (letters/numbers/symbols)
        char pressed_char = txt[0];
        Serial.printf("Keyboard pressed: %c\n", pressed_char);
        if (global_game) {
            global_game->process(pressed_char);
        }
    } else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Handle "OK" or "Close" if needed, e.g., to switch back to the main tab
        Serial.printf("Keyboard action: %s\n", (code == LV_EVENT_READY) ? "READY (OK)" : "CANCEL (Close)");
        lv_tabview_set_act(global_tabview, 0, LV_ANIM_ON); // Switch back to image tab
    }
}


static void canvas_draw_event_cb(lv_event_t * e) {
    lv_obj_t* canvas = lv_event_get_target(e);
    lv_indev_t* indev = lv_indev_get_act();
    lv_event_code_t code = lv_event_get_code(e);

    if (indev == NULL) return;

    if (code == LV_EVENT_PRESSING) {
        lv_point_t current_point;
        
        // Get the current touch coordinates relative to the screen
        lv_indev_get_point(indev, &current_point);
        
        // Translate screen coordinates to canvas-local coordinates
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

lv_obj_t* initialize_and_place_canvas(lv_obj_t* parent_obj, lv_coord_t width, lv_coord_t height) {
     lv_obj_t *cont = lv_obj_create(parent_obj);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    lv_obj_set_size(cont, 300, 150);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(cont, 10, 0);
    lv_obj_set_flex_align( cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER  );

    lv_obj_set_style_bg_color(cont, lv_color_hex(0x003773), 0); // Dark Blue Gray
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0); 



    lv_obj_t *button_cont = lv_obj_create(cont);
    lv_obj_set_size(button_cont, 60, 140);
    lv_obj_center(button_cont);
    lv_obj_set_flex_flow(button_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(button_cont, 10, 0);
    lv_obj_set_flex_align(button_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_bg_color(button_cont, lv_color_hex(0x003773), 0); // Dark Blue Gray
    lv_obj_set_style_bg_opa(button_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button_cont, 0, 0); 

    lv_obj_clear_flag(button_cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    lv_obj_t *clear_btn = lv_btn_create(button_cont);
    lv_obj_set_size(clear_btn, 60, 60);

    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, LV_SYMBOL_TRASH);
    lv_obj_center(clear_label);


    lv_obj_add_event_cb(clear_btn, [](lv_event_t *e) {
            handle_clear_btn();
    }, LV_EVENT_CLICKED, NULL);


     lv_obj_t *check_btn = lv_btn_create(button_cont);
    lv_obj_set_size(check_btn, 60, 60);

    lv_obj_t *check_label = lv_label_create(check_btn);
    lv_label_set_text(check_label, LV_SYMBOL_UPLOAD);
    lv_obj_center(check_label);


    lv_obj_add_event_cb(check_btn, [](lv_event_t *e) {
            handle_check_btn();
    }, LV_EVENT_CLICKED, NULL);




    // Calculate required buffer size in bytes
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height);

    // Allocate the buffer dynamically on the heap
    lv_color_t* canvas_buffer = (lv_color_t*) malloc(buf_size);
    
    if (canvas_buffer == nullptr) {
        return nullptr; // Allocation failed
    }

    // Create the canvas object
    lv_obj_t* canvas = lv_canvas_create(cont);
    
    if (!canvas) {
        free(canvas_buffer); // Free the buffer if canvas creation failed
        return nullptr;
    }

    global_canvas = canvas;

    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);



    // 4. Set the canvas size and center it
    lv_obj_set_size(canvas, width, height);
    lv_obj_center(canvas);
    
    // Set a border and shadow to make it look like a drawing pad
    lv_obj_set_style_border_color(canvas, lv_color_make(0xDD, 0xDD, 0xDD), 0);
    lv_obj_set_style_border_width(canvas, 0, 0);
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
    pinMode(melodyPin, OUTPUT);
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ_MAX, BUZZER_RESOLUTION);
    ledcAttachPin(melodyPin, BUZZER_CHANNEL);

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

    connectToWiFi();


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
        global_tabview = tabview;
        lv_obj_t *content = lv_tabview_get_content(tabview);
        // Clear the scrollable flag on the content area to prevent swiping
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
    
        lv_obj_set_size(tabview, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(tabview, 1); 

        // Add 4 tabs
        lv_obj_t *tab0 = lv_tabview_add_tab(tabview, LV_SYMBOL_IMAGE);
        lv_obj_t *tab1 = lv_tabview_add_tab(tabview, LV_SYMBOL_KEYBOARD);
        lv_obj_t *tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_EDIT);
        //lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "q-x");

        auto setup_tab_layout = [](lv_obj_t* tab) {
            lv_obj_set_style_bg_color(tab, lv_color_hex(0x504DB3), 0); // Dark Blue Gray
            lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0); // Ensure the color is visible
            
            lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

            lv_obj_set_style_pad_all(tab, 5, 0); 
            lv_obj_set_style_pad_gap(tab, 0, 0);
            
            // The default border width is already 0 from the tabview setup, but ensuring here:
            lv_obj_set_style_border_width(tab, 0, 0); 
            
            // Use flex layout to make sure children fill space
            lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN); 
            lv_obj_set_flex_align( tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER );
        };
        
        setup_tab_layout(tab0);
        setup_tab_layout(tab1);
        setup_tab_layout(tab2);
       // setup_tab_layout(tab3);


        lv_obj_t *keyboard = lv_keyboard_create(tab1);
        lv_obj_set_size(keyboard, LV_PCT(100), LV_PCT(100)); // Make it fill most of the tab
        lv_obj_center(keyboard);

        lv_obj_set_style_pad_all(keyboard, 0, 0);
        lv_obj_set_style_border_width(keyboard, 0, 0);

        // Set the mode to lowercase letters
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);

        // Set the event handler to capture key presses
        lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);



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
                    global_game->process(text[0]);
                }, LV_EVENT_CLICKED, NULL);
            }
    };



    // Letters for each tab
    //const char *letters1[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    //const char *letters2[] = {"i", "j", "k", "l", "m", "n", "o", "p"};
    //const char *letters3[] = {"q", "r", "s", "t", "u", "v", "w", "x"};

    // Intialize tabs
    

    lv_obj_t *timer_bar = create_timer_bar(QUIZ_DURATION_SECONDS, main_cont);
    global_game
 = new Game(current_image_obj, timer_bar);
    init_image_display(tab0, global_game->get_path().c_str());
    initialize_and_place_canvas(tab2, 120, 120);
    //add_letter_buttons(tab1, letters1, 8);
    //add_letter_buttons(tab2, letters2, 8);
    //add_letter_buttons(tab3, letters3, 8);


    //writeRGBLED(0,0,0);
    Serial.println("Setup done"); 
    }
}

void loop() {
    lv_timer_handler(); // let LVGL run
    //setRGBColor();
    delay(5);
}