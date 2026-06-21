#include <stdio.h>
#include <stdbool.h>
#include "lib/builtins.h"
#include "lib/graphics.h"

void px_print_int(int px_x) {
    
        print_int(px_x);
    
}

void px_print_float(double px_x) {
    
        print_float(px_x);
    
}

void px_print_string(const char* px_s) {
    
        print_string(px_s);
    
}

void px_print_bool(bool px_b) {
    
        print_bool(px_b);
    
}

void px_print_pointer(void* px_p) {
    
        print_pointer(px_p);
    
}

int px_abs(int px_x) {
    
        return abs(px_x);
    
}

int px_max(int px_a, int px_b) {
    
        return _max(px_a, px_b);
    
}

int px_min(int px_a, int px_b) {
    
        return _min(px_a, px_b);
    
}

const char* px_input_string() {
    
        return input_string();
    
}

void* px_malloc(int px_size) {
    
        return c_malloc(px_size);
    
}

void px_free(void* px_ptr) {
    
        c_free(px_ptr);
    
}

void px_memset(void* px_ptr, int px_value, int px_size) {
    
        c_memset(px_ptr, px_value, px_size);
    
}

void px_memcpy(void* px_dest, void* px_src, int px_size) {
    
        c_memcpy(px_dest, px_src, px_size);
    
}

void px_init_graphics(int px_width, int px_height, const char* px_title) {
    
        c_init_window(px_width, px_height, px_title);
    
}

void px_close_graphics() {
    
        c_close_window();
    
}

bool px_should_close() {
    
        return c_window_should_close();
    
}

bool px_not_should_close() {
    
        return !c_window_should_close();
    
}

int px_get_screen_width() {
    
        return c_get_screen_width();
    
}

int px_get_screen_height() {
    
        return c_get_screen_height();
    
}

void px_begin_frame() {
    
        c_begin_drawing();
    
}

void px_end_frame() {
    
        c_end_drawing();
    
}

double px_get_deltatime() {
    
        return c_get_frame_time();
    
}

int px_get_fps() {
    
        return c_get_fps();
    
}

bool px_is_key_down(int px_key) {
    
        return c_is_key_down(px_key);
    
}

bool px_is_key_pressed(int px_key) {
    
        return c_is_key_pressed(px_key);
    
}

bool px_is_mouse_button_down(int px_button) {
    
        return c_is_mouse_button_down(px_button);
    
}

double px_get_mouse_x() {
    
        return c_get_mouse_x();
    
}

double px_get_mouse_y() {
    
        return c_get_mouse_y();
    
}

void px_clear_background(int* px_color) {
    
        c_clear_background(px_color);
    
}

void px_draw_rect(double px_x, double px_y, double px_w, double px_h, int* px_color) {
    
        c_draw_rect(px_x, px_y, px_w, px_h, px_color);
    
}

void px_draw_circle(double px_x, double px_y, double px_radius, int* px_color) {
    
        c_draw_circle(px_x, px_y, px_radius, px_color);
    
}

void px_draw_line(double px_x1, double px_y1, double px_x2, double px_y2, int* px_color) {
    
        c_draw_line(px_x1, px_y1, px_x2, px_y2, px_color);
    
}

void px_draw_text(const char* px_text, int px_x, int px_y, int px_size, int* px_color) {
    
        c_draw_text(px_text, px_x, px_y, px_size, px_color);
    
}

double px_distance(double px_x1, double px_y1, double px_x2, double px_y2) {
    
        return c_distance(px_x1, px_y1, px_x2, px_y2);
    
}

int px_get_random(int px_min, int px_max) {
    
        return c_get_random_value(px_min, px_max);
    
}

int main() {
    px_init_graphics(800, 600, "Pixel Game");
    double px_player_x = 400.0;
    double px_player_y = 300.0;
    while (px_not_should_close()) {
    int px_key_left = 65;
    int px_key_right = 68;
    int px_key_up = 87;
    int px_key_down = 83;
    double px_speed = 200.0;
    double px_dt = px_get_deltatime();
    double px_move_dist = px_speed * px_dt;
    double px_new_x = px_player_x;
    double px_new_y = px_player_y;
    if (px_is_key_down(px_key_left)) {
    px_new_x = px_new_x - px_move_dist;
    }
    if (px_is_key_down(px_key_right)) {
    px_new_x = px_new_x + px_move_dist;
    }
    if (px_is_key_down(px_key_up)) {
    px_new_y = px_new_y - px_move_dist;
    }
    if (px_is_key_down(px_key_down)) {
    px_new_y = px_new_y + px_move_dist;
    }
    int px_width = px_get_screen_width();
    int px_height = px_get_screen_height();
    if (px_new_x < 0) {
    px_new_x = 0;
    }
    if (px_new_x > px_width) {
    px_new_x = px_width;
    }
    if (px_new_y < 0) {
    px_new_y = 0;
    }
    if (px_new_y > px_height) {
    px_new_y = px_height;
    }
    px_begin_frame();
    int px_bg_color[] = { 50, 50, 50, 255 };
    px_clear_background(px_bg_color);
    int px_player_color[] = { 0, 255, 0, 255 };
    px_draw_circle(px_new_x, px_new_y, 10.0, px_player_color);
    int px_text_color[] = { 255, 255, 255, 255 };
    px_draw_text("WASD to move", 10, 10, 20, px_text_color);
    int px_fps = px_get_fps();
    px_print_int(px_fps);
    px_end_frame();
    px_player_x = px_new_x;
    px_player_y = px_new_y;
    }
    px_close_graphics();
    return 0;
}
