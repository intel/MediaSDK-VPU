/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
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

#if defined(LIBVA_SUPPORT) && defined(HDDL_UNITE) && !defined(MFX_HW_VSI_TARGET)

#include <stdio.h>
#include <assert.h>

#include "hddl_allocator.h"
#include "vaapi_utils.h"

using namespace HddlUnite;

ExternalHDDLAllocator::ExternalHDDLAllocator() {
}

ExternalHDDLAllocator::~ExternalHDDLAllocator() {
    Close();
}

mfxStatus ExternalHDDLAllocator::Init(mfxAllocatorParams* )
{
    uint64_t workloadId;

    HddlStatusCode hddlStatus = getWorkloadContextId(msdk_get_current_pid(), msdk_get_current_tid(), &workloadId);
    if (hddlStatus != HDDL_OK)
        return MFX_ERR_DEVICE_FAILED;

    m_workload = queryWorkloadContext(workloadId);
    if (!m_workload.get())
        return MFX_ERR_DEVICE_FAILED;

    return MFX_ERR_NONE;
}

mfxStatus ExternalHDDLAllocator::CheckRequestType(mfxFrameAllocRequest* request)
{
    mfxStatus sts = BaseFrameAllocator::CheckRequestType(request);
    if (MFX_ERR_NONE != sts)
        return sts;

    if ((request->Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)) != 0)
        return MFX_ERR_NONE;
    else
        return MFX_ERR_UNSUPPORTED;
}

mfxStatus ExternalHDDLAllocator::Close()
{
    return BaseFrameAllocator::Close();
}

mfxStatus ExternalHDDLAllocator::ReallocImpl(mfxMemId mid, const mfxFrameInfo* info, mfxU16 memType, mfxMemId* midOut)
{
    if (!info || !midOut) return MFX_ERR_NULL_PTR;

    mfxStatus mfx_res = MFX_ERR_NONE;
    return MFX_ERR_UNSUPPORTED;
}

#define ALIGN(X, Y) (((mfxU32)((X)+(Y) - 1)) & (~ (mfxU32)(Y - 1)))

HddlUnite::RemoteMemoryDesc ExternalHDDLAllocator::GetDefaultLayout(mfxFrameAllocRequest* request)
{
    mfxU32 Pitch = ALIGN(request->Info.Width, 64);
    mfxU32 Height = ALIGN(request->Info.Height, 64);

    if (request->Info.BitDepthLuma > 8)
        return HddlUnite::RemoteMemoryDesc(request->Info.Width, request->Info.Height, Pitch * 2, Height,
            eRemoteMemoryFormat::NV12, eRemoteMemoryBitDepth::BITDEPTH_10bit, false);
    else
        return HddlUnite::RemoteMemoryDesc(request->Info.Width, request->Info.Height, Pitch, Height,
            eRemoteMemoryFormat::NV12, eRemoteMemoryBitDepth::BITDEPTH_8bit, false);
}

mfxStatus ExternalHDDLAllocator::AllocImpl(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU16 surfaces_num = request->NumFrameSuggested, numAllocated = 0, i = 0;

    memset(response, 0, sizeof(mfxFrameAllocResponse));

    if (!surfaces_num)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    HddlUnite::RemoteMemoryDesc desc = GetDefaultLayout(request);
    MSDK_CHECK_RESULT(mfx_res, MFX_ERR_NONE, mfx_res);

    hddlMemId* vaapi_mids = (hddlMemId*)calloc(surfaces_num, sizeof(hddlMemId));
    mfxMemId* mids = (mfxMemId*)calloc(surfaces_num, sizeof(mfxMemId));
    if ((NULL == vaapi_mids) || (NULL == mids)) mfx_res = MFX_ERR_MEMORY_ALLOC;

    if (MFX_ERR_NONE == mfx_res)
    {
        for (i = 0; i < surfaces_num; ++i)
        {
            hddlMemId * vaapi_mid = &(vaapi_mids[i]);
            RemoteMemory::Ptr rm = std::make_shared<RemoteMemory>(*m_workload.get(), desc, 0);

            remoteMemoryObjects.push_back(rm);
            vaapi_mid->m_surface = rm;

            mids[i] = vaapi_mid;
        }
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        response->mids = mids;
        response->NumFrameActual = surfaces_num;
    }
    else // i.e. MFX_ERR_NONE != mfx_res
    {
        response->mids = NULL;
        response->NumFrameActual = 0;

        if (mids)
        {
            free(mids);
            mids = NULL;
        }
        if (vaapi_mids) { free(vaapi_mids); vaapi_mids = NULL; }
    }
    return mfx_res;
}

mfxStatus ExternalHDDLAllocator::ReleaseResponse(mfxFrameAllocResponse* response)
{
    hddlMemId* vaapi_mids = NULL;
    bool isBitstreamMemory = false;

    if (!response) return MFX_ERR_NULL_PTR;

    if (response->mids)
    {
        vaapi_mids = (hddlMemId*)(response->mids[0]);
        free(vaapi_mids);
        free(response->mids);
        response->mids = NULL;
    }
    response->NumFrameActual = 0;
    return MFX_ERR_NONE;
}

mfxStatus ExternalHDDLAllocator::LockFrame(mfxMemId mid, mfxFrameData* ptr)
{
    hddlMemId* vaapi_mid = (hddlMemId*)mid;
    if (!ptr || !vaapi_mid || !(vaapi_mid->m_surface)) return MFX_ERR_INVALID_HANDLE;
    return LockRemoteFrame(vaapi_mid->m_surface, ptr);
}

