// Copyright (c) 2017-2020 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)

#include <windows.h>
#include "vm_semaphore.h"
#include "limits.h"

/* Invalidate a semaphore */
void vm_semaphore_set_invalid(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return;

    sem->handle = NULL;

} /* void vm_semaphore_set_invalid(vm_semaphore *sem) */

/* Verify if a semaphore is valid */
int32_t vm_semaphore_is_valid(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return 0;

    return (int32_t)(0 != sem->handle);

} /* int32_t vm_semaphore_is_valid(vm_semaphore *sem) */

/* Init a semaphore with value */
vm_status vm_semaphore_init(vm_semaphore *sem, int32_t count)
{
    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

#if (_WIN32_WINNT >= 0x0600)
    sem->handle = CreateSemaphoreEx(NULL, count, LONG_MAX, 0, 0, 0);
#else
    sem->handle = CreateSemaphore(NULL, count, LONG_MAX, 0);
#endif
    return (0 != sem->handle) ? VM_OK : VM_OPERATION_FAILED;

} /* vm_status vm_semaphore_init(vm_semaphore *sem, int32_t count) */

/* Init a semaphore with value */
vm_status vm_semaphore_init_max(vm_semaphore *sem, int32_t count, int32_t max_count)
{
    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

#if (_WIN32_WINNT >= 0x0600)
    sem->handle = CreateSemaphoreEx(NULL, count, max_count, 0, 0, 0);
#else
    sem->handle = CreateSemaphore(NULL, count, max_count, 0);
#endif
    return (0 != sem->handle) ? VM_OK : VM_OPERATION_FAILED;

} /* vm_status vm_semaphore_init_max(vm_semaphore *sem, int32_t count, int32_t max_count) */

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_timedwait(vm_semaphore *sem, uint32_t msec)
{
    vm_status umcRes = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (sem->handle)
    {
        uint32_t dwRes = WaitForSingleObject(sem->handle, msec);
        umcRes = VM_OK;

        if (WAIT_TIMEOUT == dwRes)
            umcRes = VM_TIMEOUT;
        else if (WAIT_OBJECT_0 != dwRes)
            umcRes = VM_OPERATION_FAILED;
    }
    return umcRes;

} /* vm_status vm_semaphore_timedwait(vm_semaphore *sem, uint32_t msec) */

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_wait(vm_semaphore *sem)
{
    vm_status umcRes = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (sem->handle)
    {
        umcRes = VM_OK;
        if (WAIT_OBJECT_0 != WaitForSingleObject(sem->handle, INFINITE))
            umcRes = VM_OPERATION_FAILED;
    }
    return umcRes;

} /* vm_status vm_semaphore_wait(vm_semaphore *sem) */

/* Decrease the semaphore value without blocking, return 1 if success */
vm_status vm_semaphore_try_wait(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    return vm_semaphore_timedwait(sem, 0);

} /* vm_status vm_semaphore_try_wait(vm_semaphore *sem) */

/* Increase the semaphore value */
vm_status vm_semaphore_post(vm_semaphore *sem)
{
    vm_status umcRes = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (sem->handle)
    {
        if (ReleaseSemaphore(sem->handle, 1, NULL))
            umcRes = VM_OK;
        else
            umcRes = VM_OPERATION_FAILED;
    }
    return umcRes;

} /* vm_status vm_semaphore_post(vm_semaphore *sem) */

/* Increase the semaphore value */
vm_status vm_semaphore_post_many(vm_semaphore *sem, int32_t post_count)
{
    vm_status umcRes = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (sem->handle)
    {
        if (ReleaseSemaphore(sem->handle, post_count, NULL))
            umcRes = VM_OK;
        else
            umcRes = VM_OPERATION_FAILED;
    }
    return umcRes;

} /* vm_status vm_semaphore_post_many(vm_semaphore *sem, int32_t post_count) */

/* Destroy a semaphore */
void vm_semaphore_destroy(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return;

    if (sem->handle)
    {
        CloseHandle(sem->handle);
        sem->handle = NULL;
    }
} /* void vm_semaphore_destroy(vm_semaphore *sem) */

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */
