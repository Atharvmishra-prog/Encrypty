#include <iostream>
#include "ProcessManagement.hpp"
#include <unistd.h>
#include <cstring>
#include "../encryptDecrypt/Cryption.hpp"
#include <sys/mman.h>
#include <atomic>
#include <fcntl.h>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <vector>
#include <sys/wait.h>

using namespace std;

ProcessManagement::ProcessManagement(bool useMultithreading)
    : useMultithreading(useMultithreading) {
    itemsSemaphore = sem_open("/items_semaphore", O_CREAT, 0666, 0);
    empty_semaphore = sem_open("/empty_slots_semaphore", O_CREAT, 0666, 1000);
    // if (itemsSemaphore == SEM_FAILED || emptySlotsSemaphore == SEM_FAILED) {
    //     perror("sem_open failed");
    //     exit(EXIT_FAILURE);
    // }
    shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    // if (shmFd == -1) {
    //     perror("shm_open failed");
    //     exit(EXIT_FAILURE);
    // }
    ftruncate(shmFd, sizeof(SharedMemory));
    
    sharedMem = static_cast<SharedMemory *>(mmap(
        nullptr, sizeof(SharedMemory),
        PROT_READ | PROT_WRITE,
        MAP_SHARED, shmFd, 0
    ));

    sharedMem->front = 0;
    sharedMem->rear = 0;
    sharedMem->size.store(0);
}

bool ProcessManagement::submitToQueue(unique_ptr<Task> task) {
    sem_wait(empty_semaphore);
    unique_lock<mutex> lock(queueLock);

    if (sharedMem->size.load() >= 1000) {
        return false;
    }

    strcpy(sharedMem->tasks[sharedMem->rear], task->toString().c_str());
    sharedMem->rear = (sharedMem->rear + 1) % 1000;
    sharedMem->size.fetch_add(1);

    lock.unlock();
    sem_post(itemsSemaphore);

    // Don't create threads/processes here - just queue the task
    return true;
} 

void ProcessManagement::executeTasks() {
    if (useMultithreading) {
        // Use multiple threads to process tasks
        size_t num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this]() {
                while (sharedMem->size.load() > 0) {
                    sem_wait(itemsSemaphore);
                    unique_lock<mutex> lock(queueLock);
                    
                    if (sharedMem->size.load() == 0) {
                        sem_post(itemsSemaphore);
                        lock.unlock();
                        break;
                    }
                    
                    char taskStr[256];
                    strcpy(taskStr, sharedMem->tasks[sharedMem->front]);
                    sharedMem->front = (sharedMem->front + 1) % 1000;
                    sharedMem->size.fetch_sub(1);
                    
                    sem_post(empty_semaphore);
                    lock.unlock();
                    
                    executeCryption(taskStr);
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    } else {
        // Use multiple processes to process tasks
        size_t num_processes = std::thread::hardware_concurrency();
        
        for (size_t i = 0; i < num_processes; ++i) {
            int pid = fork();
            if (pid < 0) {
                continue;
            } else if (pid == 0) {
                // Child process
                while (sharedMem->size.load() > 0) {
                    sem_wait(itemsSemaphore);
                    unique_lock<mutex> lock(queueLock);
                    
                    if (sharedMem->size.load() == 0) {
                        sem_post(itemsSemaphore);
                        lock.unlock();
                        break;
                    }
                    
                    char taskStr[256];
                    strcpy(taskStr, sharedMem->tasks[sharedMem->front]);
                    sharedMem->front = (sharedMem->front + 1) % 1000;
                    sharedMem->size.fetch_sub(1);
                    
                    sem_post(empty_semaphore);
                    lock.unlock();
                    
                    executeCryption(taskStr);
                }
                exit(0);
            }
        }
        
        // Wait for all child processes
        for (size_t i = 0; i < num_processes; ++i) {
            wait(nullptr);
        }
    }
}

ProcessManagement::~ProcessManagement() {
    munmap(sharedMem, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);
}