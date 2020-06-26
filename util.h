#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <stdarg.h>

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(-1);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*) malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(-1);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(-1);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

struct timeval tv;

uint64_t utime() {
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LU + tv.tv_usec;
}

uint64_t mtime() {
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LU + (tv.tv_usec / 1000);
}

uint8_t* encode(int entries, ... ) {
    uint8_t* encoded = malloc(1 + 2 * entries + 65536);
    va_list args;

    va_start(args, entries * 2);

    encoded[0] = entries;
    uint16_t* entrs = (uint16_t*) (encoded + 1);


    uint16_t off = 0;

    for (uint8_t i = 0; i < entries; i++) {
        uint8_t* temp = va_arg(args, uint8_t*);
        uint16_t len = va_arg(args, uint16_t);
        entrs[i] = len;
        memcpy(encoded + 1 + 2 * entries + off, temp, len);
        off += len;
    }

    va_end(args);

    encoded = realloc(encoded, 1 + 2 * entries + off);
    return encoded;
}

typedef struct {
    uint64_t len;
    uint8_t* data;
} slice;

typedef struct {
    uint64_t slices;
    slice* data;
    uint8_t* payload;
} deserres;

deserres* decode(uint8_t* data) {
    deserres* out = malloc(sizeof(deserres));
    out->slices = data[0];
    out->data = malloc(sizeof(slice) * out->slices);
    out->payload = malloc(65536);

    uint16_t* entrs = (uint16_t*) (data + 1);

    uint64_t off = 0;

    for (uint64_t i = 0; i < out->slices; i++) {
        out->data[i].len = entrs[i];
        out->data[i].data = out->payload + off;
        memcpy(out->data[i].data, data + 1 + out->slices * 2 + off, out->data[i].len);
        off += out->data[i].len;
    }

    return out;
}


char* toString(uint8_t* start, uint8_t* end) {
    char* outstr = malloc((end - start) * 2 + 1);
    outstr[0] = '\0';

    char* ptr = outstr;

    while (start != end) {
        ptr += sprintf(ptr, "%02hhx", *start);
        start++;
    }

    return outstr;
}

char* slToString(slice* sl) {
    return toString(sl->data, sl->data + sl->len);
}

static uint8_t chartohex(char* c) {
    if (*c >= '0' && *c <= '9') {
        return *c - '0';
    }
    if (*c >='a' && *c <= 'f') {
        return (*c - 'a') + 10;
    }

    perror("Not a hex char.\n");
    exit(-1);
}

slice* toHex(char* str) {
    uint64_t len = strlen(str);
    if (len % 2 != 0) {
        perror("Can't decode hex string with odd number of nibbles.\n");
        exit(-1);
    }

    uint8_t* out = malloc(len / 2);

    for (uint64_t i = 0; i < len; i += 2) {
        uint8_t top = chartohex(str + i);
        uint8_t bottom = chartohex(str + i + 1);
        out[i / 2] = (top << 4) | (bottom);
    }

    slice* sl = malloc(sizeof(slice));
    sl->data = out;
    sl->len = len / 2;
    return sl;
}