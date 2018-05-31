#include "threadPool.h"

/**
 * Tries to execute tasks in tasks queue.
 * @param threadPool The thread pool.
 * @return n/a
 */
void *execute(void *threadPool) {
  ThreadPool *pool = threadPool;

  while (pool->shouldWaitForTasks) {
    pthread_mutex_lock(&pool->mutex);

    // Checks if queue is empty.
    if (osIsQueueEmpty(pool->tasksQueue)) {
      if (!pool->isActive) {
        pthread_mutex_unlock(&pool->mutex);
        break;
      }

      pthread_cond_wait(&pool->cond, &pool->mutex);
      pthread_mutex_unlock(&pool->mutex);
      continue;
    }

    // Queue is not empty.
    Task *taskToRun = osDequeue(pool->tasksQueue);
    pthread_mutex_unlock(&pool->mutex);
    taskToRun->computeFunc(taskToRun->parameter);

    free(taskToRun);
  }
}

/**
 * Writes an error message to STDERR.
 * @param message The message to write.
 */
void writeError(const char *message) {
  write(STDERR_FILENO, message, strlen(message));
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
  int i;
  threadPool->shouldWaitForTasks = shouldWaitForTasks;
  threadPool->isActive = 0;

  if (pthread_cond_broadcast(&threadPool->cond) != 0 || pthread_mutex_unlock
      (&threadPool->mutex) != 0) {
    writeError(INIT_COND_OR_MUTEX_ERR);
    exit(1);
  }

  // Waits for threads to finish and deallocates
  for (i = 0; i < threadPool->size; i++) {
    pthread_t *test = threadPool->threadsList[i];
    pthread_join(*test, NULL);
    free(threadPool->threadsList[i]);
  }

  free(threadPool->threadsList);
  osDestroyQueue(threadPool->tasksQueue);

  free(threadPool);
}

int tpInsertTask(ThreadPool *pool, void (*computeFunc)(void *), void *param) {

  // Returns 1 if thread pool is being destroyed.
  if (!pool->isActive) {
    return 1;
  }

  Task *t = malloc(sizeof(Task));
  t->computeFunc = computeFunc;
  t->parameter = param;

  pthread_mutex_lock(&pool->mutex);
  osEnqueue(pool->tasksQueue, t);
  if (pthread_cond_signal(&pool->cond) != 0) {
    writeError(MEM_ALLOCATION_ERR);
    exit(1);
  }
  pthread_mutex_unlock(&pool->mutex);

  return 0;
}

ThreadPool *tpCreate(int numOfThreads) {
  int i;
  int j;

  ThreadPool *pool = NULL;
  if ((pool = malloc(sizeof(ThreadPool))) == NULL) {
    writeError(MEM_ALLOCATION_ERR);
    exit(1);
  }

  pool->isActive = 1;
  pool->shouldWaitForTasks = 1;
  pool->size = numOfThreads;

  // Creates tasks queue
  if ((pool->tasksQueue = osCreateQueue()) == NULL) {
    writeError(MEM_ALLOCATION_ERR);
    exit(1);
  }

  // Initializes mutex and conditional variable
  if (pthread_mutex_init(&pool->mutex, NULL) != 0 ||
      pthread_cond_init(&pool->cond, NULL) != 0) {
    writeError(INIT_COND_OR_MUTEX_ERR);
  }

  // Allocates pthreads array
  if ((pool->threadsList = malloc(sizeof(pthread_t *) * numOfThreads))
      == NULL) {
    writeError(MEM_ALLOCATION_ERR);
    free(pool);
    exit(1);
  }

  // Allocates space for pthreads and creates them
  for (i = 0; i < numOfThreads; i++) {
    if ((pool->threadsList[i] = malloc(sizeof(pthread_t))) == NULL) {

      // Deallocates successfully allocated threads
      for (j = 0; j < i; j++) {
        pthread_cancel((pthread_t) pool->threadsList[j]);
        pthread_join((pthread_t) pool->threadsList[j], NULL);
        free(pool->threadsList[j]);
      }
      free(pool);
      writeError(MEM_ALLOCATION_ERR);
      exit(1);
    }

    if (pthread_create(pool->threadsList[i], NULL, execute, pool) != 0) {

      // Deallocates successfully allocated threads.
      free(pool->threadsList[i]);
      for (j = 0; j < i; j++) {
        pthread_cancel((pthread_t) pool->threadsList[j]);
        pthread_join((pthread_t) pool->threadsList[j], NULL);
        free(pool->threadsList[j]);
      }
      free(pool);
      writeError(MEM_ALLOCATION_ERR);
      exit(1);
    }
  }

  return pool;
}



