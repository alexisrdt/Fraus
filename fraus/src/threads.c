#include "../include/fraus/threads.h"

#include <errno.h>

#ifdef _WIN32

#include <process.h>

#else

#include <unistd.h>

#endif

// Function definitions

/* Thread */

uint32_t frGetNumberOfLogicalCores(void)
{
#ifdef _WIN32

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	return systemInfo.dwNumberOfProcessors;

#else

	return sysconf(_SC_NPROCESSORS_ONLN);

#endif
}

FrResult frCreateThread(FrThread* pThread, FrThreadProc proc, void* pArg)
{
#ifdef _WIN32

	*pThread = (FrThread)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)proc, pArg, 0, NULL);
	if(!*pThread) return errno == EACCES ? FR_ERROR_OUT_OF_MEMORY : FR_ERROR_UNKNOWN;

#else

	int res = thrd_create(pThread, proc, pArg);

	if(res == thrd_nomem) return FR_ERROR_OUT_OF_MEMORY;
	if(res != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frJoinThread(FrThread thread, int* pReturnValue)
{
#ifdef _WIN32

	if(WaitForSingleObject(thread, INFINITE) == WAIT_FAILED) return GetLastError() == ERROR_INVALID_HANDLE ? FR_ERROR_INVALID_ARGUMENT : FR_ERROR_UNKNOWN;

	if(pReturnValue)
	{
		if(!GetExitCodeThread(thread, (LPDWORD)pReturnValue)) return FR_ERROR_UNKNOWN;
	}

	if(!CloseHandle(thread)) return GetLastError() == ERROR_INVALID_HANDLE ? FR_ERROR_INVALID_ARGUMENT : FR_ERROR_UNKNOWN;

#else

	if(thrd_join(thread, pReturnValue) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

/* Mutex */

FrResult frInitializeMutex(FrMutex* pMutex)
{
#ifdef _WIN32

	InitializeCriticalSection(pMutex);

#else

	if(mtx_init(pMutex, mtx_plain) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frLockMutex(FrMutex* pMutex)
{
#ifdef _WIN32

	EnterCriticalSection(pMutex);

#else

	if(mtx_lock(pMutex) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frUnlockMutex(FrMutex* pMutex)
{
#ifdef _WIN32

	LeaveCriticalSection(pMutex);

#else

	if(mtx_unlock(pMutex) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

void frDestroyMutex(FrMutex* pMutex)
{
#ifdef _WIN32

	DeleteCriticalSection(pMutex);

#else

	mtx_destroy(pMutex);

#endif
}

/* Condition variable */

FrResult frInitializeConditionVariable(FrConditionVariable* pConditionVariable)
{
#ifdef _WIN32

	InitializeConditionVariable(pConditionVariable);

#else

	int res = cnd_init(pConditionVariable);

	if(res == thrd_nomem) return FR_ERROR_OUT_OF_MEMORY;
	if(res != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frWaitConditionVariable(FrConditionVariable* pConditionVariable, FrMutex* pMutex)
{
#ifdef _WIN32

	if(!SleepConditionVariableCS(pConditionVariable, pMutex, INFINITE)) return FR_ERROR_UNKNOWN;

#else

	if(cnd_wait(pConditionVariable, pMutex) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frSignalConditionVariable(FrConditionVariable* pConditionVariable)
{
#ifdef _WIN32

	WakeConditionVariable(pConditionVariable);

#else

	if(cnd_signal(pConditionVariable) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frBroadcastConditionVariable(FrConditionVariable* pConditionVariable)
{
#ifdef _WIN32

	WakeAllConditionVariable(pConditionVariable);

#else

	if(cnd_broadcast(pConditionVariable) != thrd_success) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

void frDestroyConditionVariable(FrConditionVariable* pConditionVariable)
{
#ifndef _WIN32

	cnd_destroy(pConditionVariable);

#else

	(void)pConditionVariable;

#endif
}
