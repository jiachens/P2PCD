//
// Created by h1994st on 6/21/18.
//

#include "utils.h"

void print_hex(const uint8_t *buf, size_t len) {
    for (int i = 0; i < len; ++i) {
        printf("%.2X ", buf[i] & 0xFF);
        if ((i + 1) == len || (i + 1) % 16 == 0)
            printf("\n");
    }
}
