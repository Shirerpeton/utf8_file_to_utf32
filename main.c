#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>

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
    while(1) {
        size_t bytes_read = fread(buf_start, sizeof(char), BUF_SIZE, file);
        if(ferror(file) || bytes_read <= 0) {
            puts("error reading a file");
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
        unsigned char *buf_temp = buf;
        size_t wchars_written = mbsrtowcs(dest + dest_length, (const char **)&buf, DEST_SIZE, NULL);
        buf = buf_temp;
        if(wchars_written == -1) {
            return -1;
        }
        dest_length += wchars_written;
        for(int i = overflow_buf_len - 1; i >= 0; i--) {
            buf[overflow_buf_len - 1 - i] = overflow_buf[i];
        }
        buf_start = buf + overflow_buf_len;
        overflow_buf_len = 0;

        if(feof(file)) {
            break;
        }
    }

    fputws(L"file content:\n", stdout);
    fputws(dest, stdout);
    
    free(buf);
    free(dest);
    fclose(file);

    return 0;
}


