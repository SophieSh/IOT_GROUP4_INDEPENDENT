/*******************************************************************************
 *       ~- Full Project V2 -~
 * All functionallity from V1 and more
 * Keyboard for access to all alphabet
 * Popup timeout/fail/success messages
 * Symbol icons - for illiterate users
 * Wifi, IP & server TCP communication
 * Draw-By-Touch canvas with clear_btn
 * Send canvas to a server - check_btn
 * Play buzzer melody for fail/success
 * MULTILINGUAL SUPPORT - English & Hebrew
 ******************************************************************************/

// Includes
#include <lvgl.h> 
#include <Arduino_GFX_Library.h> // for Touch controls
#include <vector>                // For image_name_list
#include <string>                // Names in: S:/images
#include "FS.h"                  // For driver_handlers
#include "SD.h"                  // Micro-SD-Card reads
#include "driver_handlers.h"     // Drivers for SD card
#include "timer_bar.h"           // Timer progress bars
#include "sound.h"               // Victory/loss buzzes
#include <WiFi.h>                // For wifi connection
#include <WiFiClient.h>          // Client TCP protocol
#include "SECRETS.h"             // MUST CONFIGURE FILE
#include <random>


#define TFT_BL 27       
#define GFX_BL DF_GFX_BL
#define QUIZ_DURATION_SECONDS 30 // Time to submit answer
#define BL_CHANNEL 0

// Language support
enum Language {
    LANG_ENGLISH,
    LANG_HEBREW
};

// Global language variable - CHANGE THIS LINE TO SWITCH LANGUAGE:
// Use LANG_ENGLISH for English or LANG_HEBREW for Hebrew
static Language selected_language = LANG_HEBREW;  
static bool language_selected = true;


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
static lv_obj_t *global_letter_container = nullptr;
static lv_obj_t *global_keyboard = nullptr;
static lv_obj_t *global_main_cont = nullptr;

static lv_coord_t last_x = -1;
static lv_coord_t last_y = -1;

// --- Drawing Descriptor ---
// Define how the line should look (color, width).
static lv_draw_line_dsc_t line_dsc;

     

WiFiClient client;
const char* DEVICE_HOSTNAME = "ESP32-LVGL-Client";


