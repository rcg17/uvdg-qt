
#ifndef __MUTEX_H__
#define __MUTEX_H__

#ifdef _MSC_VER

#include <Windows.h>
typedef HANDLE Mutex;

#else

#include <mutex>
typedef std::mutex Mutex;

#endif

void MutexCreate(Mutex *mutex);
void MutexDestroy(Mutex *mutex);
void MutexLock(Mutex *mutex);
void MutexUnlock(Mutex *mutex);

#endif
