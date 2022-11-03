#pragma once
#include <process.h>

typedef CRITICAL_SECTION pthread_mutex_t;
typedef uintptr_t pthread_t;
typedef struct{
	int nId;
	int detachState;
}pthread_attr_t;

#define PTHREAD_CREATE_DETACHED 0
inline void pthread_attr_init(pthread_attr_t* attr) {
	attr->nId = 0;
}

inline void pthread_attr_setdetachstate(pthread_attr_t* attr, int state) {
	attr->detachState = state;
}

inline void pthread_attr_destroy(pthread_attr_t* attr) {

}

inline void pthread_mutex_init(pthread_mutex_t* pmutex, void* p) {
	InitializeCriticalSection(pmutex);
}

inline void pthread_mutex_destroy(pthread_mutex_t* pmutex) {
	DeleteCriticalSection(pmutex);
}

inline void pthread_mutex_lock(pthread_mutex_t* pmutex) {
	EnterCriticalSection(pmutex);
}

inline void pthread_mutex_unlock(pthread_mutex_t* mutex) {
	LeaveCriticalSection(mutex);
}

typedef void* (*fun_worker)(void* opaque);

typedef struct thread_arg {
	fun_worker func;
	void* args;
}thread_arg;

static inline void thread_fun(void* opaque) {
	thread_arg* pArgs = (thread_arg*)opaque;
	pArgs->func(pArgs->args);
	free(pArgs);
}

inline int pthread_create(pthread_t* tid, pthread_attr_t* attr, fun_worker func, void* args) {
	thread_arg* pArgs = malloc(sizeof(thread_arg));
	pArgs->func = func;
	pArgs->args = args;
	uintptr_t ret = _beginthread(thread_fun, 0, pArgs);
	if (ret) {
		*tid = ret;
		return 0;
	}
	else {
		return -1;
	}
}