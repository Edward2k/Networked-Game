#include "CircularLineBuffer.h"

#include <string>
#include <iostream>
#include <cstdlib>
#include <mutex>

//TODO:: Assign3
//  Work on all the functions.
//  Also ask what the tick(); function is in client.cpp (do we need to use it?)


using namespace std;

int CircularLineBuffer::freeSpace() {
    int spaceFree = bufferSize - (count-start) - 1; //Will give the difference between pointers to return size

    return spaceFree;
}

bool CircularLineBuffer::isFull() {

    return start == (count+1)%bufferSize;//To loop back

}

bool CircularLineBuffer::isEmpty() {

    //If the head is equal to the tail, we are free (pointers aligned).
    return start==count;

}

int CircularLineBuffer::nextFreeIndex() {
    if (isFull() == false) {
        return (count + 1) % bufferSize; //next free index
    } else {
        return -1;
    }

}

int CircularLineBuffer::findNewline() { //looking for '\n' char

    for (int i = start; i != count; i = (i + 1) % bufferSize) {
        if (buffer[i] == '\n') {
            return (i+1)%bufferSize; //returns element of new line
        }
    }
    return -1; //error
}

bool CircularLineBuffer::hasLine() { //Is there a line in the buffer

    for (int i = start; i != count; i++) {
        if (buffer[i] == '\n') {
            return true;
        }
        i = i % bufferSize;
    }
    return false; // end
}

bool CircularLineBuffer::writeChars(const char *chars, size_t nchars) {
    if (nchars <= freeSpace()) {
        for (int i=1; i <= nchars; i++) {
            buffer[(count)%bufferSize] = chars[i-1]; //writes from count
            count ++;
            count = count % bufferSize;
        }
        return true; //no error
    }

    return false; //no room to write

}

std::string CircularLineBuffer::readLine() {
    string theLine;
    int end = findNewline();

    for (int i = start; i != end; i++) { //loop through the circular buffer (i-1 so we read \n
        theLine += buffer[start]; // append char
        start ++;
        start = start % bufferSize;
        i = i % bufferSize;
    }

    return theLine;
}
