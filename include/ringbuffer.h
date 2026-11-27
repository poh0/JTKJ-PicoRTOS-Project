//from gemini, prompt was asking if there is a ring buffer built in some c library or freertos library
//helper data structure for displaying recieved characters
#pragma once
#include <stddef.h>
#include <string.h>

#define BUF_SIZE 21

typedef struct {
    char data[BUF_SIZE];
    size_t head;
    size_t count;
} ring_buf_t;

size_t rb_to_string(const ring_buf_t *rb, char *out, size_t out_size);

void rb_push(ring_buf_t *rb, char c);
