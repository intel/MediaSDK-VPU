// Copyright (c) 2017 Intel Corporation
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

//#include <windows.h>

#ifdef _UNICODE
# define vm_main wmain
#else
# define vm_main _tmain
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __ICL

#define VM_ALIGN_DECL(X,Y) __declspec(align(X)) Y

#else /* !__ICL */

#define VM_ALIGN_DECL(X,Y) Y

#endif /* __ICL */

#define CONST_LL(X) X
#define CONST_ULL(X) X##ULL

typedef void * vm_handle;

/* vm_event.h */
typedef struct vm_event
{
    vm_handle handle;

} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
    vm_handle fd_file, fd_map;
    void  *address;
    int32_t fAccessAttr;
} vm_mmap;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
    vm_handle handle;

} vm_semaphore;

typedef struct vm_time
{
   long long start;
   long long diff;
   long long freq;

} vm_time;

struct vm_timeval
{
  uint32_t tv_sec;
  long int tv_usec;

};

struct vm_timezone
{
  int tz_minuteswest; // This is the number of minutes west of UTC.
  int tz_dsttime;    /* If nonzero, Daylight Saving Time applies
                     during some part of the year. */
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */
