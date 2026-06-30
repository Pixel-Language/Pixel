#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <string>


int abs(int x) {
	return x < 0 ? -x : x;
}

int _max(int a, int b) {
	return a > b ? a : b;
}

int _min(int a, int b) {
	return a < b ? a : b;
}

std::string input_string() {
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