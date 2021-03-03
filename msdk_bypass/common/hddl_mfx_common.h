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

#pragma once

#include "mfxdefs.h"
#include "mfxstructures.h"
#include "mfxvideo.h"
#include "comm_helper.h"
#include <map>
#include <list>
#include <memory>
#include <algorithm>

#define MFX_ENABLE_FOURCC_RGB565
#define USE_INPLACE_EXT_ALLOC

#define CB_RESULT_FLAG      0x4000

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    e##func_name,

enum mfxFunctionId
{
    eMFXFirstFunc = 1000,
    eMFXInit = eMFXFirstFunc,
    eMFXClose,
    eMFXQueryIMPL,
    eMFXQueryVersion,
    eMFXJoinSession,
    eMFXDisjoinSession,
    eMFXCloneSession,
    eMFXSetPriority,
    eMFXGetPriority,
    eMFXInitEx,
    
    eMFXInitialize,
    eMFXReleaseImplDescription,
    eMFXQueryImplsDescription,
    eMFXMemory_GetSurfaceForVPP,
    eMFXMemory_GetSurfaceForEncode,
    eMFXMemory_GetSurfaceForDecode,

    eMFXVideoDECODE_Query,
    eMFXVideoDECODE_DecodeHeader,
    eMFXVideoDECODE_QueryIOSurf,
    eMFXVideoDECODE_Init,
    eMFXVideoDECODE_Reset,
    eMFXVideoDECODE_Close,
    eMFXVideoDECODE_GetVideoParam,

    eMFXVideoDECODE_GetDecodeStat,
    eMFXVideoDECODE_SetSkipMode,
    eMFXVideoDECODE_GetPayload,
    eMFXVideoDECODE_DecodeFrameAsync,

    eMFXVideoENCODE_Query,
    eMFXVideoENCODE_QueryIOSurf,
    eMFXVideoENCODE_Init,
    eMFXVideoENCODE_Reset,
    eMFXVideoENCODE_Close,
    eMFXVideoENCODE_GetVideoParam,
    eMFXVideoENCODE_GetEncodeStat,
    eMFXVideoENCODE_EncodeFrameAsync,

    eMFXVideoCORE_SyncOperation,
    eMFXVideoCORE_SetFrameAllocator,
    eMFXVideoCORE_QueryPlatform,
    eMFXVideoCORE_SetHandle,
    eMFXVideoCORE_GetHandle,

    eMFXProxyInit,
    eMFXProxyTerm,
    eMFXLastFunc
};

enum mfxCBId
{
    eMFXFirstCB = 1100,
    eMFXAllocCB = eMFXFirstCB,
    eMFXLockCB,
    eMFXUnlockCB,
    eMFXGetHDLCB,
    eMFXFreeCB,
    eMFXLastCB
};

typedef mfxStatus(*mfxTargetFunction)(HDDLShimCommContext* ctx, PayloadReader& src, PayloadWriter& result);

struct SurfLockData
{
    mfxFrameSurface1* surf;
    mfxU16  locked;
};

struct SyncDependentInfo
{
    inline SyncDependentInfo(mfxFrameSurface1* surface = nullptr, mfxBitstream* bits = nullptr) : surf(surface), bs(bits) {}
    mfxFrameSurface1*   surf;
    mfxBitstream*       bs;
};

CommStatus CommWriteBypass(HDDLShimCommContext* ctx, int size, void* payload);
CommStatus CommReadBypass(HDDLShimCommContext* ctx, int size, void* payload);
CommStatus CommPeekBypass(HDDLShimCommContext* ctx, uint32_t* size, void* payload);

class Uncopyable
{
protected:
    Uncopyable() {}
    Uncopyable(const Uncopyable&)  = delete;
    Uncopyable(Uncopyable&&)  = delete;
    Uncopyable& operator = (const Uncopyable&) = delete;
    Uncopyable& operator = (Uncopyable&&) = delete;
};

template<typename T>
struct AutoTmpData
{
    T&  addr_;
    T   data_;
    explicit AutoTmpData(T& addr) : addr_(addr), data_(addr) {}
    ~AutoTmpData() { addr_ = data_; }
};

using BitstreamCache = std::vector<uint8_t>;
using SessionVector = std::vector<mfxSession>;

class ProxySessionBase
{
protected:
    mfxSession              parent_session_ = nullptr;
    SessionVector           joined_sessions_;
    mfxFrameAllocator*      allocator_ = nullptr;
    BitstreamCache          bs_cache_;
    mfxIMPL                 implementation;
    mfxU16                  enc_io_pattern_ = 0;

    mfxVideoParam           dec_par_ = { 0 };

public:
    mfxSession              session_;

    ProxySessionBase(mfxSession session, mfxIMPL impl)
        : session_(session)
        , implementation(impl) {
    }
    
    virtual ~ProxySessionBase() {}

    mfxIMPL GetImplementation() { return implementation;  }

    void SetParentSession(mfxSession session)  { parent_session_ = session; }
    mfxSession GetParentSession()  { return parent_session_; }
    void JoinSession(mfxSession session) { joined_sessions_.push_back(session); }

