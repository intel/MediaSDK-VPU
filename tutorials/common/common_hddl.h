/******************************************************************************\
Copyright (c) 2005-202, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#pragma once

#if defined(LIBVA_SUPPORT) && defined(HDDL_UNITE) && !defined(MFX_HW_VSI_TARGET)

#include <stdlib.h>
#include <va/va.h>
#include <va/va_drmcommon.h>

#include "vaapi_allocator.h"
#include "vaapi_device.h"
#include "vaapi_utils.h"

#include <WorkloadContextC.h>
#include <WorkloadContext.h>
#include <RemoteMemory.h>

#include <list>
#include <tuple>
#include <map>

struct hddlMemId
{
    HddlUnite::RemoteMemory::Ptr m_surface;
};

class ExternalHDDLAllocator : public BaseFrameAllocator
{
public:
    ExternalHDDLAllocator();
    virtual ~ExternalHDDLAllocator();

    virtual mfxStatus Init(mfxAllocatorParams* pParams);
    virtual mfxStatus Close();

    mfxStatus AllocFrames(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) override;
    mfxFrameSurface1* GetFreeSurfaceIndex();

    HddlUnite::RemoteMemoryDesc GetDefaultLayout(mfxFrameAllocRequest* request);

    HddlUnite::RemoteMemory::Ptr ExportRemoteMemory(mfxFrameSurface1 * surface);
    void ImportRemoteMemory(mfxFrameSurface1* surface, HddlUnite::RemoteMemory::Ptr rm);

protected:
    DISALLOW_COPY_AND_ASSIGN(ExternalHDDLAllocator);

    mfxStatus LockRemoteFrame(HddlUnite::RemoteMemory::Ptr remoteMemory, mfxFrameData* ptr);
    mfxStatus UnlockRemoteFrame(HddlUnite::RemoteMemory::Ptr remoteMemory, mfxFrameData* ptr);

    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData* ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData* ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL* handle);

    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest* request);
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse* response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response);
    virtual mfxStatus ReallocImpl(mfxMemId midIn, const mfxFrameInfo* info, mfxU16 memType, mfxMemId* midOut);

    mfxStatus setFrameData(HddlUnite::RemoteMemoryDesc desc, mfxU8* pBuffer, mfxFrameData* ptr);

    HddlUnite::WorkloadContext::Ptr m_workload;
    std::vector<mfxU8> m_lock_buffer;
    std::vector<mfxFrameSurface1> surfacesPool;

    std::list<HddlUnite::RemoteMemory::Ptr> remoteMemoryObjects;
};

class HDDLWorkload {
public:

    HDDLWorkload() {
        ContextHint contextHint = {};
        workload = -1;

        HddlStatusCode hddlStatus = createWorkloadContext(&workload, &contextHint);
        if (hddlStatus != HDDL_OK)
            throw 0;
    }

    ~HDDLWorkload() {
        if (workload != -1) {
            destroyWorkloadContext(workload);
        }
    }

protected:
    uint64_t workload;
};

std::shared_ptr<ExternalHDDLAllocator> CreateAllocator();
mfxStatus Initialize(mfxIMPL impl, mfxVersion ver, MFXVideoSession* pSession,
    std::shared_ptr<ExternalHDDLAllocator>& alloc);

#endif //#if defined(LIBVA_SUPPORT) && defined(HDDL_UNITE) && !defined(MFX_HW_VSI_TARGET)

