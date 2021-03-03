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

#include "vm_sys_info.h"

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#if _MSC_VER >= 1400
#  pragma warning(disable: 4996)
#endif

/* disable warning C4201: nonstandard extension used : nameless struct/union */
#pragma warning(disable: 4201)

#if defined (__ICL)
/* non-pointer conversion from "unsigned __int64" to "int32_t={signed int}" may lose significant bits */
#pragma warning(disable:2259)
#endif

void vm_sys_info_get_os_name(vm_char *os_name)
{
    OSVERSIONINFO osvi;
    BOOL bOsVersionInfo;

    /* check error(s) */
    if (NULL == os_name)
        return;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    bOsVersionInfo = GetVersionEx((OSVERSIONINFO *) &osvi);
    if (!bOsVersionInfo)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO *) &osvi))
        {
            vm_string_sprintf(os_name, VM_STRING("Unknown"));
            return;
        }
    }

    switch (osvi.dwPlatformId)
    {
    case 2:
        /* test for the specific product family. */
        if ((5 == osvi.dwMajorVersion) && (2 == osvi.dwMinorVersion))
            vm_string_sprintf(os_name, VM_STRING("Win2003"));

        if ((5 == osvi.dwMajorVersion) && (1 == osvi.dwMinorVersion))
            vm_string_sprintf(os_name, VM_STRING("WinXP"));

        if ((5 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion))
            vm_string_sprintf(os_name, VM_STRING("Win2000"));

        if (4 >= osvi.dwMajorVersion)
            vm_string_sprintf(os_name, VM_STRING("WinNT"));
        break;

        /* test for the Windows 95 product family. */
    case 1:
        if ((4 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("Win95"));
            if (('C' == osvi.szCSDVersion[1]) || ('B' == osvi.szCSDVersion[1]))
                vm_string_strcat_s(os_name, _MAX_LEN, VM_STRING("OSR2" ));
        }

        if ((4 == osvi.dwMajorVersion) && (10 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("Win98"));
            if ('A' == osvi.szCSDVersion[1])
                vm_string_strcat_s(os_name, _MAX_LEN, VM_STRING("SE"));
        }

        if ((4 == osvi.dwMajorVersion) && (90 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name,VM_STRING("WinME"));
        }
        break;

    case 3:
        /* get platform string */
        /* SystemParametersInfo(257, MAX_PATH, os_name, 0); */
        if ((4 == osvi.dwMajorVersion) && (20 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("PocketPC 2003"));
        }

        if ((4 == osvi.dwMajorVersion) && (21 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("WinMobile2003SE"));
        }

        if ((5 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("WinCE 5.0"));
        }
        break;

        /* Something else */
    default:
        vm_string_sprintf(os_name, VM_STRING("Win..."));
        break;
    }
} /* void vm_sys_info_get_os_name(vm_char *os_name) */

void vm_sys_info_get_program_name(vm_char *program_name)
{
    vm_char tmp[_MAX_LEN] = {0,};
    int32_t i = 0;

    /* check error(s) */
    if (NULL == program_name)
        return;

    GetModuleFileName(NULL, tmp, _MAX_LEN);
    i = (int32_t) (vm_string_strrchr(tmp, (vm_char)('\\')) - tmp + 1);
    vm_string_strncpy_s(program_name, _MAX_LEN, tmp + i, vm_string_strlen(tmp) - i);

} /* void vm_sys_info_get_program_name(vm_char *program_name) */

void vm_sys_info_get_program_path(vm_char *program_path)
{
    vm_char tmp[_MAX_LEN] = {0,};
    int32_t i = 0;

    /* check error(s) */
    if (NULL == program_path)
        return;

    GetModuleFileName(NULL, tmp, _MAX_LEN);
    i = (int32_t) (vm_string_strrchr(tmp, (vm_char)('\\')) - tmp + 1);
    vm_string_strncpy_s(program_path, _MAX_LEN, tmp, i - 1);
    program_path[i - 1] = 0;

} /* void vm_sys_info_get_program_path(vm_char *program_path) */

