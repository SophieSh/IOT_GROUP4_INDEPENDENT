//
// Created by Yonatan, Gital, Sofiya on 08/10/2025.
//

#ifndef SRC_TIMER_BAR_H
#define SRC_TIMER_BAR_H


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

static void set_timer_value(void *bar, int32_t v) {
    lv_obj_t* bar_obj = (lv_obj_t *)bar;
    lv_bar_set_value(bar_obj, v, LV_ANIM_OFF);
    update_timer_color(bar_obj);
}

static void timer_display_event_cb(lv_event_t * e) {
    // Get parameters related to the drawing process
    lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);

    // Check if the current drawing event is for the INDICATOR part of the bar
    if(dsc->part != LV_PART_INDICATOR) return;

    // Get the bar object
    lv_obj_t* obj = lv_event_get_target(e);

    // Setup text style (font, color)
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT; // Default LVGL font

    // Convert current value (seconds remaining) to a string
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);
    lv_area_t txt_area;

    // If the indicator is wide enough, place the text inside on the right
    if(lv_area_get_width(dsc->draw_area) > txt_size.x + 20) {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white(); // Text color is white for contrast inside the bar
    } else { 
        //  O.W., put the text outside to the right of the bar
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black(); 
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;
    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

lv_obj_t *create_timer_bar(int duration_seconds, lv_obj_t *parent) {
    lv_obj_t* timer_bar_cont = lv_obj_create(parent);
    lv_obj_set_size(timer_bar_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_clear_flag(timer_bar_cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC); // Eliminate movement
    
    // Make the container invisible (no background, border, padding gap)
    lv_obj_set_style_pad_gap(timer_bar_cont, 0, 0);
    lv_obj_set_style_radius(timer_bar_cont, 0, 0); 
    lv_obj_set_style_border_width(timer_bar_cont, 0, 0); 
    lv_obj_set_style_bg_opa(timer_bar_cont, LV_OPA_TRANSP, 0); 
    
    // Apply padding
    lv_obj_set_style_pad_all(timer_bar_cont, 7, 0);

    lv_obj_set_style_bg_color(timer_bar_cont, lv_color_hex(0x504DB3), 0); // Dark Blue Gray
    lv_obj_set_style_bg_opa(timer_bar_cont, LV_OPA_COVER, 0);     

    lv_obj_t* timer_bar = lv_bar_create(timer_bar_cont);  
    lv_bar_set_range(timer_bar, 0, duration_seconds);
    lv_obj_set_size(timer_bar, LV_PCT(100), 20); // size in pixels
    

    // Position it in the center of the screen
    lv_obj_center(timer_bar); 
    
    lv_obj_set_style_pad_all(timer_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(timer_bar, 0, LV_PART_MAIN);

    

    lv_anim_t a; // Create structure for animation settings.
    lv_anim_init(&a); 
    lv_anim_set_var(&a, timer_bar); // Set target object for animation
    
    lv_anim_set_values(&a, duration_seconds, 0); // Value range: Counts from duration_seconds down to 0
    
    lv_anim_set_time(&a, (uint32_t)duration_seconds * 1000);  // Time Duration in milliseconds
    
    lv_anim_set_exec_cb(&a, set_timer_value); // Set the callback function to be called at every animation step
    lv_anim_set_playback_time(&a, 0);         // No reverse playback
    lv_anim_set_repeat_count(&a, 0);          // Run only once  

    lv_anim_start(&a);


    // Dynamic color handeling
    lv_obj_add_event_cb(timer_bar, timer_display_event_cb, LV_EVENT_DRAW_PART_END, NULL);

    return timer_bar;
}

#endif //SRC_TIMER_BAR_H
