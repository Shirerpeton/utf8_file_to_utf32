#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>

int utf8towchar(wchar_t *dest, unsigned char *src) {
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
    const unsigned int BUF_SIZE = 4096; 
    unsigned char *buf = malloc(BUF_SIZE + 1 + 3); // 1 for null teminator + 3 for max overflow from previous chunk
    const unsigned int DEST_SIZE = 65535;
    wchar_t *dest = malloc(DEST_SIZE * sizeof(wchar_t));
    unsigned int dest_length = 0;
    char overflow_buf[4];
    unsigned int overflow_buf_len = 0;
    unsigned char *buf_start = buf;
    //fputws(L"file content:\n", stdout);
    while(1) {
        size_t bytes_read = fread(buf_start, sizeof(char), BUF_SIZE, file);
        if(ferror(file) || bytes_read <= 0) {
            fputws(L"error reading a file\n", stdout);
            return -1;
        }

        overflow_buf_len = 0;
        while(1) {
            unsigned char prefix = (buf_start[bytes_read - 1 - overflow_buf_len]) & 0b11000000;
            if(prefix == 0b10000000) {
                overflow_buf[overflow_buf_len] = buf_start[bytes_read - 1 - overflow_buf_len];
                overflow_buf_len++;
            } else if(prefix == 0b11000000) {
                unsigned int trailing_bytes = 0;
                for(unsigned int i = 6; i > 3; i--) {
                    if(buf_start[bytes_read - 1 - overflow_buf_len] & (1 << i)) {
                        trailing_bytes++;
                    } else {
                        break;
                    }
                }
                if(trailing_bytes > overflow_buf_len) {
                    overflow_buf[overflow_buf_len] = buf_start[bytes_read - 1 - overflow_buf_len];
                    overflow_buf_len++;
                    buf_start[bytes_read - overflow_buf_len] = '\0';
                } else {
                    overflow_buf_len = 0;
                    buf_start[bytes_read] = '\0';
                }
                break;
            } else {
                buf_start[bytes_read - overflow_buf_len] = '\0';
                break;
            }
        }
        int wchars_written = utf8towchar(dest + dest_length, buf);
        //unsigned char *tmp_buf = buf;
        //size_t wchars_written = mbsrtowcs(dest + dest_length, (const char **)&buf, DEST_SIZE, NULL);
        //buf = tmp_buf;
        if(wchars_written == -1) {
            fputws(L"error converting utf8 to wchar_t\n", stdout);
            return -1;
        }
        dest[wchars_written] = L'\0';
        fputws(dest, stdout);
        //dest_length += wchars_written;
        for(int i = overflow_buf_len - 1; i >= 0; i--) {
            buf[overflow_buf_len - 1 - i] = overflow_buf[i];
        }
        buf_start = buf + overflow_buf_len;
        overflow_buf_len = 0;

        if(feof(file)) {
            break;
        }
    }

    //fputws(L"file content:\n", stdout);
    //fputws(dest, stdout);
    
    free(buf);
    free(dest);
    fclose(file);

    return 0;
}