// Function to handle Wi-Fi connection
bool connectToWiFi() {
    Serial.print("Connecting to: ");
    Serial.println(WIFI_SSID);
    WiFi.setHostname(DEVICE_HOSTNAME);
    
    // Begin connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int timeout = 0;
    lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_WIFI, "Connecting to WiFi", NULL, false);
    lv_obj_center(pop_up);
    lv_obj_set_width(pop_up, 250);
    lv_obj_t *text_label = lv_msgbox_get_text(pop_up);

    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        if (text_label) {
            char dots[21] = ""; // Max 20 dots + null terminator

            // Build the string of dots
            for (int i = 0; i < timeout + 1 && i < 20; i++)
                dots[i] = '.';
            
            dots[timeout + 1] = '\0';

            lv_label_set_text_fmt(text_label, "Connecting to WiFi%s", dots);
            lv_timer_handler(); 
        }
        timeout++;
    }
    lv_msgbox_close(pop_up);

    if (WiFi.status() == WL_CONNECTED) {
        
        Serial.println("\nWiFi Connected.");
        Serial.print("Client IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\nWiFi Failed to Connect. Check credentials and try again.");
        return false;
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
        
        // Get the correct directory based on selected language
        const char* image_dir = get_image_directory();
        
        for (int i = 0; i < 10; ++i) {
            image_name_list_m = read_directory_file_list(image_dir);
            if (!image_name_list_m.empty()) break;
            delay(100);
        }
        if (image_name_list_m.empty()) {
            char error_msg[150];
            snprintf(error_msg, sizeof(error_msg), "Error: No images\nDirectory: \"%s/*.bin\" empty\nPlease add images", image_dir);
            lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_SD_CARD, error_msg, NULL, false);
            lv_obj_center(pop_up);
            lv_obj_set_width(pop_up, 250);
        }
    }

    std::string get_path() {
        if (image_pos_m == -1) return "";
        std::string dir = get_image_directory();
        return dir + "/" + image_name_list_m[image_pos_m];
    }
    
    // Get the correct image directory based on language
    static const char* get_image_directory() {
        if (selected_language == LANG_ENGLISH) {
            return "S:/images/english";
        } else {
            return "S:/images/hebrew";
        }
    }

    char get_first_letter() {
        if(image_pos_m == -1) return 0;
        return image_name_list_m[image_pos_m][0];
    }
    
    // Get first character as string (handles UTF-8 multi-byte characters)
    std::string get_first_letter_str() {
        if(image_pos_m == -1) return "";
        
        std::string filename = image_name_list_m[image_pos_m];
        if (filename.empty()) return "";
        
        // For UTF-8, we need to get the complete first character
        // UTF-8 character can be 1-4 bytes
        unsigned char first_byte = filename[0];
        int char_len = 1;
        
        // Determine UTF-8 character length
        if ((first_byte & 0x80) == 0) {
            // Single byte (ASCII)
            char_len = 1;
        } else if ((first_byte & 0xE0) == 0xC0) {
            // 2-byte character
            char_len = 2;
        } else if ((first_byte & 0xF0) == 0xE0) {
            // 3-byte character (Hebrew is typically 2-3 bytes)
            char_len = 3;
        } else if ((first_byte & 0xF8) == 0xF0) {
            // 4-byte character
            char_len = 4;
        }
        
        return filename.substr(0, char_len);
    }

    void start_timer_animation();
    void pause_timer_animation();

    void process(char c) {
        // For single char comparison (English)
        std::string first_letter = get_first_letter_str();
        std::string pressed(1, c);
        
        // Handle both single byte (English) and multi-byte (Hebrew) comparison
        bool match = false;
        if (selected_language == LANG_ENGLISH) {
            // Simple single character comparison
            match = (tolower(c) == tolower(first_letter[0]));
        } else {
            // Hebrew - compare first byte (simplified, since Hebrew letters have unique first bytes)
            match = (c == first_letter[0]);
        }
        
        if (!match) {
            Serial.println("No");
            handle_failure();
            return;
        }

        Serial.println("Yes");
        handle_success();
    }
    
    // Process with string (for UTF-8 Hebrew support)
    void process_str(const char* str) {
        std::string first_letter = get_first_letter_str();
        std::string pressed(str);
        
        if (pressed != first_letter) {
            Serial.printf("No match: pressed='%s' vs expected='%s'\n", pressed.c_str(), first_letter.c_str());
            handle_failure();
            return;
        }

        Serial.println("Yes - correct!");
        handle_success();
    }

    void iterate_image() {
        image_pos_m = (image_pos_m + 1) % image_name_list_m.size();
    }
private:
    lv_obj_t *current_image_obj_m;
    lv_obj_t *timer_bar_obj_m;
    int image_pos_m;
    std::vector<std::string> image_name_list_m;

    void handle_failure();
    void handle_success();


};



Game* global_game; // Game instance

// Hebrew alphabet (UTF-8 encoded)
const char* hebrew_alphabet[] = {
    "א", "ב", "ג", "ד", "ה", "ו", "ז", "ח",
    "ט", "י", "כ", "ל", "מ", "נ", "ס", "ע",
    "פ", "צ", "ק", "ר", "ש", "ת"
};
const int hebrew_alphabet_size = 22;

// Function to get the appropriate font based on language
const lv_font_t* get_language_font() {
    if (selected_language == LANG_HEBREW) {
        return &lv_font_dejavu_16_persian_hebrew;
    } else {
        return &lv_font_montserrat_32;
    }
}

// Function to generate 8 random letters including the correct answer (English)
void generate_random_letters_english(char correct_letter, char* output_letters) {
    // Ensure correct letter is lowercase
    correct_letter = tolower(correct_letter);
    
    // Place the correct letter at a random position (0-7)
    int correct_position = random(0, 8);
    output_letters[correct_position] = correct_letter;
    
    // Fill the rest with random letters (excluding the correct one)
    for (int i = 0; i < 8; i++) {
        if (i == correct_position) continue;
        
        char random_letter;
        bool duplicate;
        do {
            duplicate = false;
            random_letter = 'a' + random(0, 26); // Random letter a-z
            
            // Check if this letter is already used
            for (int j = 0; j < i; j++) {
                if (output_letters[j] == random_letter) {
                    duplicate = true;
                    break;
                }
            }
            // Also check if it's the correct letter
            if (random_letter == correct_letter) {
                duplicate = true;
            }
        } while (duplicate);
        
        output_letters[i] = random_letter;
    }
}

