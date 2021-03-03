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

#include "mfx_config.h"
#include <mfxvideo.h>

#if defined (MFX_VA_LINUX) && defined(HDDL_UNITE)

#pragma once

#include <va/va.h>
#include <map>

#ifndef MFX_HW_VSI_TARGET

#include <RemoteMemory.h>
#include <WorkloadContext.h>

class HDDLAdapterFrameAllocator : public mfxFrameAllocator
{
public:
    HDDLAdapterFrameAllocator(mfxFrameAllocator& allocator, VADisplay dpy);
    virtual ~HDDLAdapterFrameAllocator();

    mfxStatus UpdateRemoteMemoryAfterRealloc(const mfxFrameSurface1* surface);
    mfxStatus Close();

    mfxStatus Init();

protected:
    static mfxStatus MFX_CDECL  Alloc_(mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response);
    static mfxStatus MFX_CDECL  Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr);
    static mfxStatus MFX_CDECL  Unlock_(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr);
    static mfxStatus MFX_CDECL  GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL* handle);
    static mfxStatus MFX_CDECL  Free_(mfxHDL pthis, mfxFrameAllocResponse* response);

    mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL* handle);

    VASurfaceID CreateSurfaceFromRemoteMemory(const HddlUnite::RemoteMemory* rm);
    VASurfaceID CreateSurface(const HddlUnite::RemoteMemory* rm);
    mfxStatus ExportSurface(VASurfaceID surf, HddlUnite::RemoteMemory* rm);

    uint64_t workloadId = -1;
    HddlUnite::WorkloadContext::Ptr workload;
    VADisplay m_dpy = 0;

    struct RMEntry {
        int dmaBuf = 0;
        VASurfaceID surf = VA_INVALID_ID;
        bool needUpdateDMA = true;
        HddlUnite::RemoteMemory* rm = 0;
    };

    std::map<mfxMemId, RMEntry> surfaceIdMap;
    mfxFrameAllocator& alloc;
};
#endif // #ifndef MFX_HW_VSI_TARGET

#endif // MFX_VA_LINUX
/* EOF */
