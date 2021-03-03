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
#include "vm_event.h"

/* Invalidate an event */
void vm_event_set_invalid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    event->handle = NULL;

} /* void vm_event_set_invalid(vm_event *event) */

/* Verify if the event handle is valid */
int32_t vm_event_is_valid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return 0;

    return (int32_t)(NULL != event->handle);

} /* int32_t vm_event_is_valid(vm_event *event) */

/* Init an event. Event is created unset. return 1 if success */
vm_status vm_event_init(vm_event *event, int32_t manual, int32_t state)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    vm_event_destroy(event);
    event->handle = CreateEvent(NULL, manual, state, NULL);

    if (NULL == event->handle)
        return VM_OPERATION_FAILED;
    else
        return VM_OK;

} /* vm_status vm_event_init(vm_event *event, int32_t manual, int32_t state) */

vm_status vm_event_named_init(vm_event *event,
                              int32_t manual, int32_t state, const char *pcName)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    vm_event_destroy(event);
    event->handle = CreateEventA(NULL, manual, state, pcName);

    if (NULL == event->handle)
        return VM_OPERATION_FAILED;
    else
        return VM_OK;

} /* vm_status vm_event_named_init(vm_event *event, */

/* Set the event to either HIGH (1) or LOW (0) state */
vm_status vm_event_signal(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (SetEvent(event->handle))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;

} /* vm_status vm_event_signal(vm_event *event) */

/* Set the event to either HIGH (1) or LOW (0) state */
vm_status vm_event_reset(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (ResetEvent(event->handle))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;

} /* vm_status vm_event_reset(vm_event *event) */

/* Pulse the event 0 -> 1 -> 0 */
vm_status vm_event_pulse(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

#if !defined(WIN_TRESHOLD_MOBILE)
    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (PulseEvent(event->handle))
        return VM_OK;
    else
#endif

    return VM_OPERATION_FAILED;

} /* vm_status vm_event_pulse(vm_event *event) */

/* Wait for event to be high with blocking */
vm_status vm_event_wait(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (WAIT_OBJECT_0 == WaitForSingleObject(event->handle, INFINITE))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;

} /* vm_status vm_event_wait(vm_event *event) */

/* Wait for event to be high without blocking, return 1 if signaled */
vm_status vm_event_timed_wait(vm_event *event, uint32_t msec)
{
    vm_status umcRes = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL != event->handle)
    {
        uint32_t dwRes = WaitForSingleObject(event->handle, msec);

        if (WAIT_OBJECT_0 == dwRes)
            umcRes = VM_OK;
        else if (WAIT_TIMEOUT == dwRes)
            umcRes = VM_TIMEOUT;
        else
            umcRes = VM_OPERATION_FAILED;
    }
    return umcRes;

} /* vm_status vm_event_timed_wait(vm_event *event, uint32_t msec) */

/* Destroy the event */
void vm_event_destroy(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    if (event->handle)
    {
        CloseHandle(event->handle);
        event->handle = NULL;
    }

} /* void vm_event_destroy(vm_event *event) */

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */
