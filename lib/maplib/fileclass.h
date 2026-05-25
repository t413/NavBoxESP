#pragma once

/// File class that matches the Arduino File

#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#else
#include <stdint.h>
#include <stdarg.h>
#include <cstdio>
#include <sys/stat.h>
#include <string>

namespace fs {

#define FILE_READ       "r"
#define FILE_WRITE      "w"
#define FILE_APPEND     "a"

class File {
    FILE* f = nullptr;
public:
    File() = default;
    File(FILE* file) : f(file) {}
    ~File() { close(); }

    operator bool() const { return f != nullptr; }
    bool seek(long pos) { return f && fseek(f, pos, SEEK_SET) == 0; }
    long size() {
        if (!f) return -1;
        long pos = ftell(f);
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, pos, SEEK_SET);
        return sz;
    }
    int available() { return f ? (size() - position()) : 0; }
    int readBytesUntil(char terminator, char* buffer, size_t length) {
        if (!f) return 0;
        size_t i = 0;
        while (i < length - 1) {
            int c = fgetc(f);
            if (c == EOF) break;
            if (c == terminator) break;
            buffer[i++] = (char)c;
        }
        buffer[i] = '\0';
        return i;
    }
    long position() { return f ? ftell(f) : -1; }
    size_t read(uint8_t* buf, size_t len) { return f ? fread(buf, 1, len, f) : 0; }
    size_t readBytes(char *buffer, size_t length) { return read((uint8_t*)buffer, length); }
    size_t write(const uint8_t* buf, size_t len) { return f ? fwrite(buf, 1, len, f) : 0; }
    int printf(const char* fmt, ...) {
        if (!f) return -1;
        va_list args;
        va_start(args, fmt);
        int ret = vfprintf(f, fmt, args);
        va_end(args);
        return ret;
    }
    int println(const char* s) { return f ? fprintf(f, "%s\n", s) : -1; }
    void close() { if (f) { fclose(f); f = nullptr; } }
};

} //namespace fs

struct SDClass {
    inline fs::File open(const char* path, const char* mode = FILE_READ) {
        return fs::File(fopen(path, mode));
    }
    bool exists(const char* path) {
        struct stat buffer;
        return (stat(path, &buffer) == 0);
    }
    bool mkdir(const char* path) {
        return ::mkdir(path, 0777) == 0;
    }
};

extern SDClass SD;

uint32_t millis();

#endif
