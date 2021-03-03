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

#include <iostream>

#include "mfx_common.h"

#if defined (MFX_VA_LINUX) && defined(HDDL_UNITE)

#include "umc_va_linux.h"
#include <va/va_drmcommon.h>

#if defined(LINUX32) || defined(LINUX64)
#include <va/va_drm.h>
#endif

#include "libmfx_core_hddl.h"
#include "libmfx_core_hw.h"

#ifndef MFX_HW_VSI_TARGET
#include <WorkloadContextC.h>
#include <WorkloadContext.h>
#endif

#if defined(LINUX32) || defined(LINUX64)
#include <unistd.h>
#include <sys/syscall.h>
#include <va/va_drm.h>
#endif

using namespace std;
using namespace UMC;

#if (defined(_WIN32) || defined(_WIN64)) && defined(MFX_HW_KMB)
extern "C" VADisplay vaGetDisplayWindowsDRM();
#endif

HDDLVideoCORE::HDDLVideoCORE(
    const mfxU32 adapterNum,
    const mfxU32 numThreadsAvailable,
    const mfxSession session)
        : VAAPIVideoCORE(adapterNum, numThreadsAvailable, session)
{
#ifndef MFX_HW_VSI_TARGET
    VADisplay dpy;

#if (defined(_WIN32) || defined(_WIN64)) && defined(MFX_HW_KMB)
    dpy = vaGetDisplayWindowsDRM();
#else
    m_fd = 0; // There's no /dev/dri device file for vaapi shim IA host
    dpy = vaGetDisplayDRM(m_fd);
#endif

    int major_version = 0, minor_version = 0;
    VAStatus va_res = vaInitialize(dpy, &major_version, &minor_version);
    
    if (va_res == VA_STATUS_SUCCESS)
        SetHandle(MFX_HANDLE_VA_DISPLAY, dpy);
#endif
}

HDDLVideoCORE::~HDDLVideoCORE() {
    Close();
}

void HDDLVideoCORE::Close() {
    VAAPIVideoCORE::Close();

#ifndef MFX_HW_VSI_TARGET
    if (m_hddlAllocator.get()) {
        m_hddlAllocator->Close();
    }

    vaTerminate(m_Display);

#if defined(LINUX32) || defined(LINUX64)
    if (m_fd >= 0) {
        close(m_fd);
    }
#endif
#endif
}

#ifndef MFX_HW_VSI_TARGET
mfxStatus HDDLVideoCORE::SetFrameAllocator(mfxFrameAllocator* allocator) {
    if (!m_hddlAllocator.get() && allocator)
    {
        m_hddlAllocator.reset(new HDDLAdapterFrameAllocator(*allocator, m_Display));
        mfxStatus sts = m_hddlAllocator->Init();
        MFX_CHECK_STS(sts);
    }

    return VAAPIVideoCORE::SetFrameAllocator(m_hddlAllocator.get());
}
#endif

mfxStatus HDDLVideoCORE::UpdateRemoteMemoryAfterRealloc(const mfxFrameSurface1 * surface) {
#ifndef MFX_HW_VSI_TARGET
    return (m_hddlAllocator.get() && surface) ? m_hddlAllocator->UpdateRemoteMemoryAfterRealloc(surface) : MFX_ERR_NONE;
#else
    return MFX_ERR_NONE;
#endif
}

#endif
/* EOF */
