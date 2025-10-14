#include <iostream>
#include <fstream>
#include <string>

using namespace std;

const size_t FILE_SIZE_MB = 20008; // Desired file size in MB
const string LINE_TEXT = "Added first line";
const size_t LINE_SIZE = LINE_TEXT.size();
const size_t MB_TO_BYTES = 1024 * 1024;

char counter='0';

int main() {
    const size_t total_bytes = FILE_SIZE_MB * MB_TO_BYTES;
    const size_t total_lines = total_bytes / LINE_SIZE;

    ofstream file("../test.txt", ios::out | ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Could not create file." << endl;
        return 1;
    }

    const size_t chunk_size = 8192; // Write in chunks (8 KB)
    const size_t lines_per_chunk = chunk_size / LINE_SIZE;

    string chunk_data;
    chunk_data.reserve(chunk_size);

    for (size_t i = 0; i < lines_per_chunk; ++i) {
        chunk_data += LINE_TEXT;
        chunk_data+=counter;
        chunk_data+="\n";
        counter++;
    }

    for (size_t i = 0; i < total_lines / lines_per_chunk; ++i) {
        file.write(chunk_data.c_str(), chunk_data.size());
    }

    // Write remaining lines if needed
    const size_t remaining_lines = total_lines % lines_per_chunk;
    for (size_t i = 0; i < remaining_lines; ++i) {
        file.write(LINE_TEXT.c_str(), LINE_SIZE);
    }

    file.close();
    cout << "File 'test.txt' created successfully with size: " << FILE_SIZE_MB << " MB." << endl;

    return 0;
}
