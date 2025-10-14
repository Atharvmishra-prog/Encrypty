#ifndef PROCESS_MANAGEMENT_HPP
#define PROCESS_MANAGEMENT_HPP
#include <atomic>
#include "Task.hpp"
#include <queue>
#include <memory>
#include <semaphore.h>
#include <mutex>

using namespace std;

class ProcessManagement
{

public:
    ProcessManagement(bool useMultithreading = false); //just initialises queue-constructor
    ~ProcessManagement();
    bool submitToQueue(unique_ptr<Task> task); //comes from <memory> package & Adds tasks to the queue
    void executeTasks(); //// Executes tasks in the queue

private:   
    // Queue to hold tasks
    //AOut) structure for task scheduling.
    //Stores tasks FIFO (First-In-First- dynamically using unique_ptr, preventing memory leaks.

    //queue<unique_ptr<Task>> taskQueue;
    //(Multiprocessing-->)queue is not gettimg shared among processes, causing infinite loop
    //we will now create a shared memory struct

    struct SharedMemory {
        atomic<int>size;
        char tasks[1000][256];
        atomic<int> front;
        atomic<int> rear;

        void printSharedMemory() {
            cout<<size<<endl;
            cout<<front<<endl;
            cout<<rear<<endl;
        }

    };
    SharedMemory* sharedMem;
    int shmFd;

      sem_t* itemsSemaphore;
      sem_t* empty_semaphore; 

    const char* SHM_NAME = "/my_queue";
    mutex queueLock;

    bool useMultithreading;

};

#endif
/*
unique_ptr<Task> ensures automatic memory management.
When a Task is added to taskQueue, it cannot be shared elsewhere (no accidental memory leaks).
unique_ptr means only one owner for each Task—it will be destroyed when no longer needed.
*/