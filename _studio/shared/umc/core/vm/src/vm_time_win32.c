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

#include "vm_time.h"

#define VM_TIMEOP(A, B, C, OP) \
{ \
    A = vvalue(B); \
    A = A OP vvalue(C); \
}

#define VM_TIMEDEST \
  destination[0].tv_sec = (uint32_t)(cv0 / 1000000); \
  destination[0].tv_usec = (long)(cv0 % 1000000)

static unsigned long long vvalue( struct vm_timeval* B )
{
  unsigned long long rtv;
  rtv = B[0].tv_sec;
  return ((rtv * 1000000) + B[0].tv_usec);
}


/* common (Linux and Windows time functions
 * may be placed before compilation fence. */
void vm_time_timeradd(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2)
{
  unsigned long long cv0;
  VM_TIMEOP(cv0, src1, src2, + );
  VM_TIMEDEST;
}

void vm_time_timersub(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2)
{
  unsigned long long cv0;
  VM_TIMEOP(cv0, src1, src2, - );
  VM_TIMEDEST;
}

int vm_time_timercmp(struct vm_timeval* src1, struct vm_timeval* src2, struct vm_timeval *threshold)
{
  if (threshold == NULL)
    return ( ((unsigned long long *)src1)[0] == ((unsigned long long *)src2)[0] ) ? 1 : 0;
  else
  {
    unsigned long long val1 = vvalue(src1);
    unsigned long long val2 = vvalue(src2);
    unsigned long long vald = (val1 > val2) ? (val1 - val2) : (val2 - val1);
    return ( vald < vvalue(threshold) ) ? 1 : 0;
  }
}

#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)

#include <windows.h>
#include "time.h"
#include "winbase.h"

#pragma comment(lib, "winmm.lib")

/* yield the execution of current thread for msec milliseconds */
void vm_time_sleep(uint32_t msec)
{
#ifndef _WIN32_WCE
  if (msec) {
    Sleep(msec);
    }
  else
    SwitchToThread();
#else
  Sleep(msec);  /* always Sleep for WinCE */
#endif

} /* void vm_time_sleep(uint32_t msec)  */

uint32_t vm_time_get_current_time(void)
{
#if defined(WIN_TRESHOLD_MOBILE)
    // uses GetTickCount on THM less accurate but no dependecy to winmm.dll
    return (uint32_t) GetTickCount();
#else
    return (uint32_t)timeGetTime();
#endif
} /* uint32_t vm_time_get_current_time(void) */

/* obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void)
{
    LARGE_INTEGER t1;

    QueryPerformanceCounter(&t1);
    return t1.QuadPart;

} /* vm_tick vm_time_get_tick(void) */

/* obtain the clock resolution */
vm_tick vm_time_get_frequency(void)
{
    LARGE_INTEGER t1;

    QueryPerformanceFrequency(&t1);
    return t1.QuadPart;

} /* vm_tick vm_time_get_frequency(void) */

/* Create the object of time measure */
vm_status vm_time_open(vm_time_handle *handle)
{
   if (NULL == handle)
       return VM_NULL_PTR;
   *handle = -1;
   return VM_OK;

} /* vm_status vm_time_open(vm_time_handle *handle); */

/* Close the object of time measure */
vm_status vm_time_close(vm_time_handle *handle)
{
   if (NULL == handle)
       return VM_NULL_PTR;
   *handle = -1;
   return VM_OK;

} /* vm_status vm_time_close(vm_time_handle *handle) */

/* Initialize the object of time measure */
vm_status vm_time_init(vm_time *m)
{
   if (NULL == m)
       return VM_NULL_PTR;
   m->start = 0;
   m->diff = 0;
   m->freq = vm_time_get_frequency();
   return VM_OK;

} /* vm_status vm_time_init(vm_time *m) */

/* Start the process of time measure */
vm_status vm_time_start(vm_time_handle handle, vm_time *m)
{
    /*  touch unreferenced parameters.
        Take into account Intel's compiler. */
    handle = handle;

   if (NULL == m)
       return VM_NULL_PTR;
   m->start = vm_time_get_tick();
   return VM_OK;

} /* vm_status vm_time_start(vm_time_handle handle, vm_time *m) */

/* Stop the process of time measure and obtain the sampling time in seconds */
double vm_time_stop(vm_time_handle handle, vm_time *m)
{
   double speed_sec;
   vm_tick end;

   /*  touch unreferenced parameters.
       Take into account Intel's compiler. */
   handle = handle;

   end = vm_time_get_tick();
   m->diff += end - m->start;

   if(m->freq == 0) m->freq = vm_time_get_frequency();
   speed_sec = (double)m->diff / (double)m->freq;
   return speed_sec;

} /* double vm_time_stop(vm_time_handle handle, vm_time *m) */

static unsigned long long offset_from_1601_to_1970 = 0;
vm_status vm_time_gettimeofday( struct vm_timeval *TP, struct vm_timezone *TZP ) {
  /* FILETIME data structure is a 64-bit value representing the number
               of 100-nanosecond intervals since January 1, 1601 */
  unsigned long long tmb;
  SYSTEMTIME bp;
  if ( offset_from_1601_to_1970 == 0 ) {
    /* prepare 1970 "epoch" offset */
    bp.wDay = 1; bp.wDayOfWeek = 4; bp.wHour = 0;
    bp.wMinute = 0; bp.wMilliseconds = 0;
    bp.wMonth = 1; bp.wSecond = 0;
    bp.wYear = 1970;
    SystemTimeToFileTime(&bp, (FILETIME *)&offset_from_1601_to_1970);
  }
#ifndef _WIN32_WCE
  GetSystemTimeAsFileTime((FILETIME *)&tmb);
#else
  GetSystemTime(&bp);
  SystemTimeToFileTime(&bp, (FILETIME *)&tmb);
#endif
  tmb -= offset_from_1601_to_1970; /* 100 nsec ticks since 1 jan 1970 */
  TP[0].tv_sec = (uint32_t)(tmb / 10000000); /* */
  TP[0].tv_usec = (long)((tmb % 10000000) / 10); /* microseconds OK? */
  return  (TZP != NULL) ? VM_NOT_INITIALIZED : VM_OK;
}

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */
