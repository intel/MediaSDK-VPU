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

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include "vm_types.h"
#include "vm_shared_object.h"

vm_so_handle vm_so_load(const vm_char *so_file_name)
{
    /* check error(s) */
    if (NULL == so_file_name)
        return NULL;

    return ((vm_so_handle)LoadLibraryExW(so_file_name, NULL, 0));
} /* vm_so_handle vm_so_load(vm_char *so_file_name) */

vm_so_func vm_so_get_addr(vm_so_handle so_handle, const char *so_func_name)
{
    /* check error(s) */
    if (NULL == so_handle)
        return NULL;

    return (vm_so_func)GetProcAddress((HMODULE)so_handle, (LPCSTR)so_func_name);

} /* void *vm_so_get_addr(vm_so_handle so_handle, const vm_char *so_func_name) */

void vm_so_free(vm_so_handle so_handle)
{
    /* check error(s) */
    if (NULL == so_handle)
        return;

    FreeLibrary((HMODULE)so_handle);

} /* void vm_so_free(vm_so_handle so_handle) */

#endif /* defined(_WIN32) || defined(_WIN64) */