// Function to generate 8 random Hebrew letters including the correct answer
void generate_random_letters_hebrew(const char* correct_letter_str, const char** output_letters) {
    // Find the correct Hebrew letter in the alphabet using string comparison
    int correct_index = -1;
    for (int i = 0; i < hebrew_alphabet_size; i++) {
        if (strcmp(hebrew_alphabet[i], correct_letter_str) == 0) {
            correct_index = i;
            Serial.printf("Found correct Hebrew letter '%s' at index %d\n", correct_letter_str, i);
            break;
        }
    }
    
    // If not found, try matching by first byte (fallback)
    if (correct_index == -1) {
        Serial.printf("Hebrew letter '%s' not found by full match, trying first byte match\n", correct_letter_str);
        for (int i = 0; i < hebrew_alphabet_size; i++) {
            if (hebrew_alphabet[i][0] == correct_letter_str[0]) {
                correct_index = i;
                Serial.printf("Found by first byte at index %d: '%s'\n", i, hebrew_alphabet[i]);
                break;
            }
        }
    }
    
    if (correct_index == -1) {
        Serial.printf("WARNING: Could not find Hebrew letter '%s', defaulting to index 0\n", correct_letter_str);
        correct_index = 0; // Default to first letter if not found
    }
    
    // Place the correct letter at a random position (0-7)
    int correct_position = random(0, 8);
    output_letters[correct_position] = hebrew_alphabet[correct_index];
    Serial.printf("Correct letter '%s' placed at button position %d\n", hebrew_alphabet[correct_index], correct_position);
    
    // Fill the rest with random Hebrew letters
    for (int i = 0; i < 8; i++) {
        if (i == correct_position) continue;
        
        int random_index;
        bool duplicate;
        do {
            duplicate = false;
            random_index = random(0, hebrew_alphabet_size);
            
            // Check if this letter is already used
            for (int j = 0; j < i; j++) {
                if (output_letters[j] == hebrew_alphabet[random_index]) {
                    duplicate = true;
                    break;
                }
            }
            // Also check if it's the correct letter
            if (random_index == correct_index) {
                duplicate = true;
            }
        } while (duplicate);
        
        output_letters[i] = hebrew_alphabet[random_index];
    }
}

