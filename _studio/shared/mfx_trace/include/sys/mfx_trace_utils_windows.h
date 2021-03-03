// Copyright (c) 2011-2018 Intel Corporation
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

#ifndef __MFX_TRACE_UTILS_WINDOWS_H__
#define __MFX_TRACE_UTILS_WINDOWS_H__

#if defined(_WIN32) || defined(_WIN64)

#ifdef MFX_TRACE_ENABLE

#include <windows.h>
#include <tchar.h>
#include <share.h>

/*------------------------------------------------------------------------------*/

// registry path to trace options and parameters
#define MFX_TRACE_REG_ROOT          HKEY_CURRENT_USER
#define MFX_TRACE_REG_PARAMS_PATH   _T("Software\\Intel\\MediaSDK\\Tracing")
#define MFX_TRACE_REG_CATEGORIES_PATH MFX_TRACE_REG_PARAMS_PATH _T("\\Categories")
// trace registry options and parameters
#define MFX_TRACE_REG_OMODE_TYPE    _T("Output")
#define MFX_TRACE_REG_CATEGORIES    _T("Categories")
#define MFX_TRACE_REG_LEVEL         _T("Level")

/*------------------------------------------------------------------------------*/

HRESULT mfx_trace_get_reg_dword(HKEY BaseKey,
                                const TCHAR *pPath, const TCHAR* pName,
                                UINT32* pValue);
HRESULT mfx_trace_get_reg_qword(HKEY BaseKey,
                                const TCHAR *pPath, const TCHAR* pName,
                                UINT64* pValue);
HRESULT mfx_trace_get_reg_string(HKEY BaseKey,
                                 const TCHAR *pPath, const TCHAR* pName,
                                 TCHAR* pValue, UINT32 cValueMaxSize);
HRESULT mfx_trace_get_reg_mstring(HKEY BaseKey,
                                  const TCHAR *pPath, const TCHAR* pName,
                                  TCHAR* pValue, UINT32 cValueMaxSize);

#endif // #if defined(_WIN32) || defined(_WIN64)

#endif // #ifdef MFX_TRACE_ENABLE

#endif // #ifndef __MFX_TRACE_UTILS_WINDOWS_H__
