#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../fileHandling/ReadEnv.cpp"

#include <sodium.h>
#include <vector>
#include <cstring>
#include <iostream>

using namespace std;

int executeCryption(const string& taskData) {
    if (sodium_init() < 0) {
        cerr << "Failed to initialize libsodium" << endl;
        return 1;
    }

    Task task = Task::fromString(taskData);
    ReadEnv env;
    string rawKey = env.getenv();  // Get key from .env

    if (rawKey.size() < crypto_stream_chacha20_KEYBYTES) {
        cerr << "Key too short, must be at least 32 bytes" << endl;
        return 1;
    }

    // Use first 32 bytes of rawKey as key
    unsigned char key[crypto_stream_chacha20_KEYBYTES];
    memcpy(key, rawKey.data(), crypto_stream_chacha20_KEYBYTES);

    // Use fixed nonce (24 bytes for ChaCha20 in libsodium)
    unsigned char nonce[crypto_stream_chacha20_NONCEBYTES] = {0};
    // You can make nonce random and store it with the file for real security!

    // Read entire file content into a buffer
    task.f_stream.seekg(0, ios::end);
    size_t fileSize = task.f_stream.tellg();
    task.f_stream.seekg(0, ios::beg);

    vector<unsigned char> buffer(fileSize);
    task.f_stream.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    task.f_stream.close();

    // Encrypt or decrypt: ChaCha20 xor is symmetric
    crypto_stream_chacha20_xor(buffer.data(), buffer.data(), fileSize, nonce, key);

    // Write back to the file (overwrite)
    task.f_stream.open(task.filePath, ios::out | ios::binary | ios::trunc);
    if (!task.f_stream.is_open()) {
        cerr << "Failed to open file for writing" << endl;
        return 1;
    }
    task.f_stream.write(reinterpret_cast<const char*>(buffer.data()), fileSize);
    task.f_stream.close();

    return 0;
}
