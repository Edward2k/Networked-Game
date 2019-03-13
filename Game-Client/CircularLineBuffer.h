//
// Created by Jesse on 2019-01-10.
//

#ifndef CPP_CHAT_CLIENT_CIRCULARBUFFER_H
#define CPP_CHAT_CLIENT_CIRCULARBUFFER_H


#include <cstdlib>
#include <string>
#include <mutex>
#include <iostream>

/**
 * Assignment 3
 *
 * See the lab manual for the assignment description.
 */
class CircularLineBuffer {
private:
    static const int bufferSize = 4096; // 4096
    char buffer[bufferSize];
    std::mutex mtx;
    int start, count;
public:
    int freeSpace();

    bool isFull();

    bool isEmpty();

    int nextFreeIndex();

    int findNewline();

    bool hasLine();

    bool writeChars(const char *chars, size_t nchars);

    std::string readLine();

    CircularLineBuffer() {
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = '\0';
            start = 0;
            count = 0;
        }
    }
};


#endif //CPP_CHAT_CLIENT_CIRCULARBUFFER_H