    void DisjoinSession(mfxSession session) { 
        joined_sessions_.erase(std::remove(joined_sessions_.begin(), joined_sessions_.end(), session), joined_sessions_.end()); 
    }

    const SessionVector& JoinedSessions() { return joined_sessions_; }
    BitstreamCache& GetBitstreamCache() { return bs_cache_; }
    void SetAllocator(mfxFrameAllocator* allocator) { allocator_ = allocator; }
    mfxFrameAllocator* GetAllocator()   { return allocator_; }

    mfxHDL GetAllocatorHandle(mfxMemId mid) { 
        mfxHDL res = nullptr; 
        mfxStatus sts = allocator_->GetHDL(allocator_->pthis, mid, &res);
        if (sts != MFX_ERR_NONE)
            res = 0;
        return res;
    }

    void SetDecoderVPar(mfxVideoParam* par) { 
        if (par)
            dec_par_ = *par; 
        else
            dec_par_ = {};
    }

    mfxVideoParam GetDecoderVPar() { return dec_par_; }

    mfxU16  GetDecoderIOPattern() { return dec_par_.IOPattern; }

    void    SetEncoderIOPattern(mfxU16 pattern) { enc_io_pattern_ = pattern; }
    mfxU16  GetEncoderIOPattern() { return enc_io_pattern_; }

    mfxStatus LockSurface(mfxFrameSurface1* surface);
    mfxStatus UnlockSurface(mfxFrameSurface1* surface)
    {
        return allocator_ ? allocator_->Unlock(allocator_->pthis, surface->Data.MemId, &surface->Data) : MFX_ERR_NONE;
    }
};

template<typename T>
class ProxyAppState : public Uncopyable
{
protected:
    using SessionStates = std::map<mfxSession, std::unique_ptr<T> >;
    SessionStates sessions_;

public:
    ProxyAppState() {}
    virtual ~ProxyAppState() {}

    T* GetSessionState(mfxSession session) {
        auto it = sessions_.find(session);
        return (it == sessions_.end() ? nullptr : it->second.get());
    }

    size_t SessionCount() { return sessions_.size(); }

    void AddSession(mfxSession session, T* state) {
        sessions_.emplace(session, state);
    }

    void RemoveSession(mfxSession session) {
        sessions_.erase(session);
    }
};

mfxU32 GetPlaneSize(const mfxFrameInfo & info, int plane, int width);
mfxStatus SetFrameData(const mfxFrameInfo & info, mfxFrameData *ptr, mfxU8 *sptr);
mfxU8* GetFramePointer(mfxU32 fourcc, mfxFrameData const& data);

// Return input error code
mfxStatus SetError(mfxStatus err);

// Read/Write surface
mfxStatus WriteSurface(PayloadWriter& cmd, mfxFrameSurface1 *surface, bool write_contents);
mfxStatus ReadSurface(PayloadReader& src, mfxFrameSurface1* surface, bool read_contents);
mfxStatus WriteSurfaceData(PayloadWriter& cmd, const mfxFrameInfo& info, const mfxFrameData& data);
mfxStatus ReadSurfaceData(PayloadReader& cmd, const mfxFrameInfo& info, const mfxFrameData& data);

// Read/Write surface with ext. buffers if any
mfxStatus PutSurface(PayloadWriter& cmd, mfxFrameSurface1* surf, bool write_contents);
mfxStatus GetSurface(PayloadReader& cmd, mfxFrameSurface1*& surf, bool read_contents);

// Read/Write bitstream
mfxStatus WriteBitstream(PayloadWriter& cmd, mfxBitstream* bs, BitstreamCache* cache, bool write_contents);
mfxStatus ReadBitstream(PayloadReader& src, mfxBitstream* bs, BitstreamCache* cache, bool read_contents);

// Set surface lock counter to the given value
void SetLockCount(mfxU16 value, mfxU16& res);

// Read and set lock counters to a set of surfaces
mfxStatus ReadSurfaceLockCounts(PayloadReader& in);

inline mfxStatus SetCBResult(mfxStatus r)
{
    return mfxStatus(r | CB_RESULT_FLAG);
}

inline mfxStatus GetCBResult(mfxStatus r)
{
    return (int)r < 0 ? r : mfxStatus(r & ~CB_RESULT_FLAG);
}

void CommLock(HDDLShimCommContext* ctx);
void CommUnlock(HDDLShimCommContext* ctx);

mfxStatus Comm_Write(HDDLShimCommContext *ctx, const mfxDataPacket& command);

mfxStatus Comm_Read(HDDLShimCommContext *ctx, mfxDataPacket& result);

mfxStatus Comm_Execute(HDDLShimCommContext *ctx, const mfxDataPacket& command, mfxDataPacket& result);

mfxStatus Comm_ExecuteNoLock(HDDLShimCommContext *ctx, const mfxDataPacket& command, mfxDataPacket& result);

// Execute callback
mfxStatus ExecuteCB(HDDLShimCommContext* ctx, mfxCBId cb_id, const mfxDataPacket& command, mfxDataPacket& result);

// Execute VA API call
mfxStatus ExecuteVA(HDDLShimCommContext* ctx, HDDLVAFunctionID functionID, const mfxDataPacket& command, mfxDataPacket& result);

//EOF