mfxStatus ExternalHDDLAllocator::UnlockFrame(mfxMemId mid, mfxFrameData* ptr)
{
    hddlMemId* vaapi_mid = (hddlMemId*)mid;
    if (!ptr || !vaapi_mid || !(vaapi_mid->m_surface)) return MFX_ERR_INVALID_HANDLE;
    return UnlockRemoteFrame(vaapi_mid->m_surface, ptr);
}

mfxStatus ExternalHDDLAllocator::GetFrameHDL(mfxMemId mid, mfxHDL* handle)
{
    hddlMemId* vaapi_mid = (hddlMemId*)mid;

    if (!handle || !vaapi_mid || !(vaapi_mid->m_surface)) return MFX_ERR_INVALID_HANDLE;

    *handle = vaapi_mid->m_surface.get(); //RemoteMemory* <-> mfxHDL
    return MFX_ERR_NONE;
}

mfxFrameSurface1* ExternalHDDLAllocator::GetFreeSurfaceIndex()
{
    auto it = std::find_if(surfacesPool.begin(), surfacesPool.end(), [this](const mfxFrameSurface1& surface) {
        if (surface.Data.Locked)
            return false;

        return 0 == surface.Data.Locked;
    });

    if (it == surfacesPool.end())
        return 0;
    else return &(*it);
}

mfxStatus ExternalHDDLAllocator::AllocFrames(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {
    mfxStatus sts = BaseFrameAllocator::AllocFrames(request, response);
    if (sts != MFX_ERR_NONE)
        return sts;

    if (surfacesPool.size())
        return MFX_ERR_NONE;

    int numSurfaces = response->NumFrameActual;
    surfacesPool.resize(numSurfaces);
    for (int i = 0; i < numSurfaces; i++) {
        memset(&surfacesPool[i], 0, sizeof(mfxFrameSurface1));
        surfacesPool[i].Info = request->Info;
        surfacesPool[i].Data.MemId = response->mids[i];      // MID (memory id) represents one video NV12 surface
    }

    return MFX_ERR_NONE;
}

mfxStatus ExternalHDDLAllocator::LockRemoteFrame(HddlUnite::RemoteMemory::Ptr rm, mfxFrameData* ptr) {
    if (!rm)
        return MFX_ERR_INVALID_HANDLE;

    RemoteMemoryDesc desc = rm->getMemoryDesc();
    size_t size = desc.getDataSize();
    if (m_lock_buffer.size() < size) {
        m_lock_buffer.resize(size);
    }

    mfxU8* pBuffer = m_lock_buffer.data();

    HddlStatusCode hddlStatus = rm->syncFromDevice(pBuffer, size);
    if (hddlStatus != HDDL_OK)
        return MFX_ERR_LOCK_MEMORY;

    return setFrameData(rm->getMemoryDesc(), pBuffer, ptr);
}

mfxStatus ExternalHDDLAllocator::setFrameData(HddlUnite::RemoteMemoryDesc desc, mfxU8* pBuffer, mfxFrameData* ptr) {
    if (!desc.checkValid())
        return MFX_ERR_INVALID_VIDEO_PARAM;

    switch (desc.m_format)
    {
    case HddlUnite::eRemoteMemoryFormat::NV12:
        ptr->Y = pBuffer + 0;
        ptr->U = pBuffer + desc.m_widthStride * desc.m_heightStride;
        ptr->V = ptr->U + (desc.m_BPP == eRemoteMemoryBitDepth::BITDEPTH_8bit ? 1 : 2);
        break;
    default:
        return MFX_ERR_LOCK_MEMORY;
    }

    ptr->PitchHigh = (mfxU16)(desc.m_widthStride / (1 << 16));
    ptr->PitchLow = (mfxU16)(desc.m_widthStride % (1 << 16));

    return MFX_ERR_NONE;
}

mfxStatus ExternalHDDLAllocator::UnlockRemoteFrame(HddlUnite::RemoteMemory::Ptr rm, mfxFrameData* ptr) {
    if (!m_lock_buffer.size()) return MFX_ERR_INVALID_HANDLE;
    mfxU8* pBuffer = m_lock_buffer.data();

    if (!rm)
        return MFX_ERR_INVALID_HANDLE;

    HddlStatusCode hddlStatus = rm->syncToDevice(pBuffer, m_lock_buffer.size());
    if (hddlStatus != HDDL_OK)
        return MFX_ERR_LOCK_MEMORY;

    if (NULL != ptr)
    {
        ptr->PitchLow = 0;
        ptr->PitchHigh = 0;
        ptr->Y = NULL;
        ptr->U = NULL;
        ptr->V = NULL;
        ptr->A = NULL;
    }

    return MFX_ERR_NONE;
}

HddlUnite::RemoteMemory::Ptr ExternalHDDLAllocator::ExportRemoteMemory(mfxFrameSurface1* surface) {
    hddlMemId* vaapi_mid = (hddlMemId*)surface->Data.MemId;
    if (!vaapi_mid || !(vaapi_mid->m_surface))
        return 0;

    return vaapi_mid->m_surface;
}

void ExternalHDDLAllocator::ImportRemoteMemory(mfxFrameSurface1* surface, HddlUnite::RemoteMemory::Ptr rm) {
    hddlMemId* vaapi_mid = (hddlMemId*)surface->Data.MemId;
    if (!vaapi_mid || !(vaapi_mid->m_surface))
        return;

    vaapi_mid->m_surface = rm;
}

#endif // #if defined(LIBVA_SUPPORT) && defined(HDDL_UNITE) && !defined(MFX_HW_VSI_TARGET)
