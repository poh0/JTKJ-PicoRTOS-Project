#include "ringbuffer.h"
//functions by gemini, prompt create a ready to use ringbuffer data structure that has a function that returns a null terminated string

void rb_push(ring_buf_t *rb, char c) {
    rb->data[rb->head] = c;
    rb->head = (rb->head + 1) % BUF_SIZE;
    if (rb->count < BUF_SIZE) {
        rb->count++;
    }
}

/**
 * Copies the ring buffer contents into `out` in chronological order
 * and appends a '\0' terminator.
 *
 * @param rb       Pointer to the ring buffer
 * @param out      Destination char array
 * @param out_size Total size of `out` (including space for '\0')
 * @return         Number of characters written (excluding '\0')
 */
size_t rb_to_string(const ring_buf_t *rb, char *out, size_t out_size) {
    if (!rb || !out || out_size == 0) {
        return 0;
    }

    // Reserve 1 byte for the null terminator
    size_t max_chars = out_size - 1;
    size_t to_copy = (rb->count < max_chars) ? rb->count : max_chars;

    if (to_copy == 0) {
        out[0] = '\0';
        return 0;
    }

    // Calculate the index of the oldest character we want to copy
    size_t start = (rb->head + BUF_SIZE - to_copy) % BUF_SIZE;

    // First chunk: from `start` up to the end of the physical array
    size_t first_chunk = BUF_SIZE - start;
    if (first_chunk >= to_copy) {
        // Data doesn't wrap around the end
        memcpy(out, &rb->data[start], to_copy);
    } else {
        // Data wraps around: copy end slice, then beginning slice
        size_t second_chunk = to_copy - first_chunk;
        memcpy(out, &rb->data[start], first_chunk);
        memcpy(out + first_chunk, &rb->data[0], second_chunk);
    }

    out[to_copy] = '\0';
    return to_copy;
}