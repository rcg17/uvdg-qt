
#include "Mutex.h"

#ifdef _MSC_VER

void MutexCreate(Mutex *mutex)
{
    *mutex = CreateMutex(NULL, FALSE, NULL);
}

void MutexDestroy(Mutex *mutex)
{
    CloseHandle(*mutex);
}

void MutexLock(Mutex *mutex)
{
    // not needed as all QTcpSocket stuff performs on main thread
    // WaitForSingleObject(*mutex, INFINITE);
}

void MutexUnlock(Mutex *mutex)
{
    // ReleaseMutex(*mutex);
}

#else

void MutexCreate(Mutex *mutex) {}

void MutexDestroy(Mutex *mutex) {}

void MutexLock(Mutex *mutex)
{
    mutex->lock();
}

void MutexUnlock(Mutex *mutex)
{
    mutex->unlock();
}

#endif
