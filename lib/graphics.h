#pragma once
#include "raylib.h"
#include <math.h>

// Window
void c_init_window(int width, int height, const char* title) {
    InitWindow(width, height, title);
}

void c_close_window() {
    CloseWindow();
}

int c_window_should_close() {
    return WindowShouldClose();
}

int c_get_screen_width() {
    return GetScreenWidth();
}

int c_get_screen_height() {
    return GetScreenHeight();
}

// Frame
void c_begin_drawing() {
    BeginDrawing();
}

void c_end_drawing() {
    EndDrawing();
}

float c_get_frame_time() {
    return GetFrameTime();
}

int c_get_fps() {
    return GetFPS();
}

// Input
int c_is_key_down(int key) {
    return IsKeyDown(key);
}

int c_is_key_pressed(int key) {
    return IsKeyPressed(key);
}

int c_is_mouse_button_down(int button) {
    return IsMouseButtonDown(button);
}

// Mouse position (return as two separate values or use ext)
float c_get_mouse_x() {
    return GetMousePosition().x;
}

float c_get_mouse_y() {
    return GetMousePosition().y;
}

// Drawing
Color make_color(int color[]) {
    Color col;
    col.r = color[0];
    col.g = color[1];
    col.b = color[2];
    col.a = color[3];
    return col;
}

void c_clear_background(int color[]) {
    ClearBackground(make_color(color));
}

void c_draw_rect(float x, float y, float w, float h, int color[]) {
    DrawRectangle(x, y, w, h, make_color(color));
}

void c_draw_circle(float x, float y, float radius, int color[]) {
    DrawCircle(x, y, radius, make_color(color));
}

void c_draw_line(float x1, float y1, float x2, float y2, int color[]) {
    DrawLine(x1, y1, x2, y2, make_color(color));
}

void c_draw_text(const char* text, int x, int y, int size, int color[]) {
    DrawText(text, x, y, size, make_color(color));
}

// Math
float c_distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx*dx + dy*dy);
}

int c_get_random_value(int min, int max) {
    return GetRandomValue(min, max);
}

float c_sqrt(float x) {
    return sqrt(x);
}