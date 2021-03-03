// Copyright (c) 2018-2020 Intel Corporation
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

#include "umc_defs.h"

#include "mfx_umc_alloc_wrapper.h"
#include "mfx_common.h"
#include "libmfx_core.h"
#include "mfx_common_int.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter_SW::StartPreparingToOutput(mfxFrameSurface1 *,
                                                                       UMC::FrameData* ,
                                                                       const mfxVideoParam *,
                                                                       mfxU16 *,
                                                                       bool )
{
    return MFX_ERR_NONE;
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter_SW::CheckPreparingToOutput(mfxFrameSurface1 *surface_work,
                                                                       UMC::FrameData* in,
                                                                       const mfxVideoParam * par,
                                                                       mfxU16 )
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (!surface_work || surface_work->Info.FourCC != MFX_FOURCC_NV12)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if(par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    else
    {
        UMC::FrameMemID indexTop = in[0].GetFrameMID();
        UMC::FrameMemID indexBottom = in[1].GetFrameMID();

        mfxFrameSurface1 srcSurface[2];

        srcSurface[0] = m_frameDataInternal.GetSurface(indexTop);
        srcSurface[1] = m_frameDataInternal.GetSurface(indexBottom);

        for (int i = 0; i < 1 + (surface_work->Info.PicStruct != MFX_PICSTRUCT_PROGRESSIVE); ++i)
        {
            srcSurface[i].Info.CropW = surface_work->Info.CropW;
            srcSurface[i].Info.CropH = surface_work->Info.CropH;
            if (surface_work->Info.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
            {
                srcSurface[i].Info.CropH /= 2;
            }
        }

        //Performance issue. We need to unlock mutex to let decoding thread run async.
        guard.Unlock();

        mfxFrameSurface1 dstTempSurface;
        memset(&dstTempSurface, 0, sizeof(mfxFrameSurface1));
        dstTempSurface.Data = surface_work->Data;
        dstTempSurface.Info = surface_work->Info;
        dstTempSurface.Info.Height /= 2;
        dstTempSurface.Info.CropH /= 2;

        mfxU8* dstPtr = GetFramePointer(surface_work->Info.FourCC, dstTempSurface.Data);
        mfxStatus sts = MFX_ERR_NONE;

        if (!dstPtr) {
            sts = m_pCore->LockExternalFrame(surface_work->Data.MemId, &dstTempSurface.Data);
            MFX_CHECK_STS(sts);
        }

        dstTempSurface.Data.Pitch <<= 1;

        sts = m_pCore->DoFastCopyWrapper(&dstTempSurface,
            MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY,
            &srcSurface[0],
            MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET
        );

        dstTempSurface.Data.Y += (dstTempSurface.Data.Pitch / 2);
        dstTempSurface.Data.UV += (dstTempSurface.Data.Pitch / 2);

        sts = m_pCore->DoFastCopyWrapper(&dstTempSurface,
            MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY,
            &srcSurface[1],
            MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET
        );

        if (!dstPtr) {
            sts = m_pCore->UnlockExternalFrame(surface_work->Data.MemId, &dstTempSurface.Data);
            MFX_CHECK_STS(sts);
        }

        guard.Lock();
        if (sts < MFX_ERR_NONE)
            return sts;

        if (!m_IsUseExternalFrames)
        {
            m_pCore->DecreaseReference(&surface_work->Data);
            m_extSurfaces[indexTop].FrameSurface = 0;
        }
    }

    return MFX_ERR_NONE;
}

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
