#ifndef HbInclude_HbPara
#define HbInclude_HbPara
#include "HbReport.h"
#if defined(HbPlatform_OS_Microsoft)
#include <Windows.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

/********
 * Mutex
 ********/
typedef struct HbPara_Mutex {
	#if defined(HbPlatform_OS_Microsoft)
	CRITICAL_SECTION microsoftCriticalSection_i;
	#else
	#error HbPara_Mutex: No implementation for the target OS.
	#endif
} HbPara_Mutex;
HbForceInline void HbPara_Mutex_Init(HbPara_Mutex * const mutex, HbBool requireRecursive) {
	HbReport_Assert_Assume(mutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	HbUnused(requireRecursive); // Always recursive.
	InitializeCriticalSection(&mutex->microsoftCriticalSection_i);
	#else
	#error HbPara_Mutex_Init: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Mutex_Shutdown(HbPara_Mutex * const mutex) {
	HbReport_Assert_Assume(mutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	DeleteCriticalSection(&mutex->microsoftCriticalSection_i);
	#else
	#error HbPara_Mutex_Shutdown: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Mutex_Lock(HbPara_Mutex * const mutex) {
	HbReport_Assert_Assume(mutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	EnterCriticalSection(&mutex->microsoftCriticalSection_i);
	#else
	#error HbPara_Mutex_Lock: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Mutex_Unlock(HbPara_Mutex * const mutex) {
	HbReport_Assert_Assume(mutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	LeaveCriticalSection(&mutex->microsoftCriticalSection_i);
	#else
	#error HbPara_Mutex_Unlock: No implementation for the target OS.
	#endif
}

/*********************
 * Reader/writer lock
 * Not recursive!
 *********************/
typedef struct HbPara_RWMutex {
	#if defined(HbPlatform_OS_Microsoft)
	SRWLOCK microsoftSRWLock_i;
	#else
	#error HbPara_RWMutex: No implementation for the target OS.
	#endif
} HbPara_RWMutex;
HbForceInline void HbPara_RWMutex_Init(HbPara_RWMutex * const rwMutex) {
	HbReport_Assert_Assume(rwMutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	InitializeSRWLock(&rwMutex->microsoftSRWLock_i);
	#else
	#error HbPara_RWMutex_Init: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_RWMutex_Shutdown(HbPara_RWMutex * const rwMutex) {
	HbReport_Assert_Assume(rwMutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	HbUnused(rwMutex);
	#else
	#error HbPara_RWMutex_Shutdown: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_RWMutex_LockRead(HbPara_RWMutex * const rwMutex) {
	HbReport_Assert_Assume(rwMutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	AcquireSRWLockShared(&rwMutex->microsoftSRWLock_i);
	#else
	#error HbPara_RWMutex_LockRead: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_RWMutex_UnlockRead(HbPara_RWMutex * const rwMutex) {
	HbReport_Assert_Assume(rwMutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	ReleaseSRWLockShared(&rwMutex->microsoftSRWLock_i);
	#else
	#error HbPara_RWMutex_UnlockRead: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_RWMutex_LockWrite(HbPara_RWMutex * const rwMutex) {
	HbReport_Assert_Assume(rwMutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	AcquireSRWLockExclusive(&rwMutex->microsoftSRWLock_i);
	#else
	#error HbPara_RWMutex_LockWrite: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_RWMutex_UnlockWrite(HbPara_RWMutex * const rwMutex) {
	HbReport_Assert_Assume(rwMutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	ReleaseSRWLockExclusive(&rwMutex->microsoftSRWLock_i);
	#else
	#error HbPara_RWMutex_UnlockWrite: No implementation for the target OS.
	#endif
}

/*********************
 * Condition variable
 *********************/
typedef struct HbPara_Cond {
	#if defined(HbPlatform_OS_Microsoft)
	CONDITION_VARIABLE microsoftConditionVariable_i;
	#else
	#error HbPara_Cond: No implementation for the target OS.
	#endif
} HbPara_Cond;
HbForceInline void HbPara_Cond_Init(HbPara_Cond * const cond) {
	HbReport_Assert_Assume(cond != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	InitializeConditionVariable(&cond->microsoftConditionVariable_i);
	#else
	#error HbPara_Cond_Init: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Cond_Shutdown(HbPara_RWMutex * const cond) {
	HbReport_Assert_Assume(cond != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	HbUnused(cond);
	#else
	#error HbPara_Cond_Shutdown: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Cond_NotifyOne(HbPara_Cond * const cond) {
	HbReport_Assert_Assume(cond != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	WakeConditionVariable(&cond->microsoftConditionVariable_i);
	#else
	#error HbPara_Cond_NotifyOne: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Cond_NotifyAll(HbPara_Cond * const cond) {
	HbReport_Assert_Assume(cond != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	WakeAllConditionVariable(&cond->microsoftConditionVariable_i);
	#else
	#error HbPara_Cond_NotifyAll: No implementation for the target OS.
	#endif
}
HbForceInline void HbPara_Cond_Wait(HbPara_Cond * const cond, HbPara_Mutex * const mutex) {
	HbReport_Assert_Assume(cond != NULL);
	HbReport_Assert_Assume(mutex != NULL);
	#if defined(HbPlatform_OS_Microsoft)
	SleepConditionVariableCS(&cond->microsoftConditionVariable_i, &mutex->microsoftCriticalSection_i, INFINITE);
	#else
	#error HbPara_Cond_Wait: No implementation for the target OS.
	#endif
}

#ifdef __cplusplus
}
#endif
#endif
