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
//! \file    hddl_mfx_common.cpp
//! \brief   Implementations of functions for KMB MFX proxy host and target
//! \details Provide easier implementation of upper level code.
//!

#include "hddl_mfx_common.h"
#include <atomic>

#if defined(MFX_SHIM_WRAPPER)
#include "mfx_payload.h"
#endif

mfxStatus SetError(mfxStatus err)
{
    if (err < MFX_ERR_NONE)
        err = mfxStatus(err & MFX_ERR_UNKNOWN); // -1
    if (err > MFX_ERR_NONE)
        err = mfxStatus(err & MFX_ERR_UNKNOWN); // -1
    return err;
}

mfxStatus ProxySessionBase::LockSurface(mfxFrameSurface1* surface)
{
    mfxStatus sts = allocator_ ? allocator_->Lock(allocator_->pthis, surface->Data.MemId, &surface->Data) : MFX_ERR_NONE;
    if (sts == MFX_ERR_NONE && !GetFramePointer(surface->Info.FourCC, surface->Data))
        Throw(MFX_ERR_UNDEFINED_BEHAVIOR, "Lock() returns null plane pointer");
    return sts;
}

mfxStatus ReadSurfaceLockCounts(PayloadReader& out)
{
    uint32_t count;
    out.Read(count);
    SHIM_NORMAL_MESSAGE("ReadSurfaceLockCounts(%d)", count);
    while (count-- != 0)
    {
        SurfLockData data;
        out.Read(data);
        SHIM_NORMAL_MESSAGE("%p: %d -> %d", data.surf, (int)data.surf->Data.Locked, (int)data.locked);
        SetLockCount(data.locked, data.surf->Data.Locked);
    }
    return MFX_ERR_NONE;
}

// Write bitstream data and ext buffers if exist
mfxStatus WriteBitstream(PayloadWriter& cmd, mfxBitstream *bs, BitstreamCache* cache, bool write_contents)
{
    cmd.Write(bs);
    if (!bs)
        return MFX_ERR_NONE;

    if (bs->DataOffset + bs->DataLength > bs->MaxLength)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    cmd.Write(*bs);
    cmd.Write(bs->Data);
    if (!bs->Data)
        return MFX_ERR_NONE;

    uint64_t tag = cache ? 0 : bs->DataLength;

    if (cache && bs->DataLength)
    {
        if (!cache->empty()) {
            if (cache->size() < bs->DataOffset + bs->DataLength ||
                memcmp((const char*)bs->Data + bs->DataOffset, (const char*)&cache->at(bs->DataOffset), bs->DataLength) != 0)
                cache->clear();
        }

        if (cache->empty())
        {
            cache->resize(bs->DataOffset + bs->DataLength);
            memcpy(&cache->at(bs->DataOffset), bs->Data + bs->DataOffset, bs->DataLength);
            tag = bs->DataLength;
        }
    }

    if (!write_contents)
        tag = 0;
    cmd.Write(tag);
    if (tag)
        cmd.Write(bs->Data + bs->DataOffset, bs->DataLength);

    return MFX_ERR_NONE;
}

static size_t MAX_BS_SIZE = 256 * 1024 * 1024; // 256 Mb

// Check and read bitstream
mfxStatus ReadBitstream(PayloadReader& src, mfxBitstream* bs, BitstreamCache* cache, bool read_contents)
{
    mfxBitstream* orig_bs = 0;
    src.Read(orig_bs);
    if (!orig_bs) {
        bs = 0;
        return MFX_ERR_NONE;
    }

    src.Read(*bs);
    
    mfxU8* data_ptr_orig = 0;
    src.Read(data_ptr_orig);

    if (!data_ptr_orig) {
        bs->Data = data_ptr_orig;
        return MFX_ERR_NONE;
    }

    if (!read_contents && !data_ptr_orig)
        return MFX_ERR_NONE;

    if (bs->DataLength > MAX_BS_SIZE)
        return MFX_ERR_MEMORY_ALLOC;

    bs->MaxLength = std::min<mfxU32>(bs->MaxLength, MAX_BS_SIZE);

    uint64_t needReadData = 0;
    src.Read(needReadData);

    if (cache)
    {
        if (needReadData)
            cache->resize(bs->DataOffset + bs->DataLength);

        if (!cache->size())
            cache->resize(1);

        bs->Data = &cache->at(0);
    } else if (!bs->Data) {
        bs->Data = new uint8_t[bs->MaxLength];
    }

    if (needReadData)
        src.Read(bs->Data + bs->DataOffset, bs->DataLength);

    return MFX_ERR_NONE;
}

