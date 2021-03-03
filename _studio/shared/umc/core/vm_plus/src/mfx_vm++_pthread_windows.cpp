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

#if defined(_WIN32) || defined(_WIN64)

#include "mfx_vm++_pthread.h"

MfxMutex::MfxMutex(void)
{
    InitializeCriticalSection(&m_handle.m_CritSec);
}

MfxMutex::~MfxMutex(void)
{
    DeleteCriticalSection(&m_handle.m_CritSec);
}

mfxStatus MfxMutex::Lock(void)
{
    EnterCriticalSection(&m_handle.m_CritSec);
    return MFX_ERR_NONE;
}

mfxStatus MfxMutex::Unlock(void)
{
    LeaveCriticalSection(&m_handle.m_CritSec);
    return MFX_ERR_NONE;
}

bool MfxMutex::TryLock(void)
{
    return (TryEnterCriticalSection(&m_handle.m_CritSec))? true: false;
}

#endif // #if defined(_WIN32) || defined(_WIN64)
