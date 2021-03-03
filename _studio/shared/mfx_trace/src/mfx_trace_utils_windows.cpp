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

#if defined(_WIN32) || defined(_WIN64)

#include "mfx_trace.h"

#ifdef MFX_TRACE_ENABLE
extern "C"
{

#include "mfx_trace_utils.h"

/*------------------------------------------------------------------------------*/

HRESULT mfx_trace_get_reg_dword(HKEY BaseKey,
                                const TCHAR *pPath, const TCHAR* pName,
                                UINT32* pValue)
{
    HRESULT hr = S_OK;
    HKEY key = NULL;
    LSTATUS res = ERROR_SUCCESS;
    DWORD size = sizeof(UINT32), type = 0;

    if (SUCCEEDED(hr))
    {
        res = RegOpenKeyEx(BaseKey, pPath, 0, KEY_QUERY_VALUE, &key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        res = RegQueryValueEx(key, pName, NULL, &type, (LPBYTE)pValue, &size);
        RegCloseKey(key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr) && (REG_DWORD != type)) hr = E_FAIL;
    return hr;
}

/*------------------------------------------------------------------------------*/

HRESULT mfx_trace_get_reg_qword(HKEY BaseKey,
                                const TCHAR *pPath, const TCHAR* pName,
                                UINT64* pValue)
{
    HRESULT hr = S_OK;
    HKEY key = NULL;
    LSTATUS res = ERROR_SUCCESS;
    DWORD size = sizeof(UINT64), type = 0;

    if (SUCCEEDED(hr))
    {
        res = RegOpenKeyEx(BaseKey, pPath, 0, KEY_QUERY_VALUE, &key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        res = RegQueryValueEx(key, pName, NULL, &type, (LPBYTE)pValue, &size);
        RegCloseKey(key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr) && (REG_QWORD != type)) hr = E_FAIL;
    return hr;
}

/*------------------------------------------------------------------------------*/

HRESULT mfx_trace_get_reg_string(HKEY BaseKey,
                                 const TCHAR *pPath, const TCHAR* pName,
                                 TCHAR* pValue, UINT32 cValueMaxSize)
{
    HRESULT hr = S_OK;
    HKEY key = NULL;
    LSTATUS res = ERROR_SUCCESS;
    DWORD size = cValueMaxSize*sizeof(TCHAR);
    DWORD type = 0;

    if (SUCCEEDED(hr))
    {
        res = RegOpenKeyEx(BaseKey, pPath, 0, KEY_QUERY_VALUE, &key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        res = RegQueryValueEx(key, pName, NULL, &type, (LPBYTE)pValue, &size);
        RegCloseKey(key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr) && (REG_SZ != type)) hr = E_FAIL;
    return hr;
}

/*------------------------------------------------------------------------------*/

HRESULT mfx_trace_get_reg_mstring(HKEY BaseKey,
                                  const TCHAR *pPath, const TCHAR* pName,
                                  TCHAR* pValue, UINT32 cValueMaxSize)
{
    HRESULT hr = S_OK;
    HKEY key = NULL;
    LSTATUS res = ERROR_SUCCESS;
    DWORD size = cValueMaxSize*sizeof(TCHAR), type = 0;

    if (SUCCEEDED(hr))
    {
        res = RegOpenKeyEx(BaseKey, pPath, 0, KEY_QUERY_VALUE, &key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        res = RegQueryValueEx(key, pName, NULL, &type, (LPBYTE)pValue, &size);
        RegCloseKey(key);
        if (ERROR_SUCCESS != res) hr = E_FAIL;
    }
    if (SUCCEEDED(hr) && (REG_MULTI_SZ != type)) hr = E_FAIL;
    return hr;
}

} // extern "C"
#endif // #ifdef MFX_TRACE_ENABLE

#endif // #if defined(_WIN32) || defined(_WIN64)
