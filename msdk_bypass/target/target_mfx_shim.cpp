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
//! \file    target_mfx_shim.cpp
//! \brief   Main program execution for KMB MFX proxy
//! \details Include basic functions required to retrieve/send payload & call corresponding
//!          Media SDK function.
//!

extern "C" {
    #include "payload.h"
}

#include "hddl_mfx_common.h"
#include "mfx_payload.h"
#include "target_va_shim.h"
#if !defined(WIN_HOST)
#include <sys/syscall.h>
#endif

extern "C" int ProcessMFXPayload(uint32_t functionID, HDDLShimCommContext* ctx, char* payload, uint32_t size)
{
    if (functionID < eMFXFirstFunc || functionID >= eMFXLastFunc)
        return -1;
    
    mfxDataPacket   args(payload, payload + size);

    // Call corresponding function to handle MFX Function by ID
    PayloadReader   in(args);
    mfxDataPacket   result;
    PayloadWriter   out(result);
    mfxStatus sts = MFX_ERR_UNSUPPORTED;
    
    mfxTargetFunction   handler = GetMFXHandler(functionID);
    if (!handler)
    {
        SHIM_ERROR_MESSAGE("GetMFXHandler() returned NULL");
        return -1;
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
        out.Clear();
        sts = MFX_ERR_UNKNOWN;
    }
    // Write back processed result
    SHIM_NORMAL_MESSAGE("status: %d\n", sts);
    out.SetStatus(sts);

    auto commStatus = Comm_Write(ctx, result.size(), result.data());
    if (commStatus != COMM_STATUS_SUCCESS)
    {
        SHIM_ERROR_MESSAGE("Error write socket");
        return -1;
    }

    return 0;
}

mfxStatus ExecuteVA(HDDLShimCommContext* ctx, HDDLVAFunctionID functionID, const mfxDataPacket& command, mfxDataPacket& result)
{
    // Call corresponding function to handle VAFunctionID
    void *vaDataRX = HDDLShim_MainPayloadExtraction(functionID, ctx, (void*)command.data(), command.size());

    if (!vaDataRX)
    {
        SHIM_ERROR_MESSAGE("ExecuteVA: vaDataRX returned NULL");
        return MFX_ERR_UNKNOWN;
    }

    size_t size = ((HDDLVAData*)vaDataRX)->size;
    result.resize(size);
    memcpy(result.data(), vaDataRX, size);
    HDDLMemoryMgr_FreeMemory(vaDataRX);
    return MFX_ERR_NONE;
}
