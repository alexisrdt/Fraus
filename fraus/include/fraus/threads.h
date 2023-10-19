#ifndef FRAUS_THREADS_H
#define FRAUS_THREADS_H

/*
 * Fraus threading
 * Win32 and pthread supported
 */

#include <stdint.h>

// Include FrResult

#include "./utils.h"

// Type definitions

/*
 * Thread procedure
 * Chose to return int instead of void* as pthread does to match main function behaviour
 */
typedef int(*FrThreadProc)(void* pData);

#ifdef _WIN32

#include <windows.h>

typedef HANDLE FrThread;
typedef CRITICAL_SECTION FrMutex;
typedef CONDITION_VARIABLE FrConditionVariable;

#else

#include <threads.h>

typedef thrd_t FrThread;
typedef mtx_t FrMutex;
typedef cnd_t FrConditionVariable;

#endif

// Function declarations

/* Thread */

uint32_t frGetNumberOfLogicalCores(void);

FrResult frCreateThread(FrThread* pThread, FrThreadProc proc, void* pArg);
FrResult frJoinThread(FrThread thread, int* pReturnValue);

/* Mutex */

FrResult frInitializeMutex(FrMutex* pMutex);
FrResult frLockMutex(FrMutex* pMutex);
FrResult frUnlockMutex(FrMutex* pMutex);
void frDestroyMutex(FrMutex* pMutex);

/* Condition variable */

FrResult frInitializeConditionVariable(FrConditionVariable* pConditionVariable);
FrResult frWaitConditionVariable(FrConditionVariable* pConditionVariable, FrMutex* pMutex);
FrResult frSignalConditionVariable(FrConditionVariable* pConditionVariable);
FrResult frBroadcastConditionVariable(FrConditionVariable* pConditionVariable);
void frDestroyConditionVariable(FrConditionVariable* pConditionVariable);

#endif
