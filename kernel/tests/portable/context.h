#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>

struct mcu_context {
    uint8_t sreg;
    uint8_t r[32];
    union {
        struct {
            uint8_t low;
            uint8_t high;
        } part;
        void *ptr;
    } pc;
    union {
        struct {
            uint8_t low;
            uint8_t high;
        } part;
        void *ptr;
    } sp;
};

#endif