uint32_t vm_sys_info_get_cpu_num(void)
{
    SYSTEM_INFO siSysInfo;

    ZeroMemory(&siSysInfo, sizeof(SYSTEM_INFO));
    GetSystemInfo(&siSysInfo);

    return siSysInfo.dwNumberOfProcessors;

} /* uint32_t vm_sys_info_get_cpu_num(void) */

uint32_t vm_sys_info_get_numa_nodes_num(void)
{
#if !defined(_WIN32_WCE)
    ULONG HighestNodeNumber;

    BOOL ret = GetNumaHighestNodeNumber(&HighestNodeNumber);

    if (!ret) return 1;

    return HighestNodeNumber + 1;
#else
    return 1;
#endif
} /* uint32_t vm_sys_info_get_numa_nodes_num(void) */

unsigned long long vm_sys_info_get_cpu_mask_of_numa_node(uint32_t node)
{
    GROUP_AFFINITY group_affinity;
    memset(&group_affinity, 0, sizeof(GROUP_AFFINITY));
    GetNumaNodeProcessorMaskEx((UCHAR)node, &group_affinity);
    return (unsigned long long)group_affinity.Mask;
}

uint32_t vm_sys_info_get_mem_size(void)
{
    MEMORYSTATUSEX m_memstat;

    ZeroMemory(&m_memstat, sizeof(MEMORYSTATUS));
    GlobalMemoryStatusEx(&m_memstat);

    return (uint32_t)((double)m_memstat.ullTotalPhys / (1024 * 1024) + 0.5);

} /* uint32_t vm_sys_info_get_mem_size(void) */

#if !defined(_WIN32_WCE) && !defined(WIN_TRESHOLD_MOBILE)

#pragma comment(lib, "Setupapi.lib")
#include <setupapi.h>

#if _WIN32_WINNT >= 0x600
typedef BOOL (WINAPI *FUNC_SetupDiGetDeviceProperty)(
    __in         HDEVINFO         DeviceInfoSet,
    __in         PSP_DEVINFO_DATA DeviceInfoData,
    __in   CONST DEVPROPKEY      *PropertyKey,
    __out        DEVPROPTYPE     *PropertyType,
    __out_bcount_opt(PropertyBufferSize) PBYTE PropertyBuffer,
    __in         DWORD            PropertyBufferSize,
    __out_opt    PDWORD           RequiredSize,
    __in         DWORD            Flags
    );

#define MY_DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) \
    static const DEVPROPKEY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }

MY_DEFINE_DEVPROPKEY(MY_DEVPKEY_Device_DriverDate,       0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 2);      // DEVPROP_TYPE_FILETIME
MY_DEFINE_DEVPROPKEY(MY_DEVPKEY_Device_DriverVersion,    0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 3);      // DEVPROP_TYPE_STRING
MY_DEFINE_DEVPROPKEY(MY_DEVPKEY_Device_DriverDesc,       0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 4);      // DEVPROP_TYPE_STRING
#endif

#if 0
void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    SP_DEVINFO_DATA sp_dev_info;
    HDEVINFO DeviceInfoSet;
    vm_char string1[] = VM_STRING("Display");
    uint32_t dwIndex = 0;
    vm_char data[_MAX_LEN] = {0,};

    /* check error(s) */
    if (NULL == vga_card)
        return;

    ZeroMemory(&sp_dev_info, sizeof(SP_DEVINFO_DATA));
    ZeroMemory(&DeviceInfoSet, sizeof(HDEVINFO));

    DeviceInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    sp_dev_info.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInfo(DeviceInfoSet, dwIndex, &sp_dev_info))
    {
        SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         &sp_dev_info,
                                         SPDRP_CLASS,
                                         NULL,
                                         (uint8_t *) data,
                                         _MAX_LEN,
                                         NULL);
        if (!vm_string_strcmp((vm_char*)data, string1))
        {
#if (_WIN32_WINNT >= 0x600) && (defined(UNICODE) || defined(_UNICODE))
            HMODULE pLib = LoadLibrary(_T("setupapi.dll"));
            FUNC_SetupDiGetDeviceProperty pGetDeviceProperty = NULL;
            if (pLib)
            {
                pGetDeviceProperty = (FUNC_SetupDiGetDeviceProperty)GetProcAddress(pLib, "SetupDiGetDevicePropertyW");
            }
            if (pGetDeviceProperty)
            {
                TCHAR desc[1024] = _T("");
                TCHAR ver[1024] = _T("");
                FILETIME driver_filetime = {0, 0};
                FILETIME driver_filetime2 = {0, 0};
                SYSTEMTIME driver_time;
                SYSTEMTIME driver_time2;
                DEVPROPTYPE PropertyType;
                HRESULT hr;
                DWORD size;
                int ww;

                hr = pGetDeviceProperty(DeviceInfoSet, &sp_dev_info, &MY_DEVPKEY_Device_DriverDesc,    &PropertyType, (BYTE*)desc, sizeof(desc), &size, 0);
                hr = pGetDeviceProperty(DeviceInfoSet, &sp_dev_info, &MY_DEVPKEY_Device_DriverVersion, &PropertyType, (BYTE*)ver, sizeof(ver), &size, 0);
                hr = pGetDeviceProperty(DeviceInfoSet, &sp_dev_info, &MY_DEVPKEY_Device_DriverDate,    &PropertyType, (BYTE*)&driver_filetime, sizeof(driver_filetime), &size, 0);

                FileTimeToSystemTime(&driver_filetime, &driver_time);

                // calculate workweek number
                driver_time2 = driver_time;
                driver_time2.wDay = 1 + driver_time.wDayOfWeek;
                driver_time2.wMonth = 1;
                SystemTimeToFileTime(&driver_time2, &driver_filetime2);
                ww = 2 + (int)((*(ULONGLONG*)&driver_filetime - *(ULONGLONG*)&driver_filetime2) / ((ULONGLONG)7*24*60*60*10000000));

                _stprintf(vga_card, _T("%s, %s, %d/%d/%d ww%d.%d"), desc, ver, driver_time.wMonth, driver_time.wDay, driver_time.wYear, ww, driver_time.wDayOfWeek);
            }
            else
#endif
            {
                SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                 &sp_dev_info,
                                                 SPDRP_DEVICEDESC,
                                                 NULL,
                                                 (PBYTE) vga_card,
                                                 _MAX_LEN,
                                                 NULL);
            }

#if (_WIN32_WINNT >= 0x600) && (defined(UNICODE) || defined(_UNICODE))
            if (pLib)
            {
                FreeLibrary(pLib);
            }

#endif // (_WIN32_WINNT >= 0x600) && (defined(UNICODE) || defined(_UNICODE))

            break;
        }
        dwIndex++;
    }
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

} /* void vm_sys_info_get_vga_card(vm_char *vga_card) */
#endif //#if 0

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
    HKEY hKey;
    vm_char data[_MAX_LEN] = {0,};
    /* it can be wchar variable */
    DWORD dwSize = sizeof(data) / sizeof(data[0]);
    int32_t i = 0;
    LONG lErr;

    /* check error(s) */
    if (NULL == cpu_name)
        return;

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        VM_STRING("Hardware\\Description\\System\\CentralProcessor\\0"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        RegQueryValueEx(hKey,
                        _T("ProcessorNameString"),
                        NULL,
                        NULL,
                        (LPBYTE)&data,
                        &dwSize);
        /* error protection */
        data[sizeof(data) / sizeof(data[0]) - 1] = 0;
        while (data[i++] == ' ');
        vm_string_strcpy_s(cpu_name, _MAX_LEN, (vm_char *)(data + i - 1));
        RegCloseKey (hKey);
    }
    else
        vm_string_sprintf(cpu_name, VM_STRING("Unknown"));

} /* void vm_sys_info_get_cpu_name(vm_char *cpu_name) */

uint32_t vm_sys_info_get_cpu_speed(void)
{
    HKEY hKey;
    uint32_t data = 0;
    DWORD dwSize = sizeof(uint32_t);
    LONG lErr;

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        VM_STRING("Hardware\\Description\\System\\CentralProcessor\\0"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        RegQueryValueEx (hKey, _T("~MHz"), NULL, NULL, (LPBYTE)&data, &dwSize);
        RegCloseKey(hKey);
        return data;
    }
    else
        return 0;

} /* uint32_t vm_sys_info_get_cpu_speed(void) */

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    vm_char data[_MAX_LEN] = {0,};
    /* it can be wchar variable */
    DWORD dwSize = sizeof(data) / sizeof(data[0]);

    /* check error(s) */
    if (NULL == computer_name)
        return;

    GetComputerName(data, &dwSize);
    vm_string_sprintf(computer_name, VM_STRING("%s"), data);

} /* void vm_sys_info_get_computer_name(vm_char* computer_name) */

