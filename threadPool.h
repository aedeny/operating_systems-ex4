#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <zconf.h>
#include <wchar.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "osqueue.h"

// Errors
#define MEM_ALLOCATION_ERR "Error: Cannot allocate memory."
#define INIT_COND_OR_MUTEX_ERR "Error: Could not initialize cond or mutex."

typedef struct thread_pool {
  int size;
  int isActive;
  int shouldWaitForTasks;
  pthread_t **threadsList;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct os_queue *tasksQueue;

} ThreadPool;

typedef struct task {
  void (*computeFunc)(void *);
  char *parameter;
} Task;

/**
 * Creates thread pool.
 * @param numOfThreads Number of threads in the thread pool.
 * @return An initialized thread pool with numOfThreads threads.
 */
ThreadPool *tpCreate(int numOfThreads);

/**
 * Destroys thread pool and deallocates all allocated memory.
 * @param threadPool The thread pool to be destroyed.
 * @param shouldWaitForTasks If 0, doesn't wait for tasks in tasks queue.
 * Otherwise, waits for all tasks in queue to finish.
 */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

/**
 * Inserts given function with parameters into tasks queue to be handled by
 * thread pool.
 * @param pool The thread pool.
 * @param computeFunc The function.
 * @param param The function's parameters.
 * @return 0 on success, 1 otherwise.
 */
int tpInsertTask(ThreadPool *pool,
                 void (*computeFunc)(void *),
                 void *param);


#endif