mfxStatus WriteSurfaceData(PayloadWriter& cmd, const mfxFrameInfo& info, const mfxFrameData & data)
{
    mfxU32 pitch = (data.PitchHigh << 16) + (mfxU32)data.PitchLow;

    if (data.Y)
        cmd.Write(data.Y, GetPlaneSize(info, 0, pitch));

    if (data.U)
        cmd.Write(data.U, GetPlaneSize(info, 1, pitch));

    if (data.V)
        cmd.Write(data.V, GetPlaneSize(info, 2, pitch));

    return MFX_ERR_NONE;
}

mfxStatus ReadSurfaceData(PayloadReader& cmd, const mfxFrameInfo& info, const mfxFrameData& data)
{
    mfxU32 pitch = (data.PitchHigh << 16) + (mfxU32)data.PitchLow;

    if (data.Y)
        cmd.Read(data.Y, GetPlaneSize(info, 0, pitch));

    if (data.U)
        cmd.Read(data.U, GetPlaneSize(info, 1, pitch));

    if (data.V)
        cmd.Read(data.V, GetPlaneSize(info, 2, pitch));

    return MFX_ERR_NONE;
}

mfxStatus WriteSurface(PayloadWriter& cmd, mfxFrameSurface1 *surface, bool write_contents)
{
    if (write_contents)
        WriteSurfaceData(cmd, surface->Info, surface->Data);
    return MFX_ERR_NONE;
}

mfxU8* GetFramePointer(mfxU32 fourcc, mfxFrameData const& data)
{
    switch (fourcc)
    {
    case MFX_FOURCC_RGB3:
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_BGR4:
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
#endif
    case MFX_FOURCC_ARGB16:
    case MFX_FOURCC_ABGR16:      return std::min({ data.R, data.G, data.B, data.A }); break;
#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:      return data.R; break;
#endif // MFX_ENABLE_FOURCC_RGB565
    case MFX_FOURCC_R16:         return reinterpret_cast<mfxU8*>(data.Y16); break;

    case MFX_FOURCC_AYUV:        return data.V; break;

    case MFX_FOURCC_UYVY:        return data.U; break;

    case MFX_FOURCC_A2RGB10:     return data.B; break;

#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y410:        return reinterpret_cast<mfxU8*>(data.Y410); break;
#endif

#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y416:        return reinterpret_cast<mfxU8*>(data.U16); break;
#endif

    default:                     return data.Y;
    }
}

mfxU32 GetPlaneSize(const mfxFrameInfo & info, int plane, int width)
{
    int height = info.Height;

    switch (info.FourCC) {
    case MFX_FOURCC_IYUV:
    case MFX_FOURCC_I010:
    case MFX_FOURCC_YV12:
        if (!plane)
            return width * height;
        if (plane == 1)
            return (width >> 1)*(height >> 1);
        if (plane == 2)
            return (width >> 1)*(height >> 1);
        break;
    case MFX_FOURCC_NV21:
    case MFX_FOURCC_NV12:
        if (!plane)
            return width * height;
        if (plane == 1)
            return width * (height >> 1);
        break;
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif
        if (!plane)
            return width * height;
        if (plane == 1)
            return width*(height >> 1);
        break;
    case MFX_FOURCC_P210:
        if (!plane)
            return width * height;
        if (plane == 1)
            return width*height;
        break;
    case MFX_FOURCC_UYVY:
    case MFX_FOURCC_YUY2:
        if (!plane)
            return width * height;
        if (plane == 1)
            return width*height;
        break;
    case MFX_FOURCC_RGB3:
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
#endif
        if (!plane)
            return width * height;
        break;
#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:
        if (!plane)
            return width * height;
        break;
#endif // MFX_ENABLE_FOURCC_RGB565
    case MFX_FOURCC_RGB4:
        if (plane == 2)
            return width * height;
        break;
    case MFX_FOURCC_BGR4:
    case MFX_FOURCC_AYUV:
        if (!plane)
            return width * height;
        break;
    case MFX_FOURCC_P8:
        if (!plane)
            return width * height;
        break;
    }

    return 0;
}

