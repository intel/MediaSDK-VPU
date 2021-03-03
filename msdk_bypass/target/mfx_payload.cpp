/*
 * Copyright (c) 2020 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//!
//! \file    mfx_payload.cpp
//! \brief   Target implementation of MFX proxy functions
//! \details Extract argument values from payload and call corresponding MFX function.
//!

#include "hddl_mfx_common.h"
#include "mfx_payload.h"
#include "mfxstructures.h"
#include <mutex>

#if defined(MFX_ONEVPL)
#include "mfximplcaps.h"
#endif

#if !defined(MFX_SHIM_WRAPPER)
#include "mfxvideo.h"

extern "C" {
#include "va_display.h"
}

#else
namespace mfx {
#include "mfxvideo.h"
}
#endif

#include <map>

#if defined(MFX_SHIM_WRAPPER)

namespace wrapper {
static void* mfx_dll = nullptr;

void* load_dll(const char* pFileName)
{
    void* hModule = nullptr;

#if !defined(MEDIASDK_UWP_DISPATCHER)
    // set the silent error mode
    DWORD prevErrorMode = 0;
#if (_WIN32_WINNT >= 0x0600)
    SetThreadErrorMode(SEM_FAILCRITICALERRORS, &prevErrorMode);
#else
    prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
#endif // !defined(MEDIASDK_UWP_DISPATCHER)

    // load the library's module
    hModule = LoadLibraryExA(pFileName, NULL, 0);
    DWORD err = GetLastError();
#if !defined(MEDIASDK_UWP_DISPATCHER)
    // set the previous error mode
#if (_WIN32_WINNT >= 0x0600)
    SetThreadErrorMode(prevErrorMode, NULL);
#else
    SetErrorMode(prevErrorMode);
#endif
#endif // !defined(MEDIASDK_UWP_DISPATCHER)
    return (void*)hModule;
} 

void* get_proc_addr(void*&  handle, const char *name)
{
    if (!handle)
        handle = load_dll("libmfxhw64_wrp.dla");
    return (void*)GetProcAddress((HMODULE)handle, name);
}

bool free_dll(void* handle)
{
    BOOL bRes = FreeLibrary((HMODULE)handle);
    return !!bRes;
} 

#define WRAP(a) wrapper::a

#undef FUNCTION

#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
static return_value MFX_CDECL func_name formal_param_list \
{ \
    typedef return_value (MFX_CDECL *func_type) formal_param_list; \
    static func_type func_ptr = nullptr; \
    if (!func_ptr) \
        func_ptr = (func_type)get_proc_addr(mfx_dll, #func_name); \
    if (func_ptr) \
        return func_ptr actual_param_list; \
    return MFX_ERR_UNSUPPORTED; \
}

#define FUNCTION1(return_value, func_name, formal_param_list, actual_param_list) \
static return_value MFX_CDECL func_name formal_param_list \
{ \
    typedef return_value (MFX_CDECL *func_type) formal_param_list; \
    static func_type func_ptr = nullptr; \
    if (!func_ptr) \
        func_ptr = (func_type)get_proc_addr(mfx_dll, #func_name); \
    return func_ptr actual_param_list; \
}

FUNCTION(mfxStatus, MFXInit, (mfxIMPL impl, mfxVersion *ver, mfxSession *session), (impl, ver, session))
FUNCTION(mfxStatus, MFXInitEx, (mfxInitParam par, mfxSession *session), (par, session))
FUNCTION(mfxStatus, MFXClose, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXJoinSession, (mfxSession session, mfxSession child_session), (session, child_session))
FUNCTION(mfxStatus, MFXCloneSession, (mfxSession session, mfxSession* child_session), (session, child_session))

FUNCTION(mfxStatus, MFXDisjoinSession, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXSetPriority, (mfxSession session, mfxPriority priority), (session, priority))
FUNCTION(mfxStatus, MFXGetPriority, (mfxSession session, mfxPriority *priority), (session, priority))
FUNCTION(mfxStatus, MFXQueryVersion, (mfxSession session, mfxVersion *version), (session, version))
FUNCTION(mfxStatus, MFXQueryIMPL, (mfxSession session, mfxIMPL *impl), (session, impl))
FUNCTION(mfxStatus, MFXVideoCORE_QueryPlatform, (mfxSession session, mfxPlatform* platform), (session, platform))

// CORE interface functions
FUNCTION(mfxStatus, MFXVideoCORE_SetFrameAllocator, (mfxSession session, mfxFrameAllocator *allocator), (session, allocator))
FUNCTION(mfxStatus, MFXVideoCORE_SetHandle, (mfxSession session, mfxHandleType type, mfxHDL hdl), (session, type, hdl))
FUNCTION(mfxStatus, MFXVideoCORE_GetHandle, (mfxSession session, mfxHandleType type, mfxHDL *hdl), (session, type, hdl))
FUNCTION(mfxStatus, MFXVideoCORE_SyncOperation, (mfxSession session, mfxSyncPoint syncp, mfxU32 wait), (session, syncp, wait))

// ENCODE interface functions
FUNCTION(mfxStatus, MFXVideoENCODE_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoENCODE_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoENCODE_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENCODE_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENCODE_Close, (mfxSession session), (session))

FUNCTION(mfxStatus, MFXVideoENCODE_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENCODE_GetEncodeStat, (mfxSession session, mfxEncodeStat *stat), (session, stat))
FUNCTION(mfxStatus, MFXVideoENCODE_EncodeFrameAsync, (mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp), (session, ctrl, surface, bs, syncp))

// DECODE interface functions
FUNCTION(mfxStatus, MFXVideoDECODE_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoDECODE_DecodeHeader, (mfxSession session, mfxBitstream *bs, mfxVideoParam *par), (session, bs, par))
FUNCTION(mfxStatus, MFXVideoDECODE_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoDECODE_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoDECODE_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoDECODE_Close, (mfxSession session), (session))

FUNCTION(mfxStatus, MFXVideoDECODE_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoDECODE_GetDecodeStat, (mfxSession session, mfxDecodeStat *stat), (session, stat))
FUNCTION(mfxStatus, MFXVideoDECODE_SetSkipMode, (mfxSession session, mfxSkipMode mode), (session, mode))
FUNCTION(mfxStatus, MFXVideoDECODE_GetPayload, (mfxSession session, mfxU64 *ts, mfxPayload *payload), (session, ts, payload))
FUNCTION(mfxStatus, MFXVideoDECODE_DecodeFrameAsync, (mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp), (session, bs, surface_work, surface_out, syncp))

#if defined(MFX_ONEVPL)
FUNCTION(mfxStatus, MFXInitialize, (mfxInitializationParam par, mfxSession* session), (par, session))
FUNCTION1(mfxHDL*, MFXQueryImplsDescription, (mfxImplCapsDeliveryFormat format, mfxU32* num_impls), (format, num_impls))
FUNCTION(mfxStatus, MFXReleaseImplDescription, (mfxHDL hdl), (hdl))
#endif
} //namespace wrapper
#else
#define WRAP(a)   a
#endif // defined(MFX_SHIM_WRAPPER)

template<typename Key>
class SurfaceMap
{
protected:
    std::map<Key, mfxFrameSurface1*> map_;
public:
    std::map<Key, mfxFrameSurface1*>& GetMap() { return map_; }

    mfxFrameSurface1* FindSurface(Key src)
    {
        auto ii = map_.find(src);
        if (ii != map_.end())
            return ii->second;
        return nullptr;
    }

    Key FindKey(mfxFrameSurface1* dst)
    {
        for (auto ii : map_)
            if (ii.second == dst)
                return ii.first;
        return nullptr;
    }

    Key FindByMid(mfxMemId mid)
    {
        for (auto ii : map_)
            if (ii.second->Data.MemId == mid)
                return ii.second;
        return nullptr;
    }

    void AddSurface(Key src, mfxFrameSurface1* target) {
        map_[src] = target;
    }

    void RemoveSurface(Key src) { map_.erase(src); }

    void ClearUnused()
    {
        std::map<Key, mfxFrameSurface1*> tmp_map;
        for (auto ii : map_)
            if (ii.second->Data.Locked)
                tmp_map[ii.first] = ii.second;
         std::swap(map_, tmp_map);
    }
};

using Sync2TagMap   = std::map<mfxSyncPoint,SyncDependentInfo>;
using Sync2BitsMap  = std::map<mfxSyncPoint, mfxBitstream*>;
using Bits2BitsMap  = std::map<mfxBitstream*, mfxBitstream*>;
using Sync2SurfMap  = SurfaceMap<mfxSyncPoint>;

struct SurfaceEntry {
    mfxFrameSurface1* surface;

    std::list<mfxU8*> for_free;

    bool is_free = true;
    bool was_output = false;
    uint32_t prev_locked = 0;

    std::vector<mfxU8> dataBuffer;

    SurfaceEntry(mfxFrameSurface1* s)
        : surface(0) {
        surface = new mfxFrameSurface1();
    }

    ~SurfaceEntry() {
        delete surface;
        Reset();
    }

    void Reset() {
        is_free = true;
        was_output = false;
        prev_locked = 0;

        for (auto& p : for_free)
            delete[] p;
        for_free.clear();
    }

    void UpdateState() {
        if (!prev_locked && !surface->Data.Locked)
            Reset();
    }
};

class SurfacesCache {
public:
    std::map<mfxFrameSurface1*, std::shared_ptr<SurfaceEntry>> map;

    void FreeEntry(mfxFrameSurface1* hostSurf) {
        SurfaceEntry * entry = FindEntryByHostSurf(hostSurf);
        if (entry)
            entry->Reset();
    }

    SurfaceEntry * FindEntryByHostSurf(mfxFrameSurface1* hostSurf) {
        auto ii = map.find(hostSurf);
        if (ii != map.end())
            return ii->second.get();
        return nullptr;
    }

    mfxFrameSurface1* FindTargetSurface(mfxFrameSurface1* hostSurf)
    {
        auto ii = map.find(hostSurf);
        if (ii != map.end())
            return ii->second->surface;
        return nullptr;
    }

    mfxFrameSurface1* FindHostSurface(mfxFrameSurface1* targetSurf)
    {
        for (auto ii : map)
            if (ii.second->surface == targetSurf)
                return ii.first;
        return nullptr;
    }

    mfxFrameSurface1* FindByMid(mfxMemId mid)
    {
        for (auto ii : map)
            if (ii.second->surface->Data.MemId == mid)
                return ii.second->surface;
        return nullptr;
    }

    bool IsEquals(mfxFrameSurface1* s1, mfxFrameSurface1* s2) {
        if (!s1 || !s2)
            return false;

        if (s1->Info.Width != s2->Info.Width ||
            s1->Info.Height != s2->Info.Height ||
            s1->Info.FourCC != s2->Info.FourCC)
            return false;

        return true;
    }

    SurfaceEntry* AddEntry(mfxFrameSurface1 * hostSurfPtr, mfxFrameSurface1 & hostSurf) {
        if (!FindEntryByHostSurf(hostSurfPtr)) {
            map[hostSurfPtr] = std::make_shared<SurfaceEntry>(hostSurfPtr);
        }

        SurfaceEntry* entry = FindEntryByHostSurf(hostSurfPtr);

        mfxFrameSurface1* surf = entry->surface;

        surf->Info = hostSurf.Info;

        surf->Data.DataFlag = hostSurf.Data.DataFlag;
        surf->Data.MemId = hostSurf.Data.MemId;
        surf->Data.MemType = hostSurf.Data.MemType;
        surf->Data.PitchHigh = hostSurf.Data.PitchHigh;
        surf->Data.TimeStamp = hostSurf.Data.TimeStamp;
        surf->Data.FrameOrder = hostSurf.Data.FrameOrder;
        surf->Data.PitchLow = hostSurf.Data.PitchLow;
        surf->Data.Corrupted = hostSurf.Data.Corrupted;

        if (entry->is_free) {
            surf->Data.Locked = hostSurf.Data.Locked;
            entry->prev_locked = surf->Data.Locked;
            entry->is_free = false;
        }

        surf->Data.NumExtParam = 0;
        surf->Data.ExtParam = 0;

        if (GetFramePointer(hostSurf.Info.FourCC, hostSurf.Data) && 
            (!GetFramePointer(entry->surface->Info.FourCC, entry->surface->Data) || !IsEquals(&hostSurf, entry->surface))) {
            // need to allocate
            mfxFrameData& data = surf->Data;
            mfxU32 pitch = (data.PitchHigh << 16) + (mfxU32)data.PitchLow;

            size_t size = 0;

            for (int p = 0; p < 4; p++) {
                size += GetPlaneSize(surf->Info, p, pitch);
            }

            entry->dataBuffer.resize(size);
            SetFrameData(surf->Info, &data, entry->dataBuffer.data());
        }

        return entry;
    }
};

class TargetAllocator;

class TargetSessionState : public ProxySessionBase, public Uncopyable
{
protected:
    Sync2TagMap sync_map;

public:

    Bits2BitsMap bs_bs;
    SurfacesCache surfCache;
    mfxHDL displayHDL = 0;

    std::recursive_mutex guard;

    std::shared_ptr<TargetAllocator> allocator;

    TargetSessionState(mfxSession session, mfxIMPL impl)
        : ProxySessionBase(session, impl) {
    }

    ~TargetSessionState() {
    }

    Bits2BitsMap& GetBits2BitsMap() { return bs_bs; }
    Sync2TagMap& GetSyncpMap() { return sync_map; }

    void WriteSurfaceLockCounts(PayloadWriter& out)
    {
        std::vector<SurfLockData> entries;

        {
            std::lock_guard<std::recursive_mutex> lock(guard);

            for (auto& entry : surfCache.map) {
                entry.second->UpdateState();

                if (entry.second->is_free)
                    continue;

                if (entry.second->surface->Data.Locked == entry.second->prev_locked)
                    continue;

                mfxU16 op = entry.second->prev_locked < entry.second->surface->Data.Locked;
                entry.second->prev_locked = entry.second->surface->Data.Locked;
                entry.second->UpdateState();

                SurfLockData data = { entry.first, op };
                SHIM_NORMAL_MESSAGE("%p: %d", entry.first, (int)entry.second->surface->Data.Locked);
                entries.push_back(data);
            }
        }

        out.Write((uint32_t)entries.size());
        SHIM_NORMAL_MESSAGE("WriteSurfaceLockCounts(%d)", (uint32_t)entries.size());

        for (auto& data : entries) {
            out.Write(data);
        }
    }
};

class TargetAppState : public ProxyAppState<TargetSessionState>
{
public:
    VADisplay vaDpy = 0;
    int drmFd = -1;

#if !defined(MFX_SHIM_WRAPPER)
public:
    ~TargetAppState()
    {
        if (vaDpy && drmFd >= 0)
            va_close_display(vaDpy, drmFd);
    }

    inline void SetHWDevice(mfxSession session, void* hdl) { vaDpy = hdl; }
    mfxStatus SetHWDevice(mfxSession session)
    {
        va_open_display(&vaDpy, &drmFd);

        if (!vaDpy)
            return SetError(MFX_ERR_DEVICE_FAILED);

        mfxStatus sts = MFXVideoCORE_SetHandle(session, MFX_HANDLE_VA_DISPLAY, vaDpy);
        return sts;
    }
    inline mfxStatus CheckHWDevice(mfxSession session)
    {
        return !vaDpy ? SetHWDevice(session) : MFX_ERR_NONE;
    }
    mfxStatus CloseHWDevice()
    {
        if (drmFd >= 0)
            va_close_display(vaDpy, drmFd);
        vaDpy = nullptr;
        drmFd = -1;
        return MFX_ERR_NONE;    
    }
#else
public:
    void SetHWDevice(mfxSession session, void* hdl) { vaDpy = hdl; }
    mfxStatus CheckHWDevice(mfxSession session) { return MFX_ERR_NONE; }
    mfxStatus CloseHWDevice() { return MFX_ERR_NONE; }
#endif
};

static  TargetAppState  app_state;

class TargetAllocator : public mfxFrameAllocator
{
    mfxSession session;
    HDDLShimCommContext * commCtx;
    std::unique_ptr<HDDLShimCommContext> commCtxAuto;

    std::map<mfxMemId, mfxU8*> lock_data;

public:
    TargetAllocator(mfxSession ses)
        : session(ses) {
        memset(&reserved, 0, sizeof(reserved));
        pthis = this;
        mfxFrameAllocator::Alloc = &Alloc_;
        mfxFrameAllocator::Lock = &Lock_;
        mfxFrameAllocator::Unlock = &Unlock_;
        mfxFrameAllocator::GetHDL = &GetHDL_;
        mfxFrameAllocator::Free = &Free_;
    }

    virtual ~TargetAllocator() {
        Close();
    }

#ifdef USE_INPLACE_EXT_ALLOC
    std::map<mfxMemId, mfxHDL> handles;

    void AddHDL(mfxMemId mid, mfxHDL handle) {
        handles[mid] = handle;
    }
#endif

protected:

    mfxStatus Alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {
        mfxDataPacket args;
        mfxDataPacket out;

        PayloadWriter command(args, eMFXAllocCB);
        command.Write(session);
        command.Write(*request);
        mfxStatus sts = Comm_Execute(commCtx, args, out);

        PayloadReader result(out);
        if (sts == MFX_ERR_NONE) {
            result.Read(*response);
            response->mids = new mfxMemId[response->NumFrameActual];
            result.Read(response->mids, response->NumFrameActual * sizeof(mfxMemId));
        }
        return sts;
    }

    mfxStatus Lock(mfxMemId mid, mfxFrameData* ptr)
    {
        if (!ptr)
            return MFX_ERR_NULL_PTR;

        mfxDataPacket args;
        mfxDataPacket out;

        PayloadWriter   command(args, eMFXLockCB);
        command.Write(session);
        command.Write(mid);

        command.Write(*ptr);

        TargetSessionState* env = app_state.GetSessionState(session);
        auto org_surf = env->surfCache.FindByMid(mid);

        mfxFrameInfo info;

        if (org_surf)
            info = org_surf->Info;

        command.Write(info);

        mfxStatus sts = Comm_Execute(commCtx, args, out);

        PayloadReader result(out);
       
        if (sts == MFX_ERR_NONE) {
            result.Read(*ptr);

            mfxU32 pitch = (ptr->PitchHigh << 16) + (mfxU32)ptr->PitchLow;

            size_t size = 0;

            for (int p = 0; p < 4; p++) {
                size += GetPlaneSize(org_surf->Info, p, pitch);
            }

            mfxU8 * buffer = new mfxU8[size];

            lock_data[mid] = buffer;

            SetFrameData(org_surf->Info, ptr, buffer);
            sts = ReadSurfaceData(result, info, *ptr);
        }

        return sts;
    }

    mfxStatus Unlock(mfxMemId mid, mfxFrameData* ptr)
    {
        if (!ptr)
            return MFX_ERR_NULL_PTR;

        mfxDataPacket args;
        mfxDataPacket out;

        PayloadWriter command(args, eMFXUnlockCB);
        command.Write(session);
        command.Write(mid);

        TargetSessionState* env = app_state.GetSessionState(session);
        auto org_surf = env->surfCache.FindByMid(mid);

        mfxFrameInfo info;

        if (org_surf)
            info = org_surf->Info;

        command.Write(info);

        mfxStatus sts = WriteSurfaceData(command, org_surf->Info, *ptr);
        
        if (sts != MFX_ERR_NONE)
            return sts;

        sts = Comm_Execute(commCtx, args, out);

        delete[] lock_data[mid];
        lock_data[mid] = 0;

        ptr->Pitch = 0;
        ptr->Y = 0;
        ptr->U = 0;
        ptr->V = 0;
        ptr->A = 0;
        return sts;
    }

    mfxStatus GetHDL(mfxMemId mid, mfxHDL* handle)
    {
#ifdef USE_INPLACE_EXT_ALLOC
        *handle = &handles[mid];
        return MFX_ERR_NONE;
#endif
        mfxDataPacket args;
        mfxDataPacket out;

        PayloadWriter command(args, eMFXGetHDLCB);
        command.Write(session);
        command.Write(mid);
        mfxStatus sts = Comm_Execute(commCtx, args, out);

        PayloadReader result(out);
        VASurfaceID* surf = new VASurfaceID;

        *handle = 0;
        if (sts == MFX_ERR_NONE) {
            result.Read(*surf);
            *handle = surf;
        }

        return sts;
    }

    mfxStatus Free(mfxFrameAllocResponse* response)
    {
        mfxDataPacket args;
        mfxDataPacket out;

        PayloadWriter command(args, eMFXFreeCB);
        command.Write(session);
        command.Write(*response);
        command.Write(response->mids, response->NumFrameActual * sizeof(mfxMemId));
        delete[] response->mids;
        mfxStatus sts = Comm_Execute(commCtx, args, out);
        return sts;
    }

    static mfxStatus Alloc_(mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
    {
        return reinterpret_cast<TargetAllocator*>(pthis)->Alloc(request, response);
    }
    static mfxStatus Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
    {
        return reinterpret_cast<TargetAllocator*>(pthis)->Lock(mid, ptr);
    }
    static mfxStatus Unlock_(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
    {
        return reinterpret_cast<TargetAllocator*>(pthis)->Unlock(mid, ptr);
    }
    static mfxStatus GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL* handle)
    {
        return reinterpret_cast<TargetAllocator*>(pthis)->GetHDL(mid, handle);
    }
    static mfxStatus Free_(mfxHDL pthis, mfxFrameAllocResponse* response)
    {
        return reinterpret_cast<TargetAllocator*>(pthis)->Free(response);
    }

public:
    void Close() {
#ifndef USE_INPLACE_EXT_ALLOC
#ifndef MFX_SHIM_WRAPPER
        mfxDataPacket args_;
        mfxDataPacket result_;

        PayloadWriter command(args_, eMFXLastCB);
        PayloadReader result(result_);
        command.Write(session);
        Comm_Write(commCtx, (int)args_.size(), (void*)args_.data());
        Comm_Disconnect(commCtx, TARGET);

        for (auto& p : lock_data) {
            delete[] p.second;
        }

        lock_data.clear();
        commCtx = 0;
        commCtxAuto.release();
#endif
#endif
    }

    void setCommContext(HDDLShimCommContext* mainCtx) {
#if !defined(MFX_SHIM_WRAPPER) && defined(USE_INPLACE_EXT_ALLOC)
        commCtx = mainCtx;
#endif
    }

    mfxFrameAllocator* InitTargetAllocator(HDDLShimCommContext* mainCtx, uint32_t rx, uint32_t tx)
    {
#ifndef MFX_SHIM_WRAPPER

#ifdef USE_INPLACE_EXT_ALLOC
        commCtx = mainCtx;
#else
        commCtxAuto.reset(new HDDLShimCommContext);
        commCtx = commCtxAuto.get();
        COMM_MODE(commCtx) = mainCtx->commMode;
        commCtx->pid = getpid();
        commCtx->tid = gettid();
        commCtx->batchPayload = NULL;
        IS_BATCH(commCtx) = false;

        commCtx->vaDpy = mainCtx->vaDpy;
        commCtx->vaDrmFd = mainCtx->vaDrmFd;
        commCtx->profile = mainCtx->profile;

        CommStatus commStatus = COMM_STATUS_SUCCESS;

        if ((commCtx)->commMode == COMM_MODE_XLINK)
        {
            (commCtx)->xLinkCtx = XLink_ContextInit(tx, rx);
        }
        else if ((commCtx)->commMode == COMM_MODE_TCP)
        {
            (commCtx)->tcpCtx = TCP_ContextInit(tx, rx,
                mainCtx->tcpCtx->server);
        }
        else if ((commCtx)->commMode == COMM_MODE_UNITE)
        {
            uint64_t workloadId = mainCtx->uniteCtx->workloadId != WORKLOAD_ID_NONE ?
                mainCtx->uniteCtx->workloadId : mainCtx->uniteCtx->internalWorkloadId;

            (commCtx)->uniteCtx = Unite_ContextInit(QUERY_FROM_UNITE, QUERY_FROM_UNITE,
                workloadId);
        }

        commStatus = Comm_Initialize(commCtx.get(), TARGET);
        if (commStatus != COMM_STATUS_SUCCESS)
        {
            SHIM_ERROR_MESSAGE("Dynamic Context: Error initializing communication settings");
            return 0;
        }

        commStatus = Comm_Connect(commCtx.get(), TARGET);
        if (commStatus != COMM_STATUS_SUCCESS)
        {
            SHIM_ERROR_MESSAGE("Dynamic Context: Failed to connect to communication settings");
            return 0;
        }
#endif
#endif
        return this;
    }

    mfxFrameAllocator* GetTargetAllocator() { return this; }
};

static void delete_bitstream(mfxBitstream*& bs)
{
    if (bs)
        delete [] bs->Data;
    delete bs;
    bs = nullptr;
}

static mfxStatus mfx_proxy_init(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    return MFX_ERR_NONE;
}

static mfxStatus mfx_proxy_term(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    
    return MFX_ERR_NONE;
}

static mfxStatus mfx_session_call(void* fun, PayloadReader& src, PayloadWriter& result, bool update_surfaces)
{
    mfxSession session = 0;
    src.Read(session);
    mfxStatus sts = (*reinterpret_cast<mfxStatus(MFX_CDECL*) (mfxSession)> (fun)) (session);

    if (update_surfaces) {
        TargetSessionState* env = app_state.GetSessionState(session);
        if (env) {
            env->WriteSurfaceLockCounts(result);
        }
    }

    return sts;
}

static mfxStatus mfx_session_video_param_call(void* fun, PayloadReader& src, PayloadWriter* result, bool save_ptr, bool update_surfaces)
{
    mfxSession session = 0;
    src.Read(session);

    mfxVideoParam *out_ptr = 0;
    src.ReadStruct(out_ptr);
    mfxStatus sts = (*reinterpret_cast<mfxStatus(MFX_CDECL *) (mfxSession, mfxVideoParam*)> (fun)) (session, out_ptr);
    if (result && save_ptr)
        result->WriteStruct(out_ptr);

    if (update_surfaces && result) {
        TargetSessionState* env = app_state.GetSessionState(session);
        if (env)
            env->WriteSurfaceLockCounts(*result);
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    return sts;
}

static mfxStatus mfx_query(void* fun, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    src.Read(session);

    app_state.CheckHWDevice(session);

    mfxVideoParam *in = nullptr;
    mfxVideoParam* out = nullptr;
    src.ReadStruct(in);
    src.ReadStruct(out);

    mfxStatus sts = (*reinterpret_cast<mfxStatus(MFX_CDECL *) (mfxSession, mfxVideoParam*, mfxVideoParam*)> (fun)) (session, in, out);
    result.WriteStruct(out);
    return sts;
}

static mfxStatus mfx_query_io_surf(void* fun, TargetSessionState** env, mfxFrameAllocRequest* request, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    src.Read(session);

    app_state.CheckHWDevice(session);

    mfxVideoParam *out = 0;
    src.ReadStruct(out);

    mfxStatus sts = (*reinterpret_cast<mfxStatus(MFX_CDECL *) (mfxSession, mfxVideoParam*, mfxFrameAllocRequest*)> (fun)) (session, out, request);
    result.Write(*request);
    if (sts == MFX_ERR_NONE)
    {
        *env = app_state.GetSessionState(session);
        if (!*env)
            return SetError(MFX_ERR_UNKNOWN);
    }
    return sts;
}

static mfxStatus mfx_initialize(HDDLShimCommContext* ctx, PayloadReader& in, PayloadWriter& out) {
#if defined(MFX_ONEVPL)
    SHIM_NORMAL_MESSAGE("MFXInitialize()");
    mfxInitializationParam par;
    in.Read(par);

    mfxSession session;

    mfxStatus sts = WRAP(MFXInitialize(par, &session));
    SHIM_NORMAL_MESSAGE("MFXInitialize(): %d %p", sts, session);
    out.Write(session);
    if (sts == MFX_ERR_NONE)
        app_state.AddSession(session, new (std::nothrow) TargetSessionState(session, par.AccelerationMode));

    if (ctx->vaDpy) {
        app_state.SetHWDevice(session, ctx->vaDpy);
        MFXVideoCORE_SetHandle(session, MFX_HANDLE_VA_DISPLAY, ctx->vaDpy);
    }

    return MFX_ERR_NONE;
#else
    return MFX_ERR_NONE;
#endif
}

static mfxStatus mfx_release_impls_desc(HDDLShimCommContext * ctx, PayloadReader & in, PayloadWriter & out) {
#if defined(MFX_ONEVPL)
    mfxHDL hdl;
    in.Read(hdl);

    mfxStatus sts = WRAP(MFXReleaseImplDescription(hdl));
    return sts;
#else
    return MFX_ERR_NONE;
#endif
}

static mfxStatus mfx_query_impls_desc(HDDLShimCommContext* ctx, PayloadReader& in, PayloadWriter& out) {
#if defined(MFX_ONEVPL)
    mfxImplCapsDeliveryFormat format;
    in.Read(format);

    mfxU32 num_impls = 0;
    mfxHDL * hdls = WRAP(MFXQueryImplsDescription(format, &num_impls));

    out.Write(num_impls);
    if (num_impls) {
        for (unsigned i = 0; i < num_impls; i++) {
            mfxImplDescription* implDesc = reinterpret_cast<mfxImplDescription*>(hdls[i]);
            out.Write(*implDesc);
        }
    }

#endif
    return MFX_ERR_NONE;
}

static mfxStatus mfx_init_ex(HDDLShimCommContext* ctx, PayloadReader& in, PayloadWriter& out)
{
    SHIM_NORMAL_MESSAGE("MFXInitEx()");
    mfxSession session = 0;
    mfxInitParam par;
    in.Read(par);
    mfxStatus status = WRAP(MFXInitEx(par, &session));
    SHIM_NORMAL_MESSAGE("MFXInitEx(): %d %p", status, session);
    out.Write(session);
    if (status == MFX_ERR_NONE)
        app_state.AddSession(session, new (std::nothrow) TargetSessionState(session, par.Implementation));

    if (ctx->vaDpy && (par.Implementation & -MFX_IMPL_VIA_ANY) == MFX_IMPL_VIA_HDDL) {
        MFXVideoCORE_SetHandle(session, MFX_HANDLE_VA_DISPLAY, ctx->vaDpy);
        app_state.SetHWDevice(session, ctx->vaDpy);
    }
    return status;
}


static mfxStatus mfx_close(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    src.Read(session);
    SHIM_NORMAL_MESSAGE("MFXClose(%p)", session);
    mfxStatus sts = WRAP(MFXClose(session));
    app_state.RemoveSession(session);
    if (!app_state.SessionCount())
        app_state.CloseHWDevice();
    return sts;
}

static mfxStatus mfx_decode_query(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_Query()");
    return mfx_query((void*)&WRAP(MFXVideoDECODE_Query), src, result);
}

static  mfxStatus mfx_decode_header(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxBitstream    bs;
    mfxVideoParam   *par = 0;

    src.Read(session);
    mfxStatus sts = MFX_ERR_NONE;

    TargetSessionState* env = app_state.GetSessionState(session);
    if (!env)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    bs.Data = nullptr;
    if ((sts = ReadBitstream(src, &bs, &env->GetBitstreamCache(), true)) != MFX_ERR_NONE)
        return sts;

    src.ReadStruct(par);

    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_DecodeHeader()");
    sts = WRAP(MFXVideoDECODE_DecodeHeader)(session, &bs, par);
    //PicStruct Type of the picture in the bitstream; this is an output parameter.
    //FrameType Frame type of the picture in the bitstream; this is an output parameter.
    result.Write(bs.DataOffset);
    result.Write(bs.DataLength);
    result.Write(bs.PicStruct);
    result.Write(bs.FrameType);
    result.WriteStruct(par);
    return sts;
}

static mfxStatus mfx_decode_query_io_surf(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    TargetSessionState* env = nullptr;
    mfxFrameAllocRequest request = {0};
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_QueryIOSurf()");
    mfxStatus sts = mfx_query_io_surf((void*)&WRAP(MFXVideoDECODE_QueryIOSurf), &env, &request, src, result);
    return sts;
}

static mfxStatus mfx_decode_init(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_Init()");
 
    mfxSession session = 0;
    src.Peek(session);
    app_state.CheckHWDevice(session);
    mfxStatus sts = mfx_session_video_param_call((void*)&WRAP(MFXVideoDECODE_Init), src, 0, false, false);

    if (sts == MFX_ERR_NONE) {
        mfxVideoParam par = {0};
        sts = WRAP(MFXVideoDECODE_GetVideoParam(session, &par));
        TargetSessionState* env = app_state.GetSessionState(session);
        if (!env)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        env->SetDecoderVPar(&par);
    }

    return sts;
}

static  mfxStatus mfx_decode_reset(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_Reset()");
    return mfx_session_video_param_call((void*)&WRAP(MFXVideoDECODE_Reset), src, &result, false, true);
}

static mfxStatus mfx_decode_get_param(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_GetVideoParam()");
    return mfx_session_video_param_call((void*)&WRAP(MFXVideoDECODE_GetVideoParam), src, &result, true, false);
}

static mfxStatus mfx_decode_close(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_Close()");
    return mfx_session_call((void*)&WRAP(MFXVideoDECODE_Close), src, result, true);
}

static  mfxStatus mfx_decode_get_stat(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxDecodeStat stat = { 0 };
    src.Read(session);
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_GetDecodeStat()");
    mfxStatus sts = WRAP(MFXVideoDECODE_GetDecodeStat)(session, &stat);
    result.Write(stat);
    return sts;
}

static  mfxStatus mfx_decode_set_skip_mode(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxSkipMode mode;
    src.Read(session);
    src.Read(mode);
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_SetSkipMode()");
    return WRAP(MFXVideoDECODE_SetSkipMode)(session, mode);
}

static  mfxStatus mfx_decode_get_payload(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxU64  ts = 0;
    mfxPayload *payload = 0;

    src.Read(session);
    src.ReadStruct(payload);

    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_GetPayload()");
    mfxStatus sts = WRAP(MFXVideoDECODE_GetPayload)(session, &ts, payload);
    if (sts != MFX_ERR_NONE)
        return sts;

    result.Write(ts);
    result.WriteStruct(payload);
    return sts;
}

static mfxStatus mfx_decode_frame_async(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxBitstream    bs, *bsp = nullptr;

    src.Read(session);
    mfxStatus sts = MFX_ERR_NONE;
    TargetSessionState* env = app_state.GetSessionState(session);
    if (!env)
        return SetError(MFX_ERR_UNKNOWN);

    // bitstream
    bs.Data = nullptr;
    src.Read(bsp);
    if (bsp)
    {
        bsp = &bs;
        if ((sts = ReadBitstream(src, &bs, &env->GetBitstreamCache(), true)) != MFX_ERR_NONE)
            return sts;
    }

    // work surface
    mfxFrameSurface1 *targetSurf = nullptr;
    mfxFrameSurface1* hostSurfPtr = 0;
    src.Read(hostSurfPtr);
    if (hostSurfPtr)
    {
        mfxFrameSurface1 hostSurf;
        src.Read(hostSurf);

        SurfaceEntry* entry = env->surfCache.AddEntry(hostSurfPtr, hostSurf);
        targetSurf = entry->surface;
        if ((sts = GetSurface(src, targetSurf, false)) != MFX_ERR_NONE)
            return sts;

#ifdef USE_INPLACE_EXT_ALLOC
        if (targetSurf->Data.MemId && env->allocator.get() &&
            !(env->GetDecoderIOPattern() & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)) {
            switch (env->GetImplementation() & -MFX_IMPL_VIA_ANY) {
            case MFX_IMPL_VIA_ANY:
            case MFX_IMPL_VIA_HDDL:
            case MFX_IMPL_VIA_VAAPI:
                {
                VASurfaceID handle;
                src.Read(handle);
                env->allocator->AddHDL(targetSurf->Data.MemId, reinterpret_cast<mfxHDL>(handle));
                }
                break;
            default:
                {
                mfxHDL handle;
                src.Read(handle);
                env->allocator->AddHDL(targetSurf->Data.MemId, reinterpret_cast<mfxHDL>(handle));
                }
                break;
            }
        }
#endif

    }

    mfxFrameSurface1* osp = 0;

    mfxSyncPoint    sync = nullptr;
    sts = SetError(WRAP(MFXVideoDECODE_DecodeFrameAsync(session, bsp, targetSurf, &osp, &sync)));
    SHIM_NORMAL_MESSAGE("MFXVideoDECODE_DecodeFrameAsync(): %d -> %p", sts, sync);

    auto org_surf = env->surfCache.FindHostSurface(osp);
    
    result.Write(sync);

    env->WriteSurfaceLockCounts(result);
    SyncDependentInfo   sdi(org_surf);
    result.Write(sdi);
    
    if (bsp) {
        result.Write(bsp->DataOffset);
        result.Write(bsp->DataLength);
    }

    if (targetSurf) {
        result.Write(targetSurf->Info);
        result.WriteExtBuffers(targetSurf->Data.ExtParam, targetSurf->Data.NumExtParam);
        result.Write(targetSurf->Data.TimeStamp);
        result.Write(targetSurf->Data.FrameOrder);
        result.Write(targetSurf->Data.DataFlag);
    }

    if (osp && sts >= MFX_ERR_NONE && sync)
    {
        std::lock_guard<std::recursive_mutex> lock(env->guard);
        env->GetSyncpMap()[sync] = sdi;
    }

    return sts;
}

static  mfxStatus mfx_core_sync_operation(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxSyncPoint syncp;
    mfxU32 wait;
    src.Read(session);
    src.Read(syncp);
    src.Read(wait);

    TargetSessionState*   env = app_state.GetSessionState(session);
    if (!env)
        return SetError(MFX_ERR_UNKNOWN);

    mfxStatus sts = SetError(WRAP(MFXVideoCORE_SyncOperation(session, syncp, wait)));
    SHIM_NORMAL_MESSAGE("MFXVideoCORE_SyncOperation(%p): %d", syncp, sts);

    SyncDependentInfo sdi = { 0 };

    {
        std::lock_guard<std::recursive_mutex> lock(env->guard);
        auto& map = env->GetSyncpMap();
        auto ii = map.find(syncp);
        if (ii == map.end()) {
            env->WriteSurfaceLockCounts(result);
            return SetError(MFX_WRN_OUT_OF_RANGE); // FIXME: no error code defined for an invalid sync point value
        }
        sdi = ii->second;
        if (sts == MFX_ERR_NONE)
            map.erase(ii);
    }

    if (sts == MFX_ERR_NONE)
    {
        result.Write(sdi);

        if (!sdi.bs)
        {   // Syncpoint from decoder
            mfxFrameSurface1* surf = env->surfCache.FindTargetSurface(sdi.surf);
            if (!surf)
                return SetError(MFX_ERR_UNKNOWN);
            mfxFrameSurface1* org_surf = env->surfCache.FindHostSurface(surf);
            if (!org_surf || org_surf != sdi.surf)
                return SetError(MFX_ERR_UNKNOWN);
            WriteSurface(result, surf, true);

            SurfaceEntry * entry = env->surfCache.FindEntryByHostSurf(sdi.surf);
            entry->was_output = true;
        } else if (!sdi.surf) {   // Syncpoint from encoder
            
            mfxBitstream* org_bs = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(env->guard);
                auto& b2b_map = env->GetBits2BitsMap();
                auto bi = b2b_map.find(sdi.bs);
                if (bi == b2b_map.end())
                    return SetError(MFX_ERR_UNKNOWN);
                org_bs = bi->second;
                b2b_map.erase(bi);
            }

            result.Write(org_bs);
            sts = WriteBitstream(result, sdi.bs, nullptr, true);
            delete_bitstream(sdi.bs);
        }
    }

    env->WriteSurfaceLockCounts(result);
    return sts;
}

static  mfxStatus mfx_encode_query(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_Query()");
    return mfx_query((void*)&WRAP(MFXVideoENCODE_Query), src, result);
}

static  mfxStatus mfx_encode_query_io_surf(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    TargetSessionState* env = nullptr;
    mfxFrameAllocRequest request = {0};
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_QueryIOSurf()");
    mfxStatus sts = mfx_query_io_surf((void*)&WRAP(MFXVideoENCODE_QueryIOSurf), &env, &request, src, result);
    return sts;
}

static  mfxStatus mfx_encode_init(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_Init()");
    mfxSession session = 0;
    src.Peek(session);
    app_state.CheckHWDevice(session);
    mfxStatus sts = mfx_session_video_param_call((void*)&WRAP(MFXVideoENCODE_Init), src, 0, false, false);

    if (sts == MFX_ERR_NONE) {
        mfxVideoParam par = {0};
        sts = WRAP(MFXVideoENCODE_GetVideoParam(session, &par));
        TargetSessionState* env = app_state.GetSessionState(session);
        if (!env)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        env->SetEncoderIOPattern(par.IOPattern);
    }

    return sts;
}

static  mfxStatus mfx_encode_reset(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_Reset()");
    return mfx_session_video_param_call((void*)&WRAP(MFXVideoENCODE_Reset), src, &result, false, true);
}

static  mfxStatus mfx_encode_close(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_Close()");
    return mfx_session_call((void*)&WRAP(MFXVideoENCODE_Close), src, result, true);
}

static  mfxStatus mfx_encode_get_param(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_GetVideoParam()");
    return mfx_session_video_param_call((void*)&WRAP(MFXVideoENCODE_GetVideoParam), src, &result, true, false);
}

static  mfxStatus mfx_encode_get_stat(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxEncodeStat stat = { 0 };

    src.Read(session);
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_GetEncodeStat()");
    mfxStatus sts = WRAP(MFXVideoENCODE_GetEncodeStat)(session, &stat);
    result.Write(stat);
    return sts;
}

static  mfxStatus mfx_encode_frame_async(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession  session = 0;
    mfxStatus   sts = MFX_ERR_NONE;
    src.Read(session);
    TargetSessionState*   env = app_state.GetSessionState(session);
    if (!env)
        return SetError(MFX_ERR_UNKNOWN);

    // ctrl
    
    mfxEncodeCtrl *ctrl_ptr = 0;
    src.ReadStruct(ctrl_ptr);

    // surface
    mfxFrameSurface1* hostSurfPtr = nullptr;
    src.Read(hostSurfPtr);
    mfxFrameSurface1 *targetSurf = nullptr;
    if (hostSurfPtr)
    {
        mfxFrameSurface1 hostSurf;
        src.Read(hostSurf);

        SurfaceEntry* entry = env->surfCache.AddEntry(hostSurfPtr, hostSurf);
        targetSurf = entry->surface;
        if ((sts = GetSurface(src, targetSurf, true)) != MFX_ERR_NONE)
            return sts;

        entry->for_free.splice(entry->for_free.end(), src.dataCache);

#ifdef USE_INPLACE_EXT_ALLOC
        if (targetSurf->Data.MemId && env->allocator.get() &&
            !(env->GetEncoderIOPattern() & MFX_IOPATTERN_IN_SYSTEM_MEMORY)) {
            switch (env->GetImplementation() & -MFX_IMPL_VIA_ANY) {
            case MFX_IMPL_VIA_ANY:
            case MFX_IMPL_VIA_HDDL:
            case MFX_IMPL_VIA_VAAPI:
            {
                VASurfaceID handle;
                src.Read(handle);
                env->allocator->AddHDL(targetSurf->Data.MemId, reinterpret_cast<mfxHDL>(handle));
            }
            break;
            default:
            {
                mfxHDL handle;
                src.Read(handle);
                env->allocator->AddHDL(targetSurf->Data.MemId, reinterpret_cast<mfxHDL>(handle));
            }
            break;
            }
        }
#endif
    }

    // Bitstream
    mfxBitstream *org_bs;
    src.Peek(org_bs);

    mfxBitstream* bs = new mfxBitstream();
    if ((sts = ReadBitstream(src, bs, nullptr, true)) != MFX_ERR_NONE)
        return sts;

    SyncDependentInfo sdi(nullptr, bs);

    mfxSyncPoint sync = nullptr;

    sts = SetError(WRAP(MFXVideoENCODE_EncodeFrameAsync(session, ctrl_ptr, targetSurf, bs, &sync)));
    SHIM_NORMAL_MESSAGE("MFXVideoENCODE_EncodeFrameAsync(): %d -> %p", sts, sync);

    result.Write(sync);
    env->WriteSurfaceLockCounts(result);

    if (sts >= MFX_ERR_NONE && sync)
    {
        std::lock_guard<std::recursive_mutex> lock(env->guard);
        env->GetBits2BitsMap()[bs] = org_bs;
        env->GetSyncpMap()[sync] = sdi;
    } else
        delete_bitstream(bs);

    return sts;
}

static  mfxStatus mfx_core_query_platform(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxPlatform platform;
    mfxSession session = 0;
    src.Read(session);

    SHIM_NORMAL_MESSAGE("MFXVideoCORE_QueryPlatform()");
    mfxStatus sts = WRAP(MFXVideoCORE_QueryPlatform(session, &platform));
    result.Write(platform);
    return sts;
}

mfxStatus mfx_core_set_frame_allocator(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    src.Read(session);
    mfxFrameAllocator* allocator;
    src.Read(allocator);

    TargetSessionState* env = app_state.GetSessionState(session);
    if (!env)
        return SetError(MFX_ERR_UNKNOWN);

#ifndef MFX_SHIM_WRAPPER
    if (allocator && !env->allocator.get()) {

        uint16_t rx, tx;

        if (!IS_UNITE_MODE(ctx)) {
            src.Read(rx);
            src.Read(tx);
        }

        env->allocator.reset(new TargetAllocator(session));
        env->allocator->InitTargetAllocator(ctx, rx, tx);
    }

    if (allocator)
        allocator = env->allocator.get();
#else
    env->allocator.reset(new TargetAllocator(session));
#endif

    SHIM_NORMAL_MESSAGE("MFXVideoCORE_SetFrameAllocator(%p)", session);
    mfxStatus sts = WRAP(MFXVideoCORE_SetFrameAllocator(session, allocator));
    return sts;
}

mfxStatus mfx_core_set_handle(HDDLShimCommContext *ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxHandleType type;

    mfxHDL hdl;

    src.Read(session);
    src.Read(type);
    src.Read(hdl);

    mfxHDL save = hdl;

#ifndef MFX_SHIM_WRAPPER
    hdl = ctx->vaDpy;
#endif

    app_state.SetHWDevice(session, hdl);

    SHIM_NORMAL_MESSAGE("MFXVideoCORE_SetHandle(%p, %p)", session, hdl);
    mfxStatus sts = WRAP(MFXVideoCORE_SetHandle(session, type, hdl));

    TargetSessionState* env = app_state.GetSessionState(session);
    if (env && sts == MFX_ERR_NONE)
        env->displayHDL = save;

    if (env->allocator.get()) {
        env->allocator->setCommContext(ctx);
    }

    return sts;
}

static  mfxStatus mfx_core_get_handle(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxHandleType type;
    mfxHDL hdl = nullptr;
    src.Read(session);
    src.Read(type);

    mfxStatus sts = WRAP(MFXVideoCORE_GetHandle(session, type, &hdl));
    SHIM_NORMAL_MESSAGE("MFXVideoCORE_GetHandle(%p): %p", session, hdl);

    TargetSessionState* env = app_state.GetSessionState(session);
    if (env)
        hdl = env->displayHDL;

    if (sts == MFX_ERR_NONE)
        result.Write(hdl);
    return sts;
}

static  mfxStatus mfx_join_session(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxSession child_session = 0;
    src.Read(session);
    src.Read(child_session);

    SHIM_NORMAL_MESSAGE("MFXJoinSession(%p, %p)", session, child_session);
    mfxStatus sts = WRAP(MFXJoinSession(session, child_session));
    if (sts == MFX_ERR_NONE)
    {
        TargetSessionState*   env = app_state.GetSessionState(session);
        TargetSessionState*   child_env = app_state.GetSessionState(child_session);
        if (!env || !child_env)
            return SetError(MFX_ERR_UNKNOWN);
        env->JoinSession(child_session);
        child_env->SetParentSession(session);
    }
    return sts;
}

static  mfxStatus mfx_disjoin_session(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    src.Read(session);

    SHIM_NORMAL_MESSAGE("MFXDisjoinSession(%p)", session);
    mfxStatus sts = WRAP(MFXDisjoinSession(session));
    {
        TargetSessionState* parent_env, *env = app_state.GetSessionState(session);
        if (!env || !(parent_env = app_state.GetSessionState(env->GetParentSession())))
            return SetError(MFX_ERR_UNKNOWN);
        env->SetParentSession(nullptr);
        parent_env->DisjoinSession(session);
    }
    return sts;
}

static  mfxStatus mfx_clone_session(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxSession child_session = 0;
    src.Read(session);

    mfxStatus sts = WRAP(MFXCloneSession(session, &child_session));
    SHIM_NORMAL_MESSAGE("MFXCloneSession(%p, %p)", session, child_session);
    if (sts == MFX_ERR_NONE)
    {
        result.Write(child_session);
        TargetSessionState* state = app_state.GetSessionState(session);
        if (!state)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        app_state.AddSession(child_session, new (std::nothrow) TargetSessionState(child_session, state->GetImplementation()));
    }
    return sts;
}

static  mfxStatus mfx_set_priority(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxPriority priority;
    src.Read(session);
    src.Read(priority);

    SHIM_NORMAL_MESSAGE("MFXSetPriority()");
    mfxStatus sts = WRAP(MFXSetPriority(session, priority));
    return sts;
}

static  mfxStatus mfx_get_priority(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxPriority priority;
    src.Read(session);

    SHIM_NORMAL_MESSAGE("MFXGetPriority()");
    mfxStatus sts = WRAP(MFXGetPriority(session, &priority));
    if (sts == MFX_ERR_NONE)
        result.Write(priority);
    return sts;
}

static  mfxStatus mfx_query_impl(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxIMPL impl;
    src.Read(session);

    SHIM_NORMAL_MESSAGE("MFXQueryIMPL()");
    mfxStatus sts = WRAP(MFXQueryIMPL(session, &impl));
    if (sts == MFX_ERR_NONE)
        result.Write(impl);
    return sts;
}

static  mfxStatus mfx_query_version(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result)
{
    mfxSession session = 0;
    mfxVersion version;
    src.Read(session);

    SHIM_NORMAL_MESSAGE("MFXQueryVersion()");
    mfxStatus sts = WRAP(MFXQueryVersion(session, &version));
    if (sts == MFX_ERR_NONE)
        result.Write(version);
    return sts;
}

static mfxTargetFunction   mfx_handlers[eMFXLastFunc - eMFXFirstFunc] = {
    nullptr,                    //eMFXInit,
    mfx_close,                  //eMFXClose,
    mfx_query_impl,             //eMFXQueryIMPL,
    mfx_query_version,          //eMFXQueryVersion,
    mfx_join_session,           //eMFX_JoinSession,
    mfx_disjoin_session,        //eMFX_DisjoinSession,
    mfx_clone_session,          //eMFX_CloneSession,
    mfx_set_priority,           //eMFXSetPriority,
    mfx_get_priority,           //eMFXGetPriority,
    mfx_init_ex,                //eMFXInitEx,

    mfx_initialize,             //eMFXInitialize,
    mfx_release_impls_desc,     //eMFXReleaseImplDescription,
    mfx_query_impls_desc,       //eMFXQueryImplsDescription,
    nullptr,                    //eMFXMemory_GetSurfaceForVPP,
    nullptr,                    //eMFXMemory_GetSurfaceForEncode,
    nullptr,                    //eMFXMemory_GetSurfaceForDecode,

    mfx_decode_query,           //eMFXVideoDECODE_Query,
    mfx_decode_header,          //eMFXVideoDECODE_DecodeHeader,
    mfx_decode_query_io_surf,   //eMFXVideoDECODE_QueryIOSurf,
    mfx_decode_init,            //eMFXVideoDECODE_Init,
    mfx_decode_reset,           //eMFXVideoDECODE_Reset,
    mfx_decode_close,           //eMFXVideoDECODE_Close,
    mfx_decode_get_param,       //eMFXVideoDECODE_GetVideoParam,
    mfx_decode_get_stat,        //eMFXVideoDECODE_GetDecodeStat,
    mfx_decode_set_skip_mode,   //eMFXVideoDECODE_SetSkipMode,
    mfx_decode_get_payload,     //eMFXVideoDECODE_GetPayload,
    mfx_decode_frame_async,     //eMFXVideoDECODE_DecodeFrameAsync,

    mfx_encode_query,           //eMFXVideoENCODE_Query,
    mfx_encode_query_io_surf,   //eMFXVideoENCODE_QueryIOSurf,
    mfx_encode_init,            //eMFXVideoENCODE_Init,
    mfx_encode_reset,           //eMFXVideoENCODE_Reset,
    mfx_encode_close,           //eMFXVideoENCODE_Close,
    mfx_encode_get_param,       //eMFXVideoENCODE_GetVideoParam,
    mfx_encode_get_stat,        //eMFXVideoENCODE_GeEncodeStat,
    mfx_encode_frame_async,     //eMFXVideoENCODE_EncodeFrameAsync,

    mfx_core_sync_operation,        //eMFXVideoCORE_SyncOperation,
    mfx_core_set_frame_allocator,   //eMFXVideoCORE_SetFrameAllocator
    mfx_core_query_platform,        //eMFXVideoCORE_QueryPlatform
    mfx_core_set_handle,            //eMFXVideoCORE_SetHandle,
    mfx_core_get_handle,            //eMFXVideoCORE_GetHandle,

    mfx_proxy_init, //eMFXProxyInit,
    mfx_proxy_term, //eMFXProxyTerm,
    //eMFXProxyTotal
};

mfxTargetFunction GetMFXHandler(size_t id)
{
    id -= eMFXFirstFunc;
    if (id < (eMFXLastFunc - eMFXFirstFunc))
        return mfx_handlers[id];
    return nullptr;
}

#if !defined(MFX_SHIM_WRAPPER)
// Execute callback
mfxStatus   ExecuteCB(HDDLShimCommContext* ctx, mfxCBId cb_id, const mfxDataPacket& command, mfxDataPacket& result)
{
    return MFX_ERR_NONE;
}

CommStatus CommWriteBypass(HDDLShimCommContext* ctx, int size, void* payload) {
    return Comm_Write(ctx, size, payload);
}

CommStatus CommReadBypass(HDDLShimCommContext* ctx, int size, void* payload) {
    return Comm_Read(ctx, size, payload);
}

CommStatus CommPeekBypass(HDDLShimCommContext* ctx, uint32_t* size, void* payload) {
    return Comm_Peek(ctx, size, payload);
}
#endif



//EOF
