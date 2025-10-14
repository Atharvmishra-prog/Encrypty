#include <openssl/evp.h>
#include <openssl/err.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <functional>
#include "./utils/ThreadPool/thread_pool.h"
#include <fstream>
#include <filesystem>
#include <atomic>
#include <chrono>
#include "./utils/progress_utils.h"

using namespace std;
namespace fs = std::filesystem;

static const unsigned char FIXED_KEY[32] = {
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};
static const unsigned char FIXED_IV[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x08, 0x09, 0x0a, 0x0b,
    0x04, 0x05, 0x06, 0x07,
    0x0c, 0x0d, 0x0e, 0x0f
};

// AES Context structure
// it holds the OpenSSL context and the encryption/decryption flag
struct AESContext {
    EVP_CIPHER_CTX* ctx;
    bool encrypt;
};

void init_openssl() {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

void cleanup_openssl() {
    EVP_cleanup();
    ERR_free_strings();
}

AESContext* create_aes_context(bool encrypt) {
    AESContext* aes = new AESContext();
    aes->ctx = EVP_CIPHER_CTX_new();
    aes->encrypt = encrypt;
    EVP_CipherInit_ex(aes->ctx, EVP_aes_256_ctr(), NULL, FIXED_KEY, FIXED_IV, encrypt);
    return aes;
}

// process_chunk function encrypts or decrypts a chunk of data
void process_chunk(AESContext* aes, unsigned char* data, size_t len) {
    int outlen = 0;
    std::vector<unsigned char> outbuf(len);
    // Don't reinitialize the cipher context - it resets the CTR counter
    EVP_CipherUpdate(aes->ctx, outbuf.data(), &outlen, data, len);
    memcpy(data, outbuf.data(), outlen);
}

int main(int argc, char* argv[]) {
    
    string inpath, action, password;
    const string passwordFileName = ".password.txt";

    cout << "Enter the input file path: ";
    getline(cin, inpath);

    cout << "Enter the action (encrypt/decrypt): ";
    getline(cin, action);

    cout << "Enter password: ";
    getline(cin, password);

    fs::path inputPath(inpath);
    fs::path dir = inputPath.parent_path();

    string passwordFilePath = (dir / passwordFileName).string();
    const char* outpath;
    bool encrypt = (action == "encrypt" || action == "ENCRYPT");
    if (encrypt) {
        // saved password in file
        ofstream outFile(passwordFilePath);
        if (!outFile) {
            cout << "Failed to create password file!" << endl;
            return 1;
        }
        outFile << password;
        outFile.close();
    } else if (action == "decrypt" || action == "DECRYPT") {
        // reading and comparing the saved password
        ifstream inFile(passwordFilePath);
        if (!inFile) {
            cout << "Password file not found, cannot decrypt!" << endl;
            return 1;
        }
        string storedPassword;
        getline(inFile, storedPassword);
        inFile.close();
        if (storedPassword != password) {
            cout << "Incorrect password. Access denied!" << endl;
            return 1;
        }
    } else {
        cout << "Unknown action. Please use 'encrypt' or 'decrypt'." << endl;
        return 1;
    }

    fs::path outputPath = dir / (encrypt ? "mid.txt" : "out.txt");
    outpath = outputPath.c_str();
    size_t num_threads = std::thread::hardware_concurrency();

    init_openssl();

    // input file descriptor
    int fd_in = open(inpath.c_str(), O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        return 1;
    }
    struct stat st;
    if (fstat(fd_in, &st) < 0) {
        perror("fstat");
        return 1;
    }
    size_t file_size = st.st_size;
    if (file_size == 0) {
        std::cerr << "Error: input file is empty or size=0\n";
        return 1;
    }

    std::cout << "Input file size: " << file_size << " bytes\n";

    //output file descriptor
    int fd_out = open(outpath, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("open output");
        return 1;
    }
    if (ftruncate(fd_out, file_size) < 0) {
        perror("ftruncate");
        return 1;
    }

    // Memory map the input and output files
    void* in_map = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd_in, 0);
    if (in_map == MAP_FAILED) {
        perror("mmap input");
        return 1;
    }
    void* out_map = mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_out, 0);
    if (out_map == MAP_FAILED) {
        perror("mmap output");
        return 1;
    }

    // Progress tracking
    std::atomic<size_t> bytes_processed(0);
    const size_t chunk = 16 * 1024 * 1024;  // 16 MB slices
    auto start_time = std::chrono::steady_clock::now();

    // thread pool for parallel processing
    {
        ThreadPool pool(num_threads);
        for (size_t off = 0; off < file_size; off += chunk) {
            size_t len = std::min(chunk, file_size - off);
            pool.enqueue([=, &bytes_processed, &start_time]() {
                AESContext* aes = create_aes_context(encrypt);
                auto src  = static_cast<unsigned char*>(in_map)  + off;
                auto dst  = static_cast<unsigned char*>(out_map) + off;
                memcpy(dst, src, len);
                process_chunk(aes, dst, len);
                EVP_CIPHER_CTX_free(aes->ctx);
                delete aes;
                // Update progress
                size_t processed = bytes_processed.fetch_add(len) + len;
                // Print progress every 100MB or on last chunk
                if (processed == file_size || processed / (100*1024*1024) != (processed-len) / (100*1024*1024)) {
                    print_progress(processed, file_size, start_time,true);
                }
            });
        }
        // Wait for all threads to finish by letting ThreadPool go out of scope
    }
    // Print 100% progress at the end
    print_progress(file_size, file_size, start_time,true);
    std::cout << std::endl;

    // unmap the files
    if (munmap(in_map,  file_size) < 0) 
        perror("munmap input");
    if (munmap(out_map, file_size) < 0) 
        perror("munmap output");
    
    close(fd_in);
    close(fd_out);
    cleanup_openssl();

    std::cout << (encrypt ? "Encryption" : "Decryption") << " completed successfully.\n";
    return 0;
}