// Function to update the letter buttons with new random letters
void update_letter_buttons() {
    if (!global_letter_container || !global_game) return;
    
    // Get the correct answer for the current image
    char correct_letter = global_game->get_first_letter();
    if (correct_letter == 0) return;
    
    // Clear all existing buttons
    lv_obj_clean(global_letter_container);
    
    // Recreate button styles (since we cleaned the container)
    static lv_style_t button_style;
    lv_style_init(&button_style);
    lv_style_set_bg_color(&button_style, lv_color_hex(0x59BDB8));
    lv_style_set_bg_opa(&button_style, LV_OPA_COVER);
    
    static lv_style_t pressed_style;
    lv_style_init(&pressed_style);
    lv_style_set_bg_color(&pressed_style, lv_color_hex(0x449490));

    static lv_style_t button_label_style;
    lv_style_init(&button_label_style);
    lv_style_set_text_font(&button_label_style, get_language_font()); 
    lv_style_set_text_color(&button_label_style, lv_color_hex(0x000000));
    
    if (selected_language == LANG_ENGLISH) {
        // English letters
        char random_letters[8];
        generate_random_letters_english(correct_letter, random_letters);
        
        // Create 8 buttons with the random letters
        for (int i = 0; i < 8; i++) {
            lv_obj_t *btn = lv_btn_create(global_letter_container);
            lv_obj_set_size(btn, 60, 55);

            lv_obj_add_style(btn, &button_style, LV_PART_MAIN);
            lv_obj_add_style(btn, &pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);

            lv_obj_t *label = lv_label_create(btn);
            char letter_str[2] = {random_letters[i], '\0'};
            lv_label_set_text(label, letter_str);
            lv_obj_center(label);
            lv_obj_add_style(label, &button_label_style, 0);

            // Add event handler for button press
            lv_obj_add_event_cb(btn, [](lv_event_t *e) {
                lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
                const char *text = lv_label_get_text(lbl);
                
                Serial.printf("Pressed: %s\n", text);
                if (global_game) {
                    if (selected_language == LANG_ENGLISH) {
                        global_game->process(text[0]);
                    } else {
                        global_game->process_str(text);
                    }
                }
            }, LV_EVENT_CLICKED, NULL);
        }
    } else {
        // Hebrew letters - get the correct letter as a string
        std::string correct_letter_str = global_game->get_first_letter_str();
        const char* random_letters[8];
        generate_random_letters_hebrew(correct_letter_str.c_str(), random_letters);
        
        // Create 8 buttons with the random Hebrew letters
        for (int i = 0; i < 8; i++) {
            lv_obj_t *btn = lv_btn_create(global_letter_container);
            lv_obj_set_size(btn, 60, 55);

            lv_obj_add_style(btn, &button_style, LV_PART_MAIN);
            lv_obj_add_style(btn, &pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, random_letters[i]);
            lv_obj_center(label);
            lv_obj_add_style(label, &button_label_style, 0);

            // Add event handler for button press
            lv_obj_add_event_cb(btn, [](lv_event_t *e) {
                lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
                const char *text = lv_label_get_text(lbl);
                
                Serial.printf("Pressed Hebrew: %s\n", text);
                if (global_game) {
                    global_game->process_str(text);
                }
            }, LV_EVENT_CLICKED, NULL);
        }
    }
}

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
    update_letter_buttons(); // Update random letters for new image
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
    update_letter_buttons(); // Update random letters for new image
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

    //stopBuzzer(); 
    //Serial.println("Buzzer forcibly stopped to prioritize Wi-Fi transfer.");
    //delay(100);
    // Get Canvas Data Descriptor
    const lv_img_dsc_t* img_dsc = lv_canvas_get_img(global_canvas);
    if (!img_dsc || !img_dsc->data) {
        Serial.println("Error: Canvas buffer data not found.");
        return;
    }
    
    // Calculate the total size of the pixel buffer (Width * Height * 2 bytes/color for 16-bit color)
    size_t buf_size = img_dsc->header.w * img_dsc->header.h * sizeof(lv_color_t);
    uint8_t* buffer = (uint8_t*)img_dsc->data;

    Serial.printf("\n*** Button Pressed! Sending Canvas Buffer (%dx%d, %d bytes) ***\n", img_dsc->header.w, img_dsc->header.h, buf_size);

    // Attempt to connect to the server
    if (!client.connected()) {
        Serial.print("Attempting to connect to TCP Server...");
        if (client.connect(SERVER_IP, SERVER_PORT)) {
            Serial.println(" connected!");
        } else {
            Serial.println(" connection failed. Server might be offline or IP is wrong.");
            return;
        }
    }

    // Send the raw data
    if (client.connected()) {
        // --- Protocol Header ---
        // Send a simple header so the Python server knows what to expect (Width,Height,Size)
        String header = "CANVAS:" + String(img_dsc->header.w) + "," + String(img_dsc->header.h) + "," + String(buf_size) + "\n";
        client.print(header);

        delay(200);
        
        // Send the raw pixel buffer data
        size_t bytes_sent = client.write(buffer, buf_size);
        
        Serial.printf("Sent %d bytes of canvas data.\n", bytes_sent);

        // Read server acknowledgment (ACK)
        delay(200); 
        if (client.available()) {
            Serial.print("<- Server ACK: ");
            while (client.available()) {
                String line = client.readStringUntil('\n');
                Serial.println(line);
            }
        } else {
            Serial.println("<- No acknowledgment received from server.");
        }

        // Close the connection after sending/receiving
        client.stop();
        Serial.println("Connection closed after transfer.");
    }

    //ledcAttachPin(melodyPin, BUZZER_CHANNEL); 
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
    update_letter_buttons(); // Update random letters for new image

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

    if (code == LV_EVENT_VALUE_CHANGED && txt) {
        // Process character buttons
        Serial.printf("Keyboard pressed: %s\n", txt);
        if (global_game && txt[0] != '\0') {
            if (selected_language == LANG_ENGLISH) {
                global_game->process(txt[0]);
            } else {
                global_game->process_str(txt);
            }
        }
    } else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Handle "OK" or "Close" if needed, e.g., to switch back to the main tab
        Serial.printf("Keyboard action: %s\n", (code == LV_EVENT_READY) ? "READY (OK)" : "CANCEL (Close)");
        lv_tabview_set_act(global_tabview, 0, LV_ANIM_ON); // Switch back to image tab
    }
}

