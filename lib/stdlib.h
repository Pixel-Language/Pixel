#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Math

double absf(double x) {
	return x < 0.0 ? -x : x;
}

int power(int base, int exp) {
	int result = 1;
	for (int i = 0; i < exp; i++) {
		result *= base;
	}
	return result;
}

// String operations
int string_length(const char* s) {
	return strlen(s);
}

const char* string_concat(const char* a, const char* b) {
	// WARNING: this is a hack ill make a proper one someday
	static char buffer[1024];
	sprintf(buffer, "%s%s", a, b);
	return buffer;
}

int string_equals(const char* a, const char* b) {
	return strcmp(a, b) == 0;
}

// Utilities
