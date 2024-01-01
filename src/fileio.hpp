#pragma once

#include <fstream>

using namespace std;

size_t file_write(const char* filename, const char* data, size_t size) {
    ofstream file(filename, ios::binary | ios::out);

    if (!file.is_open()) {
        throw runtime_error("Failed to open file for writing");
    }

    file.write(data, size);

    if (file.fail()) {
        throw runtime_error("Failed to write data to file");
    }

    file.close();

    return size;
}

size_t file_read(const char* filename, char* data, size_t size) {
    ifstream file(filename, ios::binary | ios::in);

    if (!file.is_open()) {
        throw runtime_error("Failed to open file for reading");
    }

    file.read(data, size);

    if (file.fail()) {
        throw runtime_error("Failed to read data from file");
    }

    file.close();

    return file.gcount();  // Return the actual number of bytes read
}

size_t file_size(const char* filename) {
    ifstream file(filename, ios::binary | ios::ate);

    if (!file.is_open()) {
        throw runtime_error("Failed to open file to get size");
    }

    size_t size = static_cast<size_t>(file.tellg());  // Get the file size

    file.close();

    return size;
}