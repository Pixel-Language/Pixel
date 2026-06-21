#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

void print_int(int x) {
	printf("%d\n", x);
}

void print_float(double x) {
	printf("%f\n", x);
}

void print_string(const char* s) {
	printf("%s\n", s);
}

void print_bool(int b) {
	printf("%s\n", b ? "true" : "false");
}

void print_pointer(void *ptr) {
    printf("%p\n", (void *)ptr);
}

int abs(int x) {
	return x < 0 ? -x : x;
}

int _max(int a, int b) {
	return a > b ? a : b;
}

int _min(int a, int b) {
	return a < b ? a : b;
}

const char* input_string() {
	static char buffer[256];
	scanf("%255s", buffer);
	return buffer;
}

void* c_malloc(int size) {
    return malloc(size);
}

void c_free(void* ptr) {
    free(ptr);
}

void c_memset(void* ptr, int value, int size) {
    memset(ptr, value, size);
}

void c_memcpy(void* dest, void* src, int size) {
    memcpy(dest, src, size);
}