// Create Hebrew keyboard using button matrix
lv_obj_t* create_hebrew_keyboard(lv_obj_t* parent) {
    static const char * btnm_map[] = {
        "א", "ב", "ג", "ד", "ה", "ו", "ז", "\n",
        "ח", "ט", "י", "כ", "ל", "מ", "נ", "\n",
        "ס", "ע", "פ", "צ", "ק", "ר", "ש", "ת", "\n",
        LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
    };

    lv_obj_t * btnm = lv_btnmatrix_create(parent);
    lv_btnmatrix_set_map(btnm, btnm_map);
    lv_obj_set_size(btnm, LV_PCT(100), LV_PCT(100));
    lv_obj_center(btnm);
    
    // Set Hebrew font for the button matrix
    lv_obj_set_style_text_font(btnm, &lv_font_dejavu_16_persian_hebrew, 0);
    
    // Add event handler
    lv_obj_add_event_cb(btnm, [](lv_event_t *e) {
        lv_obj_t *obj = lv_event_get_target(e);
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
        const char *txt = lv_btnmatrix_get_btn_text(obj, btn_id);
        
        if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
            Serial.printf("Hebrew keyboard pressed: %s\n", txt);
            
            // Handle backspace and OK
            if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                Serial.println("Backspace pressed");
                return;
            } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
                Serial.println("OK pressed - return to main");
                lv_tabview_set_act(global_tabview, 0, LV_ANIM_ON);
                return;
            }
            
            // Process Hebrew letter
            if (global_game && txt[0] != '\0') {
                global_game->process_str(txt);
            }
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    return btnm;
}

// Function to update keyboard based on selected language
void update_keyboard_language() {
    if (!global_keyboard) return;
    
    // For now, LVGL keyboard widget only supports standard layouts
    // Hebrew keyboard would need custom button matrix
    // We'll keep the standard keyboard for both, but could be extended
    lv_keyboard_set_mode(global_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
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


// Forward declaration for main app initialization
void init_main_app();

// Language selection callback
void language_selection_callback(lv_event_t *e) {
    Language *lang = (Language*)lv_event_get_user_data(e);
    selected_language = *lang;
    language_selected = true;
    
    Serial.printf("Language selected: %s\n", selected_language == LANG_ENGLISH ? "English" : "Hebrew");
    
    // Hide the language selection screen (just hide, don't delete)
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lang_screen = lv_obj_get_parent(lv_obj_get_parent(btn));
    lv_obj_add_flag(lang_screen, LV_OBJ_FLAG_HIDDEN);
    
    // Initialize the main application (this creates the UI on top)
    init_main_app();
}

// Create language selection screen
void create_language_selection_screen() {
    // Create a full-screen container with flex layout
    lv_obj_t *lang_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(lang_screen, screenWidth, screenHeight);
    lv_obj_center(lang_screen);
    lv_obj_clear_flag(lang_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lang_screen, lv_color_hex(0x504DB3), 0);
    lv_obj_set_style_bg_opa(lang_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lang_screen, 0, 0);
    lv_obj_set_flex_flow(lang_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lang_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(lang_screen, 20, 0);
    
    // English button
    static Language lang_en = LANG_ENGLISH;
    lv_obj_t *btn_english = lv_btn_create(lang_screen);
    lv_obj_set_size(btn_english, 180, 60);
    lv_obj_set_style_bg_color(btn_english, lv_color_hex(0x3498DB), 0);
    
    lv_obj_t *label_en = lv_label_create(btn_english);
    lv_label_set_text(label_en, "English");
    lv_obj_set_style_text_font(label_en, &lv_font_montserrat_32, 0);
    lv_obj_center(label_en);
    
    lv_obj_add_event_cb(btn_english, language_selection_callback, LV_EVENT_CLICKED, &lang_en);
    
    // Hebrew button  
    static Language lang_he = LANG_HEBREW;
    lv_obj_t *btn_hebrew = lv_btn_create(lang_screen);
    lv_obj_set_size(btn_hebrew, 180, 60);
    lv_obj_set_style_bg_color(btn_hebrew, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t *label_he = lv_label_create(btn_hebrew);
    lv_label_set_text(label_he, "עברית");
    lv_obj_set_style_text_font(label_he, &lv_font_dejavu_16_persian_hebrew, 0);
    lv_obj_center(label_he);
    
    lv_obj_add_event_cb(btn_hebrew, language_selection_callback, LV_EVENT_CLICKED, &lang_he);
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
    gfx->fillScreen(BLUE);

    Serial.println("beginning initialization\n");

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

    if(!init_sd_card()) {
        lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_SD_CARD, "Error: SD Card mounting fail\nPlease insert Micro SD Card", NULL, false);
        lv_obj_center(pop_up);
        lv_obj_set_width(pop_up, 250);
        lv_timer_handler();
        delay(200);
        while(!init_sd_card()) {
            lv_timer_handler();
            delay(200);
        }
        lv_obj_t *text_label = lv_msgbox_get_text(pop_up);
        lv_label_set_text_fmt(text_label, "SD card successfully mounted!");
        lv_timer_handler(); 
        delay(1500);
        lv_msgbox_close(pop_up);
        lv_timer_handler();
    } else {
        lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_SD_CARD, "SD Card successfully mounted!", NULL, false);
        lv_obj_center(pop_up);
        lv_obj_set_width(pop_up, 250);
        lv_timer_handler();

        delay(1500);
        lv_msgbox_close(pop_up);
        lv_timer_handler();
    }

    
    if(!connectToWiFi()) {
        lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_WIFI, "Error: WiFi connection failed\nConfigure SECRETS.h & reset", NULL, false);
        lv_obj_center(pop_up);
        lv_obj_set_width(pop_up, 250);
        lv_timer_handler();
        delay(1500);
        lv_msgbox_close(pop_up);
        lv_timer_handler();
    } else {
        lv_obj_t *pop_up = lv_msgbox_create(NULL, LV_SYMBOL_WIFI, "WiFi connected successfully!", NULL, false);
        lv_obj_center(pop_up);
        lv_obj_set_width(pop_up, 250);
        lv_timer_handler();
        delay(1500);
        lv_msgbox_close(pop_up);
        lv_timer_handler();
    }

    // Initialize the main app directly (no language selection screen)
    init_main_app();

    Serial.println("Setup complete"); 
    }
}

