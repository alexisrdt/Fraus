#ifndef FRAUS_THREADS_H
#define FRAUS_THREADS_H

/*
 * Fraus threading
 * Win32 and pthread supported
 */

#include <stdint.h>

// Include FrResult

#include "utils.h"

// Type definitions

/*
 * Thread procedure
 * Chose to return int instead of void* as pthread does to match main function behaviour
 */
typedef int(*FrThreadProc)(void*);

#ifdef _WIN32

#include <windows.h>

typedef HANDLE FrThread;
typedef CRITICAL_SECTION FrMutex;

#else

#include <pthread.h>

typedef pthread_t FrThread;
typedef pthread_mutex_t FrMutex;

#endif

// Function declarations

/* Thread */

FrResult frGetNumberOfLogicalCores(uint32_t* pCount);

FrResult frCreateThread(FrThread* pThread, FrThreadProc proc, void* pArg);
FrResult frJoinThread(FrThread thread, int* pReturnValue);

/* Mutex */

FrResult frCreateMutex(FrMutex* pMutex);
FrResult frLockMutex(FrMutex* pMutex);
FrResult frUnlockMutex(FrMutex* pMutex);
FrResult frDestroyMutex(FrMutex* pMutex);

#endif