mfxStatus SetFrameData(const mfxFrameInfo & info, mfxFrameData *ptr, mfxU8 *sptr)
{
    switch (info.FourCC) {
    case MFX_FOURCC_NV12:
        ptr->Y = sptr;
        ptr->U = ptr->Y + GetPlaneSize(info, 0, ptr->Pitch);
        ptr->V = ptr->U + 1;
        break;
    case MFX_FOURCC_NV21:
        ptr->Y = sptr;
        ptr->V = ptr->Y + GetPlaneSize(info, 0, ptr->Pitch);
        ptr->U = ptr->V + 1;
        break;
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif
        ptr->Y = sptr;
        ptr->U = ptr->Y + GetPlaneSize(info, 0, ptr->Pitch);
        ptr->V = ptr->U + 2;
        break;
    case MFX_FOURCC_P210:
        ptr->Y = sptr;
        ptr->U = ptr->Y + GetPlaneSize(info, 0, ptr->Pitch);
        ptr->V = ptr->U + 2;
        break;
    case MFX_FOURCC_IYUV:
    case MFX_FOURCC_I010:
    case MFX_FOURCC_YV12:
        ptr->Y = sptr;
        ptr->V = ptr->Y + GetPlaneSize(info, 0, ptr->Pitch);
        ptr->U = ptr->V + GetPlaneSize(info, 1, ptr->Pitch);
        break;
    case MFX_FOURCC_UYVY:
        ptr->U = sptr;
        ptr->Y = ptr->U + 1;
        ptr->V = ptr->U + 2;
        break;
    case MFX_FOURCC_YUY2:
        ptr->Y = sptr;
        ptr->U = ptr->Y + 1;
        ptr->V = ptr->Y + 3;
        break;
#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:
        ptr->B = sptr;
        ptr->G = ptr->B;
        ptr->R = ptr->B;
        break;
#endif // MFX_ENABLE_FOURCC_RGB565
    case MFX_FOURCC_RGB3:
        ptr->B = sptr;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        break;
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
        ptr->B = sptr;
        ptr->G = ptr->B + ptr->Pitch*Height2;
        ptr->R = ptr->B + 2 * ptr->Pitch*Height2;;
        break;
#endif
    case MFX_FOURCC_RGB4:
        ptr->B = sptr;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->A = ptr->B + 3;
        break;
    case MFX_FOURCC_BGR4:
        ptr->A = sptr;
        ptr->R = ptr->A + 1;
        ptr->G = ptr->A + 2;
        ptr->B = ptr->A + 3;
        break;
    case MFX_FOURCC_A2RGB10:
        ptr->R = ptr->G = ptr->B = ptr->A = sptr;
        break;
    case MFX_FOURCC_P8:
        ptr->Y = sptr;
        ptr->U = 0;
        ptr->V = 0;
        break;
    case MFX_FOURCC_AYUV:
        ptr->V = sptr;
        ptr->U = ptr->V + 1;
        ptr->Y = ptr->V + 2;
        ptr->A = ptr->V + 3;
        break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus ReadSurface(PayloadReader& src, mfxFrameSurface1* surf, bool read_contents)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (read_contents)
        sts = ReadSurfaceData(src, surf->Info, surf->Data);
    return sts;
}

void SetLockCount(mfxU16 value, mfxU16& res)
{
    if (value) {
        std::atomic<uint16_t>* var = reinterpret_cast<std::atomic<uint16_t>*>(&res);
        std::atomic_fetch_add(var, (mfxU16)1);
    }

    if (!value) {
        std::atomic<uint16_t>* var = reinterpret_cast<std::atomic<uint16_t>*>(&res);
        std::atomic_fetch_sub(var, (mfxU16)1);
    }
}

mfxStatus PutSurface(PayloadWriter& cmd, mfxFrameSurface1* surf, bool write_contents)
{
    cmd.Write((void *)surf);
    mfxStatus sts = MFX_ERR_NONE;
    if (surf) {
        cmd.Write(*surf);

        sts = WriteSurface(cmd, surf, write_contents);
        if (sts == MFX_ERR_NONE)
            cmd.WriteExtBuffers(surf->Data.ExtParam, surf->Data.NumExtParam);
    }
    return sts;
}

mfxStatus GetSurface(PayloadReader& cmd, mfxFrameSurface1*& surf, bool read_contents)
{
    mfxStatus sts = ReadSurface(cmd, surf, read_contents);
    if (MFX_ERR_NONE != sts)
        return sts;
    cmd.ReadExtBuffers(surf->Data.ExtParam, surf->Data.NumExtParam);
    return sts;
}

#if !defined(MFX_SHIM_WRAPPER)

mfxStatus Comm_Read(HDDLShimCommContext *ctx, mfxDataPacket& result)
{
    PayloadHeader head;
    if (IS_TCP_MODE(ctx))
    {
        result.resize(sizeof(PayloadHeader));

        if (CommReadBypass(ctx, (uint32_t)result.size(), &result.at(0)) != COMM_STATUS_SUCCESS)
            return MFX_ERR_UNKNOWN;

        head = *reinterpret_cast<PayloadHeader*>(result.data());
        if (head.size > sizeof(PayloadHeader))
        {
            result.resize(head.size);
            if (CommReadBypass(ctx, head.size - sizeof(PayloadHeader), &result.at(sizeof(head))) != COMM_STATUS_SUCCESS)
                return MFX_ERR_UNKNOWN;
        }
    }
    else // if (IS_XLINK_MODE (ctx) || IS_UNITE_MODE (ctx))
    {
        void *payload = nullptr;
        uint32_t size = sizeof(PayloadHeader);

        // Read payload receive
        if (CommPeekBypass(ctx, &size, &payload) != COMM_STATUS_SUCCESS)
        {
            SHIM_ERROR_MESSAGE("Error peek pcie device");
            return MFX_ERR_UNKNOWN;
        }

        head = *reinterpret_cast<PayloadHeader*>(payload);

        if (!payload || !size || size < sizeof(PayloadHeader) || size > head.size)
            return MFX_ERR_UNKNOWN;
        
        result.resize(head.size);
        memcpy(result.data(), payload, size);
        
        //HDDLMemoryMgr_FreeMemory(payload);
        free(payload);

        head = *reinterpret_cast<PayloadHeader*>(result.data());

        if (head.size > DATA_MAX_SEND_SIZE)
        {
            if (CommReadBypass(ctx, head.size - DATA_MAX_SEND_SIZE, &result.at(DATA_MAX_SEND_SIZE)) != COMM_STATUS_SUCCESS) {
                SHIM_ERROR_MESSAGE("Error peek pcie device");
                return MFX_ERR_UNKNOWN;
            }
        }
    }
    return (mfxStatus)head.status;
}

mfxStatus Comm_ExecuteNoLock(HDDLShimCommContext *ctx, const mfxDataPacket& command, mfxDataPacket& result)
{
    mfxStatus sts = MFX_ERR_UNKNOWN;
    if (CommWriteBypass(ctx, (uint32_t)command.size(), (void*)command.data()) == COMM_STATUS_SUCCESS)
        sts = Comm_Read(ctx, result);
    return sts;
}

void LockMutex(pthread_mutex_t* mutex)
{
#ifndef WIN_HOST
    int32_t ret = pthread_mutex_lock(mutex);
    if (ret != 0)
    {
        SHIM_NORMAL_MESSAGE("can't lock the mutex!");
    }
#else
    EnterCriticalSection(&mutex->m_CritSec);
#endif
}

void UnlockMutex(pthread_mutex_t* mutex)
{
#ifndef WIN_HOST
    int32_t ret = pthread_mutex_unlock(mutex);
    if (ret != 0)
    {
        SHIM_NORMAL_MESSAGE("can't unlock the mutex!");
    }
#else
    LeaveCriticalSection(&mutex->m_CritSec);
#endif
}
void CommLock(HDDLShimCommContext* ctx)
{
    if (ctx->commMode == COMM_MODE_XLINK)
    {
        LockMutex(&ctx->xLinkCtx->xLinkMutex);
    }
    else if (ctx->commMode == COMM_MODE_TCP)
    {
        LockMutex(&ctx->tcpCtx->tcpMutex);
    }
    else if (ctx->commMode == COMM_MODE_UNITE)
    {
        LockMutex(&ctx->uniteCtx->xLinkCtx->xLinkMutex);
    }
}

void CommUnlock(HDDLShimCommContext* ctx)
{
    if (ctx->commMode == COMM_MODE_XLINK)
    {
        UnlockMutex(&ctx->xLinkCtx->xLinkMutex);
    }
    else if (ctx->commMode == COMM_MODE_TCP)
    {
        UnlockMutex(&ctx->tcpCtx->tcpMutex);
    }
    else if (ctx->commMode == COMM_MODE_UNITE)
    {
        UnlockMutex(&ctx->uniteCtx->xLinkCtx->xLinkMutex);
    }
}

mfxStatus Comm_Execute(HDDLShimCommContext *ctx, const mfxDataPacket& command, mfxDataPacket& result)
{
    CommLock(ctx);
    mfxDataPacket   cb_result;
    mfxStatus sts = Comm_ExecuteNoLock(ctx, command, result);
#if !defined(MFX_HW_KMB_TARGET)
    while ((mfxCBId)sts >= eMFXFirstCB && (mfxCBId)sts < eMFXLastCB)
    {
        cb_result.clear();
        sts = ExecuteCB(ctx, (mfxCBId)sts, result, cb_result);
        sts = Comm_ExecuteNoLock(ctx, cb_result, result);
    }
#else
    while ((mfxFunctionId)sts < eMFXFirstFunc)
    {
        cb_result.clear();
        sts = ExecuteVA(ctx, (HDDLVAFunctionID)sts, result, cb_result);
        sts = Comm_ExecuteNoLock(ctx, cb_result, result);
    }
#endif
    CommUnlock(ctx);
    return GetCBResult(sts);
}

mfxStatus Comm_Write(HDDLShimCommContext *ctx, const mfxDataPacket& command)
{
    CommLock(ctx);
    auto sts = CommWriteBypass(ctx, (uint32_t)command.size(), (void*)command.data());
    CommUnlock(ctx);
    return sts == COMM_STATUS_SUCCESS ? MFX_ERR_NONE : MFX_ERR_UNKNOWN;
}

#else //defined(MFX_SHIM_WRAPPER)

mfxStatus ExecuteMFXPayload(HDDLShimCommContext* ctx, const mfxDataPacket& args, mfxDataPacket& result)
{
    PayloadReader   in(args);
    PayloadWriter   out(result);

    uint32_t functionID = in.GetId();
    if (functionID < eMFXFirstFunc || functionID >= eMFXLastFunc)
        return MFX_ERR_UNSUPPORTED;

    mfxStatus sts = MFX_ERR_UNKNOWN;
    // Call corresponding function to handle MFX Function by ID
    mfxTargetFunction handler = GetMFXHandler(functionID);
    if (!handler)
    {
        SHIM_ERROR_MESSAGE("GetMFXHandler() returned NULL");
        return MFX_ERR_UNSUPPORTED;
    }
    try
    {
        sts = handler(ctx, in, out);
    }
    catch (RuntimeError& exc)
    {
        out.Clear();
        sts = mfxStatus(exc.GetCode());
    }
    catch (...)
    {
        sts = MFX_ERR_UNKNOWN;
    }
    SHIM_NORMAL_MESSAGE("status: %d\n", sts);
    // Write back processed result
    out.SetStatus(sts);
    return sts;
}

mfxStatus Comm_ExecuteNoLock(HDDLShimCommContext *ctx, const mfxDataPacket& command, mfxDataPacket& result)
{
    PayloadReader   in(command);
    result.clear();
    return ExecuteCB(ctx, (mfxCBId)in.GetId(), command, result);
}

mfxStatus Comm_Execute(HDDLShimCommContext *ctx, const mfxDataPacket& command, mfxDataPacket& result)
{
    //Comm_Lock(ctx);
    mfxDataPacket   cb_result;
    mfxStatus sts = ExecuteMFXPayload(ctx, command, result);
    while ((mfxCBId)sts >= eMFXFirstCB && (mfxCBId)sts < eMFXLastCB)
    {
        PayloadReader q(result);
        cb_result.clear();
        sts = ExecuteCB(ctx, (mfxCBId)sts, result, cb_result);
        result.clear();
        sts = ExecuteMFXPayload(ctx, cb_result, result);
    }
    //Comm_Unlock(ctx);
    return sts;
}

#endif //!defined(MFX_SHIM_WRAPPER)


