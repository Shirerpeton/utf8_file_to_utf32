#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>

struct utf8_to_wchar_state {
    unsigned char *buf;
    unsigned char overflow_buf[4];
    unsigned int overflow_buf_len; 
};

int utf8_to_wchar(wchar_t *dest, unsigned char *src);
int get_wchar_chunk(FILE *file, wchar_t *dest, struct utf8_to_wchar_state *state);
void init_utf8_to_wchar_state(struct utf8_to_wchar_state *state);
void free_utf8_to_wchar_state(struct utf8_to_wchar_state *state);

const unsigned int BUF_SIZE = 4096; 

int main(int argc, char **argv) {
    setlocale(LC_ALL, "en_US.UTF-8");  // set the locale

    if(argc < 2) {
        puts("pass file name");
        return 1;
    }
    FILE *file = fopen(argv[1], "r");
    if(!file) {
        puts("file doesn't exist or you don't have permissions to read it");
        return 1;
    } 
    struct utf8_to_wchar_state state = {0};
    init_utf8_to_wchar_state(&state);
    const unsigned int DEST_SIZE = 65535;
    wchar_t *dest = malloc(DEST_SIZE * sizeof(wchar_t));
    while(1) {
        int status = get_wchar_chunk(file, dest, &state);
        if(status < 0) {
            fputws(L"error getting next wchar_t chunk\n", stdout);
            return -1;
        }
        //fputws(dest, stdout);
        if(feof(file)) {
            break;
        }
    }

    free_utf8_to_wchar_state(&state);
    free(dest);
    fclose(file);

    return 0;
}

void init_utf8_to_wchar_state(struct utf8_to_wchar_state *state) {
    state->buf = malloc(BUF_SIZE + 1 + 3); // 1 for null teminator + 3 for max overflow from previous chunkmalloc(BUF_SIZE + 1 + 3); // 1 for null teminator + 3 for max overflow from previous chunkmalloc(BUF_SIZE + 1 + 3); // 1 for null teminator + 3 for max overflow from previous chunkmalloc(BUF_SIZE + 1 + 3); // 1 for null teminator + 3 for max overflow from previous chunk
}

void free_utf8_to_wchar_state(struct utf8_to_wchar_state *state) {
    free(state->buf);
}

int utf8_to_wchar(wchar_t *dest, unsigned char *src) {
    unsigned int wchars_written = 0;
    while(*src) {
        if((*src & 0b10000000) == 0) {
            *dest++ = *src++;
        } else if((*src & 0b11100000) == 0b11000000) {
            *dest++ = (((*src & 0b00011111) << 6) |
                    (*(src + 1) & 0b00111111));
            src += 2;
        } else if((*src & 0b11110000) == 0b11100000) {
            *dest++ = (((*src & 0b00001111) << 12) |
                    ((*(src + 1) & 0b00111111) << 6) |
                    (*(src + 2) & 0b00111111));
            src += 3;
        } else if((*src & 0b11111000) == 0b11110000) {
            *dest++ = (((*src & 0b00000111) << 18) |
                    ((*(src + 1) & 0b00111111) << 12) |
                    ((*(src + 2) & 0b00111111) << 6) |
                    (*(src + 3) & 0b00111111));
            src += 4;
        } else {
            return -1;
        }
        wchars_written++;
    }
    return wchars_written;
}

int get_wchar_chunk(FILE *file, wchar_t *dest, struct utf8_to_wchar_state *state) {
    for(int i = state->overflow_buf_len - 1; i >= 0; i--) {
        state->buf[state->overflow_buf_len - 1 - i] = state->overflow_buf[i];
    }
    unsigned char *buf_start = state->buf + state->overflow_buf_len;
    state->overflow_buf_len = 0;
    size_t bytes_read = fread(buf_start, sizeof(char), BUF_SIZE, file);
    if(ferror(file) || bytes_read <= 0) {
        fputws(L"error reading a file\n", stdout);
        return -1;
    }

    while(1) {
        unsigned char prefix = (buf_start[bytes_read - 1 - state->overflow_buf_len]) & 0b11000000;
        if(prefix == 0b10000000) {
            state->overflow_buf[state->overflow_buf_len] = buf_start[bytes_read - 1 - state->overflow_buf_len];
            state->overflow_buf_len++;
        } else if(prefix == 0b11000000) {
            unsigned int trailing_bytes = 0;
            for(unsigned int i = 6; i > 3; i--) {
                if(buf_start[bytes_read - 1 - state->overflow_buf_len] & (1 << i)) {
                    trailing_bytes++;
                } else {
                    break;
                }
            }
            if(trailing_bytes > state->overflow_buf_len) {
                state->overflow_buf[state->overflow_buf_len] = buf_start[bytes_read - 1 - state->overflow_buf_len];
                state->overflow_buf_len++;
                buf_start[bytes_read - state->overflow_buf_len] = '\0';
            } else {
                state->overflow_buf_len = 0;
                buf_start[bytes_read] = '\0';
            }
            break;
        } else {
            buf_start[bytes_read - state->overflow_buf_len] = '\0';
            break;
        }
    }
    int wchars_written = utf8_to_wchar(dest, state->buf);
    if(wchars_written == -1) {
        fputws(L"error converting utf8 to wchar_t\n", stdout);
        return -1;
    }
    dest[wchars_written] = L'\0';

    return 0;
}
