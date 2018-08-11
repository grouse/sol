/**
 * file:    win32_sol.cpp
 * created: 2018-08-11
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#include <Windows.h>

#define PACKED(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))

#define interlocked_fetch_and_sub(var, val) InterlockedExchangeSubtract(var, val)
#define interlocked_fetch_and_add(var, val) InterlockedExchangeAdd(var, val)

struct ThreadProc
{
    void *(*func)(void*);
    void *data;
};

DWORD thread_proc(void *data)
{
    ThreadProc *proc = (ThreadProc*)data;
    proc->func(proc->data);
    return 0;
}

void* create_thread(void* (*func)(void*), void *data)
{
    SIZE_T stack_size = 1024 * 1024;
    DWORD flags = 0;
    DWORD thread_id;

    ThreadProc proc = {};
    proc.func = func;
    proc.data = data;

    HANDLE thread = CreateThread(NULL, stack_size, &thread_proc, &proc, flags, &thread_id);
    assert(thread != NULL);
    return (void*)thread;
}

void* wait_for_thread(void *handle)
{
    HANDLE thread = (HANDLE)handle;
    DWORD result = WaitForSingleObject(handle, INFINITE);
    assert(result == WAIT_OBJECT_0);
    return nullptr;
}

