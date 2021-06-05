/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is a function that generates random file names.
 */

#include <cstdlib>

void generate_random_file_name(char* buffer) {
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < 10; ++i) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[10] = '\0';
}
