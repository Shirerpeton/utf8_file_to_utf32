#ifndef UTF8_CONVERTER_H
#define UTF8_CONVERTER_H
#include <stdio.h>

struct utf8_to_wchar_state {
    unsigned char *buf;
    unsigned char overflow_buf[4];
    unsigned int overflow_buf_len; 
};

int get_wchar_chunk(FILE *file, wchar_t *dest, struct utf8_to_wchar_state *state);
void init_utf8_to_wchar_state(struct utf8_to_wchar_state *state);
void free_utf8_to_wchar_state(struct utf8_to_wchar_state *state);

#endif

