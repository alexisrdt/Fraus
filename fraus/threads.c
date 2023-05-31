#include "threads.h"

#include <errno.h>

#ifdef _WIN32

#include <process.h>

#else

#include <unistd.h>

#endif

// Function definitions

/* Thread */

void frGetNumberOfLogicalCores(uint32_t* pCount)
{
#ifdef _WIN32

	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	*pCount = system_info.dwNumberOfProcessors;

#else

	*pCount = sysconf(_SC_NPROCESSORS_ONLN);

#endif
}

FrResult frCreateThread(FrThread* pThread, FrThreadProc proc, void* pArg)
{
	if(!pThread || !proc) return FR_ERROR_INVALID_ARGUMENT;

#ifdef _WIN32

	*pThread = (FrThread)_beginthreadex(NULL, 0, proc, pArg, 0, NULL);
	if(!*pThread) return errno == EACCES ? FR_ERROR_OUT_OF_MEMORY : FR_ERROR_UNKNOWN;

#else

	int res = pthread_create(pThread, NULL, proc, pArg);

	if(res == EAGAIN) return FR_ERROR_OUT_OF_MEMORY;
	if(res != 0) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frJoinThread(FrThread thread, int* pReturnValue)
{
#ifdef _WIN32

	if(WaitForSingleObject(thread, INFINITE) == WAIT_FAILED) return GetLastError() == ERROR_INVALID_HANDLE ? FR_ERROR_INVALID_ARGUMENT : FR_ERROR_UNKNOWN;

	if(pReturnValue)
	{
		if(!GetExitCodeThread(thread, pReturnValue)) return FR_ERROR_UNKNOWN;
	}

	if(!CloseHandle(thread)) return GetLastError() == ERROR_INVALID_HANDLE ? FR_ERROR_INVALID_ARGUMENT : FR_ERROR_UNKNOWN;

#else

	int res = pthread_join(thread, pReturnValue);
	
	if(res == EINVAL || res == EDEADLK) return FR_ERROR_INVALID_ARGUMENT;
	if(res != 0) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

/* Mutex */

FrResult frCreateMutex(FrMutex* pMutex)
{
	if(!pMutex) return FR_ERROR_INVALID_ARGUMENT;

#ifdef _WIN32

	InitializeCriticalSection(pMutex);

#else

	int res = pthread_mutex_init(pMutex, NULL);

	if(res == ENOMEM) return FR_ERROR_OUT_OF_MEMORY;
	if(res == EBUSY) return FR_ERROR_INVALID_ARGUMENT;
	if(res != 0) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frLockMutex(FrMutex* pMutex)
{
	if(!pMutex) return FR_ERROR_INVALID_ARGUMENT;

#ifdef _WIN32

	EnterCriticalSection(pMutex);

#else

	int res = pthread_mutex_lock(pMutex);

	if(res == EINVAL || res == EDEADLK) return FR_ERROR_INVALID_ARGUMENT;
	if(res != 0) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frUnlockMutex(FrMutex* pMutex)
{
	if(!pMutex) return FR_ERROR_INVALID_ARGUMENT;

#ifdef _WIN32

	LeaveCriticalSection(pMutex);

#else

	int res = pthread_mutex_unlock(pMutex);

	if(res == EINVAL || res == EPERM) return FR_ERROR_INVALID_ARGUMENT;
	if(res != 0) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}

FrResult frDestroyMutex(FrMutex* pMutex)
{
	if(!pMutex) return FR_ERROR_INVALID_ARGUMENT;

#ifdef _WIN32

	DeleteCriticalSection(pMutex);

#else

	int res = pthread_mutex_destroy(pMutex);

	if(res == EBUSY || res == EINVAL) return FR_ERROR_INVALID_ARGUMENT;
	if(res != 0) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}
