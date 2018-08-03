/**
 * file:    linux_sol.cpp
 * created: 2018-08-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#include <pthread.h>

#define PACKED(decl) decl __attribute__((__packed__))

#define interlocked_fetch_and_sub(var, val) __sync_fetch_and_sub(var, val)

void* create_thread(void* (*func)(void*), void *data)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_t *thread = (pthread_t*)malloc(sizeof *thread);
    pthread_create(thread, &attr, func, data);
    return (void*)thread;
}

void* wait_for_thread(void *handle)
{
    pthread_t *thread = (pthread_t*)handle;

    void *result = nullptr;
    pthread_join(*thread, &result);

    return result;
}

