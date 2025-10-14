#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>
#include "./src/app/processes/ProcessManagement.hpp"
#include "./src/app/processes/Task.hpp"
#include "./src/app/fileHandling/IO.hpp"
#include "./src/chunk-encryption/utils/progress_utils.h"
#include <atomic>
#include <chrono>

using namespace std;

int main(int argc, char* argv[]) {
    string directory, action, password;
    const string passwordFileName = ".password.txt";

    cout << "Enter the directory path: ";
    getline(cin, directory);

    cout << "Enter the action (encrypt/decrypt): ";
    getline(cin, action);

    cout << "Enter password: ";
    getline(cin, password);

    cout << "yes for multithreading? (y/n): ";
    char threadChoice;
    cin >> threadChoice;
    cin.ignore();
    bool useMultithreading = (threadChoice == 'y' || threadChoice == 'Y');

    // Check if directory exists
    struct stat dirStat;
    if (stat(directory.c_str(), &dirStat) != 0 || !S_ISDIR(dirStat.st_mode)) {
        cout << "❌ Invalid directory path!" << endl;
        return 1;
    }

    string passwordFilePath = directory + "/" + passwordFileName;

    if (action == "encrypt" || action == "ENCRYPT") {
        // Save password in file
        ofstream outFile(passwordFilePath);
        if (!outFile) {
            cout << "❌ Failed to create password file!" << endl;
            return 1;
        }
        outFile << password;
        outFile.close();
    } else if (action == "decrypt" || action == "DECRYPT") {
        // Read and compare password
        ifstream inFile(passwordFilePath);
        if (!inFile) {
            cout << "❌ Password file not found. Cannot decrypt!" << endl;
            return 1;
        }
        string storedPassword;
        getline(inFile, storedPassword);
        inFile.close();

        if (storedPassword != password) {
            cout << "❌ Incorrect password. Access denied!" << endl;
            return 1;
        }
    } else {
        cout << "❌ Unknown action. Please use 'encrypt' or 'decrypt'." << endl;
        return 1;
    }

    // Count total files to process
    size_t total_files = 0;
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG && string(entry->d_name) != passwordFileName) {
                ++total_files;
            }
        }
        closedir(dir);
    }
    cout << "Total files to process: " << total_files << endl;
    std::atomic<size_t> files_processed(0);
    auto start_time = std::chrono::steady_clock::now();

    // Proceed to encryption/decryption
    ProcessManagement processManagement(useMultithreading);

    dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG && string(entry->d_name) != passwordFileName) {
                string filePath = directory + "/" + entry->d_name;
                IO io(filePath);
                fstream f_stream = io.getFileStream();

                if (f_stream.is_open()) {
                    Action taskAction = (action == "encrypt" || action == "ENCRYPT")
                                            ? Action::ENCRYPT
                                            : Action::DECRYPT;

                    auto task = std::make_unique<Task>(std::move(f_stream), taskAction, filePath);
                    processManagement.submitToQueue(std::move(task));
                } else {
                    cout << "❌ Unable to open file: " << filePath << endl;
                }
                // Update and print progress
                size_t processed = files_processed.fetch_add(1) + 1;
                print_progress(processed, total_files, start_time,false);
            }
        }
        closedir(dir);
    }
    print_progress(total_files, total_files, start_time,false);
    cout << endl;

    // Execute tasks
    processManagement.executeTasks();
    return 0;
}