#elif defined(_WIN32_WCE)

#include <winioctl.h>

BOOL KernelIoControl(uint32_t dwIoControlCode,
                     LPVOID lpInBuf,
                     uint32_t nInBufSize,
                     LPVOID lpOutBuf,
                     uint32_t nOutBufSize,
                     LPDWORD lpBytesReturned);

#define IOCTL_PROCESSOR_INFORMATION CTL_CODE(FILE_DEVICE_HAL, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PROCESSOR_INFO
{
    uint16_t        wVersion;
    vm_char       szProcessCore[40];
    uint16_t        wCoreRevision;
    vm_char       szProcessorName[40];
    uint16_t        wProcessorRevision;
    vm_char       szCatalogNumber[100];
    vm_char       szVendor[100];
    uint32_t        dwInstructionSet;
    uint32_t        dwClockSpeed;
} PROCESSOR_INFO;

void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    /* check error(s) */
    if (NULL == vga_card)
        return;

    vm_string_sprintf(vga_card, VM_STRING("Unknown"));

} /* void vm_sys_info_get_vga_card(vm_char *vga_card) */

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
    PROCESSOR_INFO pi;
    uint32_t dwBytesReturned;
    uint32_t dwSize = sizeof(PROCESSOR_INFO);
    BOOL bResult;

    /* check error(s) */
    if (NULL == cpu_name)
        return;

    ZeroMemory(&pi, sizeof(PROCESSOR_INFO));
    bResult = KernelIoControl(IOCTL_PROCESSOR_INFORMATION,
                              NULL,
                              0,
                              &pi,
                              sizeof(PROCESSOR_INFO),
                              &dwBytesReturned);

    vm_string_sprintf(cpu_name,
                      VM_STRING("%s %s"),
                      pi.szProcessCore,
                      pi.szProcessorName);

} /* void vm_sys_info_get_cpu_name(vm_char *cpu_name) */

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    vm_char data[_MAX_LEN] = {0,};
    /* it can be wchar variable */
    uint32_t dwSize = sizeof(data) / sizeof(data[0]);

    /* check error(s) */
    if (NULL == computer_name)
        return;

    SystemParametersInfo(SPI_GETOEMINFO, dwSize, data, 0);
    vm_string_sprintf(computer_name, VM_STRING("%s"), data);

} /* void vm_sys_info_get_computer_name(vm_char *computer_name) */

uint32_t vm_sys_info_get_cpu_speed(void)
{
    PROCESSOR_INFO pi;
    uint32_t res = 400;
    uint32_t dwBytesReturned;
    uint32_t dwSize = sizeof(PROCESSOR_INFO);
    BOOL bResult;

    ZeroMemory(&pi,sizeof(PROCESSOR_INFO));
    bResult = KernelIoControl(IOCTL_PROCESSOR_INFORMATION,
                              NULL,
                              0,
                              &pi,
                              sizeof(PROCESSOR_INFO),
                              &dwBytesReturned);
    if (pi.dwClockSpeed)
        res = pi.dwClockSpeed;
    return res;

} /* uint32_t vm_sys_info_get_cpu_speed(void) */

#elif defined(WIN_TRESHOLD_MOBILE)
void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    vga_card;
    return;
} /* void vm_sys_info_get_vga_card(vm_char *vga_card) */

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
    cpu_name;
    return;
} /* void vm_sys_info_get_cpu_name(vm_char *cpu_name) */

uint32_t vm_sys_info_get_cpu_speed(void)
{
    return 0;
} /* uint32_t vm_sys_info_get_cpu_speed(void) */

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    computer_name;
    return;
} /* void vm_sys_info_get_computer_name(vm_char* computer_name) */
#endif /* defined(WIN_TRESHOLD_MOBILE) */

#endif /* defined(_WIN32) || defined(_WIN64) */