// Initialize main application (called after language selection)
void init_main_app() {
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
    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "ABC");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_KEYBOARD);
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, LV_SYMBOL_EDIT);

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
    setup_tab_layout(tab3);

    // Create letter button container for tab1 (Random Letters)
    global_letter_container = lv_obj_create(tab1);
    lv_obj_set_size(global_letter_container, 300, 150);
    lv_obj_center(global_letter_container);
    lv_obj_set_flex_flow(global_letter_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(global_letter_container, 10, 0);
    lv_obj_set_flex_align(global_letter_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(global_letter_container, lv_color_hex(0x003773), 0);
    lv_obj_set_style_bg_opa(global_letter_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(global_letter_container, 0, 0);

    // Create keyboard for tab2 (language-specific)
    lv_obj_t *keyboard;
    if (selected_language == LANG_HEBREW) {
        keyboard = create_hebrew_keyboard(tab2);
    } else {
        keyboard = lv_keyboard_create(tab2);
        lv_obj_set_size(keyboard, LV_PCT(100), LV_PCT(100));
        lv_obj_center(keyboard);

        lv_obj_set_style_pad_all(keyboard, 0, 0);
        lv_obj_set_style_border_width(keyboard, 0, 0);

        // Set the mode to lowercase letters
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);

        // Set the event handler to capture key presses
        lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
    }

    // Initialize canvas tab
    initialize_and_place_canvas(tab3, 120, 120);
    lv_timer_handler();

    // Create timer bar
    lv_obj_t *timer_bar = create_timer_bar(QUIZ_DURATION_SECONDS, main_cont);
    
    // Get the first image from the directory
    const char* image_dir = Game::get_image_directory();
    std::vector<std::string> image_list = read_directory_file_list(image_dir);
    
    std::string first_image_path;
    if (!image_list.empty()) {
        first_image_path = std::string(image_dir) + "/" + image_list[0];
    } else {
        first_image_path = std::string(image_dir) + "/cat.bin";  // Fallback
    }
    
    current_image_obj = init_image_display(tab0, first_image_path.c_str());
    
    // Create game with the image object
    global_game = new Game(current_image_obj, timer_bar);
    
    // Initialize random letter buttons for the first image
    update_letter_buttons();

    global_keyboard = keyboard;
    global_main_cont = main_cont;

    Serial.println("Main app initialization complete"); 
}

void loop() {
    lv_timer_handler(); // let LVGL run
    //setRGBColor();
    delay(5);
}