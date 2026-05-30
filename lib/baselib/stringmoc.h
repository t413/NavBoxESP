#pragma once

#ifdef ARDUINO
// In Arduino environment, use the real String
#include <Arduino.h>
#else
// Minimal String mock for native testing
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>

class String {
private:
    char* buffer_ = nullptr;
    size_t len_ = 0;
    size_t capacity_ = 0;

    void _alloc(size_t size) {
        if (capacity_ < size) {
            buffer_ = (char*)realloc(buffer_, size + 1);
            capacity_ = size;
        }
    }

public:
    String() = default;

    String(const std::string str) : String(str.c_str()) { }
    String(const char* str) {
        if (str) {
            len_ = strlen(str);
            _alloc(len_);
            strcpy(buffer_, str);
        }
    }

    String(int value) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        len_ = strlen(buf);
        _alloc(len_);
        strcpy(buffer_, buf);
    }

    String(float value) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%g", value);
        len_ = strlen(buf);
        _alloc(len_);
        strcpy(buffer_, buf);
    }

    String(double value) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%g", value);
        len_ = strlen(buf);
        _alloc(len_);
        strcpy(buffer_, buf);
    }

    String(bool value) {
        const char* str = value ? "1" : "0";
        len_ = 1;
        _alloc(len_);
        strcpy(buffer_, str);
    }

    ~String() {
        if (buffer_) free(buffer_);
    }

    // Copy constructor
    String(const String& other) {
        if (other.buffer_) {
            len_ = other.len_;
            _alloc(len_);
            strcpy(buffer_, other.buffer_);
        }
    }

    // Assignment
    String& operator=(const String& other) {
        if (this != &other) {
            if (other.buffer_) {
                len_ = other.len_;
                _alloc(len_);
                strcpy(buffer_, other.buffer_);
            } else {
                len_ = 0;
                if (buffer_) buffer_[0] = '\0';
            }
        }
        return *this;
    }

    const char* c_str() const {
        return buffer_ ? buffer_ : "";
    }

    int length() const { return len_; }

    int toInt() const {
        return buffer_ ? atoi(buffer_) : 0;
    }

    float toFloat() const {
        return buffer_ ? atof(buffer_) : 0.0f;
    }

    bool operator==(const char* str) const {
        if (!buffer_ && !str) return true;
        if (!buffer_ || !str) return false;
        return strcmp(buffer_, str) == 0;
    }

    bool operator==(const String& other) const {
        if (!buffer_ && !other.buffer_) return true;
        if (!buffer_ || !other.buffer_) return false;
        return strcmp(buffer_, other.buffer_) == 0;
    }

    bool isEmpty() const {
        return !buffer_ || len_ == 0;
    }
};

#endif
