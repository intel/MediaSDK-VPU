/*
 * Copyright (c) 2019-2020 Intel Corporation. All Rights Reserved.
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
 //! \file    host_mfx_shim.cpp
 //! \brief   Basic MFX proxy functions for host 
 //! \details Receive/Return vaFunction with application & read/write va info with KMB Target
 //!


#include "mfxdefs.h"
#include "mfxvideo.h"
#include "comm_helper.h"
#include "hddl_mfx_common.h"
#include "hddl_va_shim_common.h"
#include <map>
#include <atomic>

#include <mfxjpeg.h>

#include "libmfx_allocator_hddl.h"

#ifdef WIN_HOST
#include <Windows.h>
extern "C" VADisplay vaGetDisplayWindowsDRM();
#else
#include "va/va_drm.h"
#endif

#include <thread>

class SimpleLoader
{
public:
    SimpleLoader(const char* name);

    void* GetFunction(const char* name);

    ~SimpleLoader();

private:
    SimpleLoader(SimpleLoader&);
    void operator=(SimpleLoader&);

    void* so_handle;
};

class Bypass_Proxy
{
private:
    SimpleLoader lib; // should appear first in member list

public:
    typedef CommStatus(*Comm_Write_type)(HDDLShimCommContext*, int, void*);
    typedef CommStatus(*Comm_Read_type)(HDDLShimCommContext*, int, void*);
    typedef CommStatus(*Comm_Peek_type)(HDDLShimCommContext*, uint32_t*, void*);

    Bypass_Proxy();
    ~Bypass_Proxy();

    const Comm_Write_type    Comm_Write;
    const Comm_Read_type     Comm_Read;
    const Comm_Peek_type     Comm_Peek;
};

class AllocatorThread {
public:

    AllocatorThread(HDDLShimCommContext* context)
        : thread(&thread_func, this)
        , mainCtx(context),
        tx(0),
        rx(0)
    {
    }

    ~AllocatorThread() {
        stop = true;
        if (thread.joinable())
            thread.join();

#ifndef USE_INPLACE_EXT_ALLOC
        if (inited)
            Comm_Disconnect(commCtx.get(), HOST);
#endif
    }

    CommStatus initContex() {
        uint16_t lastChannel = 0;
#ifndef USE_INPLACE_EXT_ALLOC
        CommStatus commStatus = Comm_GetLastChannel(mainCtx, &lastChannel);
        tx = ++(lastChannel);
        rx = ++(lastChannel);
        return commStatus;
#else 
        return COMM_STATUS_SUCCESS;
#endif
    }

    CommStatus connect() {
#ifndef USE_INPLACE_EXT_ALLOC
        commCtx.reset(new HDDLShimCommContext);
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
            // TOOD: fill in later
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

        commStatus = Comm_Initialize(commCtx.get(), HOST);
        if (commStatus != COMM_STATUS_SUCCESS)
        {
            SHIM_ERROR_MESSAGE("Dynamic Context: Error initializing communication settings");
            return commStatus;
        }

        commStatus = Comm_Connect(commCtx.get(), HOST);
        if (commStatus != COMM_STATUS_SUCCESS)
        {
            SHIM_ERROR_MESSAGE("Dynamic Context: Failed to connect to communication settings");
            return commStatus;
        }

        inited = true;
        return commStatus;
#else
        return COMM_STATUS_SUCCESS;
#endif
    }

    void run() {
        for (; !stop; )
        {
#ifdef USE_INPLACE_EXT_ALLOC
            break;
#endif
            if (!inited) {
                std::this_thread::yield();
                continue;
            }

            mfxDataPacket in;
            mfxCBId funcId = (mfxCBId)Comm_Read(commCtx.get(), in);

            if (funcId == eMFXLastCB)
                break;

            if (funcId < eMFXFirstCB || funcId >= eMFXLastCB)
                continue;

            mfxDataPacket out;
            mfxStatus sts = ExecuteCB(commCtx.get(), funcId, in, out);
            if (sts != MFX_ERR_NONE)
                break;

            CommStatus s = CommWriteBypass(commCtx.get(), (int)out.size(), (void*)out.data());
            if (s != COMM_STATUS_SUCCESS)
                break;

            std::this_thread::yield();
        }

        inited = false;
    }

    static void thread_func(AllocatorThread* data) {
        data->run();
    }

    uint16_t tx;
    uint16_t rx;

protected:
    std::unique_ptr<HDDLShimCommContext> commCtx;
    HDDLShimCommContext *mainCtx;
    bool stop = false;
    volatile bool inited = false;
    std::thread thread;
};

class HostSessionState : public ProxySessionBase
{
public:
    HDDLShimCommContext* comm_ctx = 0;
    VADisplay display = 0;
    bool need_delete_context = true;

    mfxStatus Init();
    void Close();

    VADisplay GetVADisplay() { return display; }
    HDDLShimCommContext* GetCommContext() { return comm_ctx; }

    HostSessionState(mfxSession session, mfxIMPL impl) : ProxySessionBase(session, impl) {}
    std::unique_ptr<AllocatorThread> allocatorThread;
#if defined(HDDL_UNITE)
    std::unique_ptr<HDDLAdapterFrameAllocator> m_hddlAllocator;
#endif
};

mfxStatus HostSessionState::Init() {
    if (display)
        return MFX_ERR_NONE;

#ifdef WIN_HOST
    display = vaGetDisplayWindowsDRM();
#else
    display = vaGetDisplayDRM(0);
#endif
    if (!display)
        return MFX_ERR_DEVICE_FAILED;

    int major, minor;
    VAStatus vaStatus = vaInitialize(display, &major, &minor);
    if (vaStatus != VA_STATUS_SUCCESS) {
        return MFX_ERR_DEVICE_FAILED;
    }

    VADriverContextP ctx = ((VADisplayContextP)display)->pDriverContext;
    HDDLVAShimDriverContext* context = (HDDLVAShimDriverContext*)ctx->pDriverData;
    if (!context || !context->mainCommCtx)
        return MFX_ERR_DEVICE_FAILED;

    comm_ctx = context->mainCommCtx;
    comm_ctx->vaDpy = display;
    return MFX_ERR_NONE;
}

void HostSessionState::Close() {
    if (need_delete_context)
        vaTerminate(display);
}

class HostAppState : public ProxyAppState<HostSessionState>
{
public:
    HDDLShimCommContext* GetCommContext(mfxSession session) { 
        auto session_state = GetSessionState(session);
        if (!session_state)
            return 0;

        return session_state->GetCommContext();
    }

    Bypass_Proxy bypass;
};

static  HostAppState  app_state;

#if defined(MFX_ONEVPL)
mfxStatus MFXInitialize(mfxInitializationParam par, mfxSession* session) {
    if (!session)
        return MFX_ERR_NULL_PTR;
    *session = nullptr;
    mfxStatus sts = MFX_ERR_NONE;

    if (!app_state.SessionCount()) {
        app_state.Init();
    }

    auto ctx = app_state.GetCommContext();
    if (!ctx)
        return MFX_ERR_DEVICE_FAILED;

    mfxDataPacket   command, result;
    PayloadWriter   cmd(command, eMFXInitialize);

    try
    {
        cmd.Write(par);
        sts = Comm_Execute(ctx, command, result);
        if (sts < MFX_ERR_NONE)
            return sts;
        PayloadReader   in(result);
        in.Read(*session);
        app_state.AddSession(*session, new (std::nothrow) HostSessionState(*session, par.AccelerationMode));
    }
    catch (RuntimeError& err)
    {
        sts = mfxStatus(err.GetCode());
    }

    return sts;
}

mfxHDL* MFXQueryImplsDescription(mfxImplCapsDeliveryFormat format, mfxU32* num_impls) {
    mfxStatus sts = MFX_ERR_NONE;

    if (format != MFX_IMPLCAPS_IMPLDESCSTRUCTURE)
        return 0;

    app_state.Init();

    auto ctx = app_state.GetCommContext();
    if (!ctx)
        return 0;

    mfxDataPacket   command, result;
    PayloadWriter   cmd(command, eMFXQueryImplsDescription);

    mfxHDL* hdl = 0;
    try
    {
        cmd.Write(format);
        sts = Comm_Execute(ctx, command, result);
        if (sts < MFX_ERR_NONE)
            return 0;

        PayloadReader in(result);

        mfxU32 num;
        in.Read(num);
        if (num_impls)
            *num_impls = num;

        if (num) {
            hdl = new mfxHDL[num];
            for (unsigned i = 0; i < num; i++) {
                hdl[i] = new mfxImplDescription();
                mfxImplDescription* implDesc = reinterpret_cast<mfxImplDescription*>(hdl[i]);
                in.Read(*implDesc);
            }
        }
    }
    catch (RuntimeError& err)
    {
        sts = mfxStatus(err.GetCode());
    }

    return hdl;
}

mfxStatus MFXReleaseImplDescription(mfxHDL hdl) {
    mfxStatus sts = MFX_ERR_NONE;

    app_state.Init();

    auto ctx = app_state.GetCommContext();
    if (!ctx)
        return MFX_ERR_DEVICE_FAILED;

    mfxDataPacket command, result;
    PayloadWriter cmd(command, eMFXReleaseImplDescription);

    try
    {
        cmd.Write(hdl);
        sts = Comm_Execute(ctx, command, result);
    }
    catch (RuntimeError& err)
    {
        sts = mfxStatus(err.GetCode());
    }

    return sts;
}

mfxStatus MFXMemory_GetSurfaceForVPP(mfxSession session, mfxFrameSurface1** surface) {
    return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXMemory_GetSurfaceForEncode(mfxSession session, mfxFrameSurface1** surface) {
    return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXMemory_GetSurfaceForDecode(mfxSession session, mfxFrameSurface1** surface) {
    return MFX_ERR_NOT_IMPLEMENTED;
}

#endif // #if defined(MFX_ONEVPL)

mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    *session = nullptr;
    mfxStatus sts = MFX_ERR_NONE;

    try
    {
        std::unique_ptr<HostSessionState> session_state(new HostSessionState(0, par.Implementation));

        session_state->Init();

        auto ctx = session_state->GetCommContext();
        if (!ctx)
            return MFX_ERR_DEVICE_FAILED;

        mfxDataPacket command, result;
        PayloadWriter cmd(command, eMFXInitEx);

        cmd.Write(par);
        sts = Comm_Execute(ctx, command, result);
        if (sts < MFX_ERR_NONE)
            return sts;

        PayloadReader   in(result);
        in.Read(*session);
        session_state->session_ = *session;
        app_state.AddSession(*session, session_state.release());
    }
    catch (RuntimeError& err)
    {
        sts = mfxStatus(err.GetCode());
    }
    return sts;
}

static mfxStatus exec_session_call(mfxFunctionId fun, mfxSession session, bool update_surfaces)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket command, result;
        PayloadWriter cmd(command, fun);
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, result);

        if (update_surfaces && result.size()) {
            PayloadReader in(result);
            ReadSurfaceLockCounts(in);
        }
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

static mfxStatus exec_session_in_video_param_call(mfxFunctionId fun, mfxSession session, mfxVideoParam *par, bool update_surfaces)
{
    if (!session || !par)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket command, res;
        PayloadWriter cmd(command, fun);
        cmd.Write(session);
        cmd.WriteStruct(par);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        if (update_surfaces) {
            PayloadReader in(res);
            ReadSurfaceLockCounts(in);
        }
        return sts;

    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

static mfxStatus exec_session_out_video_param_call(mfxFunctionId fun, mfxSession session, mfxVideoParam *out)
{
    if (!session || !out)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket command, res;
        PayloadWriter cmd(command, fun);
        cmd.Write(session);
        mfxStatus sts = MFX_ERR_NONE;
        cmd.WriteStruct(out);
        sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader result(res);
        result.ReadStruct(out);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

static mfxStatus exec_session_query_video_param_call(mfxFunctionId fun, mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    if (!session || !out)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket command, res;
        PayloadWriter cmd(command, fun);
        cmd.Write(session);
        cmd.WriteStruct(in);
        cmd.WriteStruct(out);

        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader result(res);
        result.ReadStruct(out);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

static mfxStatus exec_session_query_io_surf_call(mfxFunctionId fun, mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest* request)
{
    if (!session || !par || !request)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket command, res;
        PayloadWriter cmd(command, fun);
        cmd.Write(session);
        mfxStatus   sts = MFX_ERR_NONE;
        cmd.WriteStruct(par);

        sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader result(res);
        result.Read(*request);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXClose(mfxSession session)
{
    mfxStatus sts = exec_session_call(eMFXClose, session, false);

    auto* session_state = app_state.GetSessionState(session);
    if (session_state)
        session_state->Close();

    app_state.RemoveSession(session);
    return sts;
}

mfxStatus MFXQueryVersion(mfxSession session, mfxVersion *version)
{
    if (!session || !version)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXQueryVersion);
        cmd.Write(session);

        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader   result(res);
        if (sts == MFX_ERR_NONE)
            result.Read(*version);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXQueryIMPL(mfxSession session, mfxIMPL *impl)
{
    if (!session || !impl)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXQueryIMPL);
        cmd.Write(session);

        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader   result(res);
        if (sts == MFX_ERR_NONE)
            result.Read(*impl);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_DecodeHeader(mfxSession session, mfxBitstream *bs, mfxVideoParam *par)
{
    if (!session || !bs || !par)
        return MFX_ERR_NULL_PTR;
    mfxDataPacket   command, result;
    PayloadWriter   cmd(command, eMFXVideoDECODE_DecodeHeader);
    cmd.Write(session);

    try
    {
        mfxStatus   sts = MFX_ERR_NONE;
        auto session_state = app_state.GetSessionState(session);
        if (!session_state)
            return MFX_ERR_UNKNOWN;

        // Write bitstream data
        if ((sts = WriteBitstream(cmd, bs, &session_state->GetBitstreamCache(), true)) != MFX_ERR_NONE)
            return sts;

        cmd.WriteStruct(par);

        // Write video params buffer headers if any
        sts = Comm_Execute(app_state.GetCommContext(session), command, result);
        PayloadReader   in(result);
        //PicStruct Type of the picture in the bitstream; this is an output parameter.
        //FrameType Frame type of the picture in the bitstream; this is an output parameter.    out.Read(*par);
        // TODO: check for mfxExtCodingOptionSPSPPS data
        in.Read(bs->DataOffset);
        in.Read(bs->DataLength);
        in.Read(bs->PicStruct);
        in.Read(bs->FrameType);
        in.ReadStruct(par);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    return exec_session_query_video_param_call(eMFXVideoDECODE_Query, session, in, out);
}

mfxStatus MFXVideoDECODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest* request)
{
    return exec_session_query_io_surf_call(eMFXVideoDECODE_QueryIOSurf, session, par, request);
}

mfxStatus MFXVideoDECODE_Init(mfxSession session, mfxVideoParam *par)
{
    if (par && app_state.GetSessionState(session))
        app_state.GetSessionState(session)->SetDecoderVPar(par);
    return exec_session_in_video_param_call(eMFXVideoDECODE_Init, session, par, false);
}

mfxStatus MFXVideoDECODE_Reset(mfxSession session, mfxVideoParam *par)
{
    return exec_session_in_video_param_call(eMFXVideoDECODE_Reset, session, par, true);
}

mfxStatus MFXVideoDECODE_Close(mfxSession session)
{
    return exec_session_call(eMFXVideoDECODE_Close, session, true);
}

mfxStatus MFXVideoDECODE_GetVideoParam(mfxSession session, mfxVideoParam *out)
{
    return exec_session_out_video_param_call(eMFXVideoDECODE_GetVideoParam, session, out);
}

mfxStatus MFXVideoDECODE_GetDecodeStat(mfxSession session, mfxDecodeStat* stat)
{
    if (!session || !stat)
        return MFX_ERR_NULL_PTR;
    mfxDataPacket command, result;
    PayloadWriter cmd(command, eMFXVideoDECODE_GetDecodeStat);

    try
    {
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, result);
        PayloadReader in(result);
        in.Read(*stat);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_SetSkipMode(mfxSession session, mfxSkipMode mode)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    mfxDataPacket command, result;
    PayloadWriter cmd(command, eMFXVideoDECODE_SetSkipMode);

    try
    {
        cmd.Write(session);
        cmd.Write(mode);
        return Comm_Execute(app_state.GetCommContext(session), command, result);
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_GetPayload(mfxSession session, mfxU64* ts, mfxPayload* payload)
{
    if (!session || !ts || !payload || !payload->Data)
        return MFX_ERR_NULL_PTR;
    mfxDataPacket command, result;
    PayloadWriter cmd(command, eMFXVideoDECODE_GetPayload);

    try
    {
        cmd.Write(session);
        cmd.WriteStruct(payload);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, result);
        if (sts != MFX_ERR_NONE)
            return sts;
        PayloadReader   in(result);
        in.Read(*ts);
        in.ReadStruct(payload);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out_ptr, mfxSyncPoint *syncp)
{
    if (!session || !surface_work || !surface_out_ptr || !syncp)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket command, res;
        PayloadWriter cmd(command, eMFXVideoDECODE_DecodeFrameAsync);
        cmd.Write(session);

        mfxStatus sts = MFX_ERR_NONE;
        auto session_state = app_state.GetSessionState(session);
        if (!session_state)
            return MFX_ERR_UNKNOWN;

        cmd.Write(bs);
        if (bs) { // Write bitstream data
            if ((sts = WriteBitstream(cmd, bs, &session_state->GetBitstreamCache(), true)) != MFX_ERR_NONE)
                return sts;
        }

        //  W/A: sample_decode allocates surfaces with zero in CropW/CropH (for HEVCd only)
        if (surface_work)
        {
            if (!surface_work->Info.CropW)
                surface_work->Info.CropW = surface_work->Info.Width;
            if (!surface_work->Info.CropH)
                surface_work->Info.CropH = surface_work->Info.Height;
        }

        mfxVideoParam par = session_state->GetDecoderVPar();

        bool jpeg_interlace = (par.mfx.CodecId == MFX_CODEC_JPEG && par.mfx.FrameInfo.PicStruct > MFX_PICSTRUCT_PROGRESSIVE);
        bool need_lock = surface_work &&
            ((session_state->GetDecoderIOPattern() & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || jpeg_interlace) &&
            !GetFramePointer(surface_work->Info.FourCC, surface_work->Data);

        if (need_lock)
            session_state->LockSurface(surface_work);

        if ((sts = PutSurface(cmd, surface_work, false)) != MFX_ERR_NONE)
            return sts;

        if (need_lock)
            session_state->UnlockSurface(surface_work);

#ifdef USE_INPLACE_EXT_ALLOC
        if (surface_work && surface_work->Data.MemId && session_state->GetAllocator() && 
            (session_state->GetDecoderIOPattern() & MFX_IOPATTERN_OUT_VIDEO_MEMORY)) {
            mfxHDL handle = session_state->GetAllocatorHandle(surface_work->Data.MemId);

            switch (session_state->GetImplementation() & -MFX_IMPL_VIA_ANY) {
            case MFX_IMPL_VIA_ANY:
            case MFX_IMPL_VIA_HDDL:
            case MFX_IMPL_VIA_VAAPI:
                cmd.Write(handle  ? *(VASurfaceID*)handle : 0);
                break;
            default:
                cmd.Write(handle);
                break;
            }
        }
#endif

        SHIM_NORMAL_MESSAGE("MFXVideoDECODE_DecodeFrameAsync(%p)", bs);
        sts = Comm_Execute(app_state.GetCommContext(session), command, res);

        PayloadReader result(res);
        result.Read(*syncp);
        SHIM_NORMAL_MESSAGE("MFXVideoDECODE_DecodeFrameAsync(): %d -> %p", sts, *syncp);
        ReadSurfaceLockCounts(result);

        SyncDependentInfo sdi;
        result.Read(sdi);

        if (bs)
        {
            result.Read(bs->DataOffset);
            result.Read(bs->DataLength);
        }

        if (surface_work) {
            result.Read(surface_work->Info);
            result.ReadExtBuffers(surface_work->Data.ExtParam, surface_work->Data.NumExtParam);
            result.Read(surface_work->Data.TimeStamp);
            result.Read(surface_work->Data.FrameOrder);
            result.Read(surface_work->Data.DataFlag);
        }

        mfxFrameSurface1* data_surf = sdi.surf;
        *surface_out_ptr = data_surf;
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoENCODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    return exec_session_query_video_param_call(eMFXVideoENCODE_Query, session, in, out);
}

mfxStatus MFXVideoENCODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest* request)
{
    return exec_session_query_io_surf_call(eMFXVideoENCODE_QueryIOSurf, session, par, request);
}

mfxStatus MFXVideoENCODE_Init(mfxSession session, mfxVideoParam *par)
{
    if (par && app_state.GetSessionState(session))
        app_state.GetSessionState(session)->SetEncoderIOPattern(par->IOPattern);

    return exec_session_in_video_param_call(eMFXVideoENCODE_Init, session, par, false);
}

mfxStatus MFXVideoENCODE_Reset(mfxSession session, mfxVideoParam *par)
{
    return exec_session_in_video_param_call(eMFXVideoENCODE_Reset, session, par, true);
}

mfxStatus MFXVideoENCODE_Close(mfxSession session)
{
    return exec_session_call(eMFXVideoENCODE_Close, session, true);
}

mfxStatus MFXVideoENCODE_GetVideoParam(mfxSession session, mfxVideoParam *out)
{
    return exec_session_out_video_param_call(eMFXVideoENCODE_GetVideoParam, session, out);
}

mfxStatus MFXVideoENCODE_EncodeFrameAsync(mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
    if (!session || !bs || !syncp)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket command, res;
        PayloadWriter cmd(command, eMFXVideoENCODE_EncodeFrameAsync);
        cmd.Write(session);

        mfxStatus sts = MFX_ERR_NONE;
        auto session_state = app_state.GetSessionState(session);
        if (!session_state)
            return MFX_ERR_UNKNOWN;

        // EncodeControl
        cmd.WriteStruct(ctrl);

        bool need_lock = surface &&
            (session_state->GetEncoderIOPattern() & MFX_IOPATTERN_IN_SYSTEM_MEMORY) &&
            !GetFramePointer(surface->Info.FourCC, surface->Data);
        if (need_lock)
            session_state->LockSurface(surface);

        if ((sts = PutSurface(cmd, surface, true)) != MFX_ERR_NONE)
            return sts;

        if (need_lock)
            session_state->UnlockSurface(surface);

#ifdef USE_INPLACE_EXT_ALLOC
        if (surface && surface->Data.MemId && session_state->GetAllocator() &&
            !(session_state->GetEncoderIOPattern() & MFX_IOPATTERN_IN_SYSTEM_MEMORY)) {
            mfxHDL handle = session_state->GetAllocatorHandle(surface->Data.MemId);

            switch (session_state->GetImplementation() & -MFX_IMPL_VIA_ANY) {
            case MFX_IMPL_VIA_ANY:
            case MFX_IMPL_VIA_HDDL:
            case MFX_IMPL_VIA_VAAPI:
                cmd.Write(handle ? *(VASurfaceID*)handle : 0);
                break;
            default:
                cmd.Write(handle);
                break;
            }
        }
#endif

        // Bitstream
        if ((sts = WriteBitstream(cmd, bs, nullptr, true)) != MFX_ERR_NONE)
            return sts;

        SHIM_NORMAL_MESSAGE("MFXVideoENCODE_EncodeFrameAsync(%p)", bs);
        sts = Comm_Execute(app_state.GetCommContext(session), command, res);

        PayloadReader   result(res);
        result.Read(*syncp);
        SHIM_NORMAL_MESSAGE("MFXVideoENCODE_EncodeFrameAsync(): %d -> %p", sts, *syncp);
        ReadSurfaceLockCounts(result);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoENCODE_GetEncodeStat(mfxSession session, mfxEncodeStat* stat)
{
    if (!session || !stat)
        return MFX_ERR_NULL_PTR;
    mfxDataPacket command, result;
    PayloadWriter cmd(command, eMFXVideoENCODE_GetEncodeStat);

    try
    {
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, result);
        PayloadReader in(result);
        in.Read(*stat);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait)
{
    if (!session || !syncp)
        return MFX_ERR_NULL_PTR;

    mfxDataPacket   command, res;
    SHIM_NORMAL_MESSAGE("MFXVideoCORE_SyncOperation(%p)", syncp);
    PayloadWriter cmd(command, eMFXVideoCORE_SyncOperation);
    cmd.Write(session);
    cmd.Write(syncp);
    cmd.Write(wait);

    mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
    SHIM_NORMAL_MESSAGE("MFXVideoCORE_SyncOperation(%p): %d", syncp, sts);

    PayloadReader result(res);
    if (sts == MFX_WRN_OUT_OF_RANGE)
        syncp = nullptr;

    if (sts == MFX_ERR_NONE)
    {
        auto* session_state = app_state.GetSessionState(session);
        if (!session_state)
            return MFX_ERR_UNKNOWN;

        SyncDependentInfo tag;
        result.Read(tag);
        if (!tag.bs)
        {   // Syncpoint for decoder
            mfxFrameSurface1* surface_out = tag.surf;
            if (!surface_out)
                return MFX_ERR_UNKNOWN;

            mfxVideoParam par = session_state->GetDecoderVPar();
            bool jpeg_interlace = (par.mfx.CodecId == MFX_CODEC_JPEG && par.mfx.FrameInfo.PicStruct > MFX_PICSTRUCT_PROGRESSIVE);
            bool need_lock = surface_out &&
                ((session_state->GetDecoderIOPattern() & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || jpeg_interlace) &&
                !GetFramePointer(surface_out->Info.FourCC, surface_out->Data);
            if (need_lock)
                session_state->LockSurface(surface_out);

            ReadSurface(result, surface_out, true);

            if (need_lock)
                session_state->UnlockSurface(surface_out);
        } else if (!tag.surf) { // Syncpoint for encoder
            mfxBitstream* bs_out = nullptr;
            result.Read(bs_out);
            if (!bs_out)
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            sts = ReadBitstream(result, bs_out, nullptr, true);
        }
    }

    if (sts == MFX_WRN_OUT_OF_RANGE)
        sts = MFX_ERR_NONE; // TODO: sample_multi_transcode called twice with the same syncp value

    ReadSurfaceLockCounts(result);
    return sts;
}

mfxStatus MFXVideoCORE_SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait) { 
    mfxStatus sts = MFX_WRN_IN_EXECUTION;
    const int delay = wait + 100;

    try
    {
        if (wait < delay) {
            return SyncOperation(session, syncp, wait);
        }

        for (uint32_t i = 0; i < wait / delay; i++) {
            sts = SyncOperation(session, syncp, delay);
            if (sts != MFX_WRN_IN_EXECUTION)
                break;
        }

        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket command, res;
        auto *session_state = app_state.GetSessionState(session);
        if (!session_state)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        if (session_state->GetAllocator() == allocator)
            return MFX_ERR_NONE;

        session_state->SetAllocator(allocator);

        PayloadWriter cmd(command, eMFXVideoCORE_SetFrameAllocator);
        cmd.Write(session);
        cmd.Write(allocator);

        uint32_t needInit = allocator && !session_state->allocatorThread.get();

#if defined(HDDL_UNITE)
        if (allocator && (session_state->GetImplementation() & -MFX_IMPL_VIA_ANY) == MFX_IMPL_VIA_HDDL)
        {
            session_state->m_hddlAllocator.reset(new HDDLAdapterFrameAllocator(*allocator, session_state->GetVADisplay()));
            mfxStatus sts = session_state->m_hddlAllocator->Init();
            if (sts != MFX_ERR_NONE)
                return sts;

            allocator = session_state->m_hddlAllocator.get();
            session_state->SetAllocator(allocator);
        }
#endif
#ifdef MFX_SHIM_WRAPPER
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        needInit = false;
#else
        if (needInit) {
            session_state->allocatorThread.reset(new AllocatorThread(app_state.GetCommContext(session)));
            CommStatus res = session_state->allocatorThread->initContex();
            if (res != COMM_STATUS_SUCCESS)
                return MFX_ERR_DEVICE_FAILED;

            if (!IS_UNITE_MODE(app_state.GetCommContext(session))) {
                cmd.Write(session_state->allocatorThread->tx);
                cmd.Write(session_state->allocatorThread->rx);
            }
        }

        HDDLShimCommContext* ctx = app_state.GetCommContext(session);

        mfxStatus sts = MFX_ERR_DEVICE_FAILED;
        CommLock(ctx);
        if (app_state.bypass.Comm_Write(ctx, (int)command.size(), (void*)command.data()) == COMM_STATUS_SUCCESS) {
            if (needInit) {
                CommStatus comm_res = session_state->allocatorThread->connect();
                if (comm_res != COMM_STATUS_SUCCESS) {
                    Comm_Read(ctx, res);
                    CommUnlock(ctx);
                    return MFX_ERR_DEVICE_FAILED;
                }
            }
            sts = Comm_Read(ctx, res);
        } else {
            sts = MFX_ERR_DEVICE_FAILED;
        }

        CommUnlock(ctx);
#endif
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_QueryPlatform(mfxSession session, mfxPlatform* platform)
{
    if (!session || !platform)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXVideoCORE_QueryPlatform);
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader   result(res);
        if (sts == MFX_ERR_NONE)
            result.Read(*platform);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_SetHandle(mfxSession session, mfxHandleType type, mfxHDL hdl)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    try
    {
#ifndef MFX_SHIM_WRAPPER
        if (hdl)
        {
            VADriverContextP ctx = ((VADisplayContextP)hdl)->pDriverContext;
            HDDLVAShimDriverContext* context = (HDDLVAShimDriverContext*)ctx->pDriverData;
            if (!context || !context->mainCommCtx)
                return MFX_ERR_DEVICE_FAILED;

            auto* session_state = app_state.GetSessionState(session);
            if (!session_state)
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            //disconnect prev
            vaTerminate(session_state->display);
            session_state->comm_ctx = context->mainCommCtx;
            session_state->display = (VADisplay)hdl;
            session_state->need_delete_context = false;
        }
#endif

        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXVideoCORE_SetHandle);
        cmd.Write(session);
        cmd.Write(type);
        cmd.Write(hdl);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_GetHandle(mfxSession session, mfxHandleType type, mfxHDL *hdl)
{
    if (!session || !hdl)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXVideoCORE_GetHandle);
        cmd.Write(session);
        cmd.Write(type);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader   result(res);
        if (sts == MFX_ERR_NONE)
            result.Read(*hdl);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXJoinSession(mfxSession session, mfxSession child_session)
{
    if (!session || !child_session)
        return MFX_ERR_NULL_PTR;

    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXJoinSession);
        cmd.Write(session);
        cmd.Write(child_session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXCloneSession(mfxSession session, mfxSession* child_session)
{
    if (!session || !child_session)
        return MFX_ERR_NULL_PTR;

    try
    {
        HostSessionState * state = app_state.GetSessionState(session);
        if (!state)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXCloneSession);
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader   result(res);
        if (sts == MFX_ERR_NONE)
        {
            result.Read(*child_session);
            app_state.AddSession(*child_session, new (std::nothrow) HostSessionState(*child_session, state->GetImplementation()));
        }
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXDisjoinSession(mfxSession session)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXDisjoinSession);
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXSetPriority(mfxSession session, mfxPriority priority)
{
    if (!session)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXSetPriority);
        cmd.Write(session);
        cmd.Write(priority);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}

mfxStatus MFXGetPriority(mfxSession session, mfxPriority *priority)
{
    if (!session || !priority)
        return MFX_ERR_NULL_PTR;
    try
    {
        mfxDataPacket   command, res;
        PayloadWriter   cmd(command, eMFXGetPriority);
        cmd.Write(session);
        mfxStatus sts = Comm_Execute(app_state.GetCommContext(session), command, res);
        PayloadReader   result(res);
        if (sts == MFX_ERR_NONE)
            result.Read(*priority);
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}


inline  mfxFrameAllocator*   get_session_allocator(PayloadReader& args)
{
    mfxSession session = 0;
    args.Read(session);
    auto *session_state = app_state.GetSessionState(session);
    if (!session_state)
        return nullptr;
    return session_state->GetAllocator();
}

static  mfxStatus host_alloc_cb(HDDLShimCommContext* ctx, PayloadReader& args, PayloadWriter& result)
{
    mfxFrameAllocator* allocator = get_session_allocator(args);
    if (!allocator)
        return MFX_ERR_UNKNOWN;
    mfxFrameAllocRequest request;
    mfxFrameAllocResponse response = {};
    args.Read(request);

    SHIM_NORMAL_MESSAGE("Redirected allocator alloc()");
    mfxStatus sts = allocator->Alloc(allocator->pthis, &request, &response);
    if (sts == MFX_ERR_NONE) {
        result.Write(response);
        result.Write(response.mids, response.NumFrameActual*sizeof(mfxMemId));
    }
    return sts;
}

static std::map<mfxMemId, mfxFrameData*> lock_data;

static  mfxStatus host_lock_cb(HDDLShimCommContext* ctx, PayloadReader& args, PayloadWriter& result)
{
    mfxFrameAllocator* allocator = get_session_allocator(args);
    if (!allocator)
        return MFX_ERR_UNKNOWN;
    mfxMemId mid;
    args.Read(mid);

    lock_data[mid] = new mfxFrameData();
    mfxFrameData* data = lock_data[mid];
    args.Read(*data);

    mfxFrameInfo info;
    args.Read(info);

    SHIM_NORMAL_MESSAGE("Redirected allocator lock()");
    mfxStatus sts = allocator->Lock(allocator->pthis, mid, data);
    if (sts == MFX_ERR_NONE) {
        result.Write(*data);
        sts = WriteSurfaceData(result, info, *data);
    }
    return sts;
}

static  mfxStatus host_unlock_cb(HDDLShimCommContext* ctx, PayloadReader& args, PayloadWriter& result)
{
    mfxFrameAllocator* allocator = get_session_allocator(args);
    if (!allocator)
        return MFX_ERR_UNKNOWN;
    mfxMemId mid;
    args.Read(mid);

    mfxFrameData* data = lock_data[mid];

    mfxFrameInfo info;
    args.Read(info);

    mfxStatus sts = ReadSurfaceData(args, info, *data);
    if (sts != MFX_ERR_NONE)
        return sts;

    SHIM_NORMAL_MESSAGE("Redirected allocator unlock()");
    sts = allocator->Unlock(allocator->pthis, mid, data);
    return sts;
}

static  mfxStatus host_get_hdl_cb(HDDLShimCommContext* ctx, PayloadReader& args, PayloadWriter& result)
{
    mfxFrameAllocator* allocator = get_session_allocator(args);
    if (!allocator)
        return MFX_ERR_UNKNOWN;
    mfxMemId mid;
    mfxHDL  hdl;
    args.Read(mid);

    SHIM_NORMAL_MESSAGE("Redirected allocator get_hdl()");
    mfxStatus sts = allocator->GetHDL(allocator->pthis, mid, &hdl);
    if (sts == MFX_ERR_NONE)
        result.Write(*(VASurfaceID *)hdl);
    return sts;
}

static  mfxStatus host_free_cb(HDDLShimCommContext* ctx, PayloadReader& args, PayloadWriter& result)
{
    mfxFrameAllocator* allocator = get_session_allocator(args);
    if (!allocator)
        return MFX_ERR_UNKNOWN;

    mfxFrameAllocResponse response;
    args.Read(response);
    response.mids = new mfxMemId[response.NumFrameActual];
    args.Read(response.mids, response.NumFrameActual * sizeof(mfxMemId));

    SHIM_NORMAL_MESSAGE("Redirected allocator free()");
    mfxStatus sts = allocator->Free(allocator->pthis, &response);
    return sts;
}

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
return_value MFX_CDECL func_name formal_param_list \
{ \
    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED; \
    return mfxRes; \
}

//
// API version 1.0 functions
//

// API version where a function is added. Minor value should precedes the major value
#define API_VERSION {{0, 1}}

FUNCTION(mfxStatus, MFXInit, (mfxIMPL impl, mfxVersion *pVer, mfxSession *session), (impl, pVar, session))

// VPP interface functions
FUNCTION(mfxStatus, MFXVideoVPP_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoVPP_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoVPP_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoVPP_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoVPP_Close, (mfxSession session), (session))

FUNCTION(mfxStatus, MFXVideoVPP_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoVPP_GetVPPStat, (mfxSession session, mfxVPPStat *stat), (session, stat))
FUNCTION(mfxStatus, MFXVideoVPP_RunFrameVPPAsync, (mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp), (session, in, out, aux, syncp))

#undef API_VERSION


#define API_VERSION {{19, 1}}

static mfxTargetFunction cb_handlers[eMFXLastCB - eMFXFirstCB] = {
    host_alloc_cb,  //eMFXAllocCB,
    host_lock_cb,   //eMFXLockCB,
    host_unlock_cb, //eMFXUnlockCB,
    host_get_hdl_cb,//eMFXGetHDLCB,
    host_free_cb,   //eMFXFreeCB,
};

mfxTargetFunction GetHostCB(size_t id)
{
    id -= eMFXFirstCB;
    if (id < eMFXLastCB)
        return cb_handlers[id];
    return nullptr;
}


// Execute callback
mfxStatus ExecuteCB(HDDLShimCommContext *ctx, mfxCBId cb_id, const mfxDataPacket& command, mfxDataPacket& result)
{
    PayloadReader in(command);
    PayloadWriter out(result);
    mfxStatus sts = MFX_ERR_UNKNOWN;
    mfxTargetFunction cb = GetHostCB(cb_id);
    try
    {
        if (cb)
        {
            sts = cb(ctx, in, out);
            SHIM_NORMAL_MESSAGE("cb status: %d\n", (int)sts);
            sts = SetCBResult(sts);
            out.SetStatus(sts);
        }
        else
        {
            SHIM_ERROR_MESSAGE("GetCBHandler() returned NULL");
        }
        return sts;
    }
    catch (RuntimeError& err)
    {
        return mfxStatus(err.GetCode());
    }
    return MFX_ERR_UNKNOWN;
}


#if !defined(MFX_ONEVPL)
#include "mfxplugin.h"
#include "mfxpak.h"

extern "C" {
    FUNCTION(mfxStatus, MFXVideoCORE_SetBufferAllocator, (mfxSession session, mfxBufferAllocator* allocator), (session, allocator))
    FUNCTION(mfxStatus, MFXVideoENC_GetVideoParam, (mfxSession session, mfxVideoParam* par), (session, par))
    FUNCTION(mfxStatus, MFXVideoPAK_GetVideoParam, (mfxSession session, mfxVideoParam* par), (session, par))
    FUNCTION(mfxStatus, MFXVideoUSER_GetPlugin, (mfxSession session, mfxU32 type, mfxPlugin* par), (session, type, par))
    FUNCTION(mfxStatus, MFXDoWork, (mfxSession session), (session))
    FUNCTION(mfxStatus, MFXVideoUSER_Load, (mfxSession session, const mfxPluginUID *uid, mfxU32 version), (session, uid, version))
    FUNCTION(mfxStatus, MFXVideoUSER_UnLoad, (mfxSession session, const mfxPluginUID *uid), (session, uid))

    FUNCTION(mfxStatus, MFXVideoENC_Query, (mfxSession session, mfxVideoParam* in, mfxVideoParam* out), (session, in, out))
    FUNCTION(mfxStatus, MFXVideoENC_QueryIOSurf, (mfxSession session, mfxVideoParam* par, mfxFrameAllocRequest* request), (session, par, request))
    FUNCTION(mfxStatus, MFXVideoENC_Init, (mfxSession session, mfxVideoParam* par), (session, par))
    FUNCTION(mfxStatus, MFXVideoENC_Reset, (mfxSession session, mfxVideoParam* par), (session, par))
    FUNCTION(mfxStatus, MFXVideoENC_Close, (mfxSession session), (session))
    FUNCTION(mfxStatus, MFXVideoENC_ProcessFrameAsync, (mfxSession session, mfxENCInput* in, mfxENCOutput* out, mfxSyncPoint* syncp), (session, in, out, syncp))

    FUNCTION(mfxStatus, MFXVideoVPP_RunFrameVPPAsyncEx, (mfxSession session, mfxFrameSurface1* in, mfxFrameSurface1* work, mfxFrameSurface1** out, mfxSyncPoint* syncp), (session, in, work, out, syncp))

    FUNCTION(mfxStatus, MFXVideoPAK_Query, (mfxSession session, mfxVideoParam* in, mfxVideoParam* out), (session, in, out))
    FUNCTION(mfxStatus, MFXVideoPAK_QueryIOSurf, (mfxSession session, mfxVideoParam* par, mfxFrameAllocRequest* request), (session, par, request))
    FUNCTION(mfxStatus, MFXVideoPAK_Init, (mfxSession session, mfxVideoParam* par), (session, par))
    FUNCTION(mfxStatus, MFXVideoPAK_Reset, (mfxSession session, mfxVideoParam* par), (session, par))
    FUNCTION(mfxStatus, MFXVideoPAK_Close, (mfxSession session), (session))
    FUNCTION(mfxStatus, MFXVideoPAK_ProcessFrameAsync, (mfxSession session, mfxPAKInput* in, mfxPAKOutput* out, mfxSyncPoint* syncp), (session, in, out, syncp))
    FUNCTION(mfxStatus, MFXVideoUSER_Register, (mfxSession session, mfxU32 type, const mfxPlugin* par), (session, type, par))
    FUNCTION(mfxStatus, MFXVideoUSER_Unregister, (mfxSession session, mfxU32 type), (session, type))
    FUNCTION(mfxStatus, MFXVideoUSER_ProcessFrameAsync, (mfxSession session, const mfxHDL* in, mfxU32 in_num, const mfxHDL* out, mfxU32 out_num, mfxSyncPoint* syncp), (session, in, in_num, out, out_num, syncp))

}
#endif // MFX_ONEVPL

CommStatus CommWriteBypass(HDDLShimCommContext* ctx, int size, void* payload) {
    return app_state.bypass.Comm_Write(ctx, size, payload);
}

CommStatus CommReadBypass(HDDLShimCommContext* ctx, int size, void* payload) {
    return app_state.bypass.Comm_Read(ctx, size, payload);
}

CommStatus CommPeekBypass(HDDLShimCommContext* ctx, uint32_t* size, void* payload) {
    return app_state.bypass.Comm_Peek(ctx, size, payload);
}

#if (defined(_WIN32) || defined(_WIN64))
SimpleLoader::SimpleLoader(const char* name)
{
    so_handle = LoadLibraryA(name); //, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    if (NULL == so_handle)
    {
        throw std::runtime_error("Can't load library");
    }
}

void* SimpleLoader::GetFunction(const char* name)
{
    void* fn_ptr = GetProcAddress((HMODULE)so_handle, name);
    if (!fn_ptr)
        throw std::runtime_error("Can't find function");
    return fn_ptr;
}

SimpleLoader::~SimpleLoader()
{
    if (so_handle)
        FreeLibrary((HMODULE)so_handle);
}

#else 
SimpleLoader::SimpleLoader(const char* name)
{
    dlerror();
    so_handle = dlopen(name, RTLD_GLOBAL | RTLD_NOW);
    if (NULL == so_handle)
    {
        throw std::runtime_error("Can't load library");
    }
}

void* SimpleLoader::GetFunction(const char* name)
{
    void* fn_ptr = dlsym(so_handle, name);
    if (!fn_ptr)
        throw std::runtime_error("Can't find function");
    return fn_ptr;
}

SimpleLoader::~SimpleLoader()
{
    dlclose(so_handle);
}
#endif

#define SIMPLE_LOADER_STRINGIFY1( x) #x
#define SIMPLE_LOADER_STRINGIFY(x) SIMPLE_LOADER_STRINGIFY1(x)
#define SIMPLE_LOADER_DECORATOR1(fun,suffix) fun ## _ ## suffix
#define SIMPLE_LOADER_DECORATOR(fun,suffix) SIMPLE_LOADER_DECORATOR1(fun,suffix)

// Following macro applied on vaInitialize will give:  vaInitialize((vaInitialize_type)lib.GetFunction("vaInitialize"))
#define SIMPLE_LOADER_FUNCTION(name) name( (SIMPLE_LOADER_DECORATOR(name, type)) lib.GetFunction(SIMPLE_LOADER_STRINGIFY(name)) )

Bypass_Proxy::Bypass_Proxy()
#if defined(_WIN32) || defined(_WIN64)
    : lib("libva-kmb.dll")
#else
    : lib("hddl_bypass_drv_video.so")
#endif
    , SIMPLE_LOADER_FUNCTION(Comm_Write)
    , SIMPLE_LOADER_FUNCTION(Comm_Read)
    , SIMPLE_LOADER_FUNCTION(Comm_Peek)
{
}

Bypass_Proxy::~Bypass_Proxy()
{}

//EOF

