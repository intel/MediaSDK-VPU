// Copyright (c) 2020 Intel Corporation
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


#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_VA_LINUX) && defined(MFX_HW_KMB)

#include "hevcehw_kmb_lin.h"
#include "hevcehw_kmb_pp_lin.h"
#include "hevcehw_kmb_hdr_ro_lin.h"
#include "hevcehw_kmb_roi_map_lin.h"
#include "hevcehw_kmb_ipcm_lin.h"

#include "hevcehw_base_iddi.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base.h"
#include "hevcehw_base_legacy.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW::Base;

namespace HEVCEHW
{
namespace Linux
{
namespace KMB
{

MFXVideoENCODEH265_HW::MFXVideoENCODEH265_HW(
    VideoCORE& core
    , mfxStatus& status
    , eFeatureMode mode)
   : TBaseImpl(core, status, mode)
{
     status = MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEH265_HW::Init(mfxVideoParam *par)
{
    auto sts = TBaseImpl::Init(par);
    MFX_CHECK_STS(sts);

    auto& queue = BQ<BQ_SubmitTask>::Get(*this); 
    Reorder(queue, { FEATURE_DDI, IDDI::BLK_SubmitTask }, { HEVCEHW::KMB::FEATURE_VSI_PREPROCESSOR, HEVCEHW::Linux::KMB::VSIPreProcessor::BLK_PatchDDITask });
    Reorder(queue, { FEATURE_DDI, IDDI::BLK_SubmitTask }, { HEVCEHW::KMB::FEATURE_HDR_RESEND, HEVCEHW::Linux::KMB::VSIHdrResend::BLK_PatchDDITask });
    Reorder(queue, { FEATURE_DDI, IDDI::BLK_SubmitTask }, { HEVCEHW::KMB::FEATURE_ROI_MAP, HEVCEHW::Linux::KMB::VSIROIMap::BLK_PatchDDITask });
    Reorder(queue, { FEATURE_DDI, IDDI::BLK_SubmitTask }, { HEVCEHW::KMB::FEATURE_IPCM, HEVCEHW::Linux::KMB::IPCM::BLK_PatchDDITask });
    return MFX_ERR_NONE;
}

#if defined(MFX_HW_KMB)
inline void SetRCMinMaxQP(void* pData, mfxU32 MinQP, mfxU32 MaxQP)
{
    auto rc = reinterpret_cast<VAEncMiscParameterRateControl*>(pData);
    rc->min_qp = MinQP;
    rc->max_qp = MaxQP;
    if (rc->initial_qp > rc->max_qp)
        rc->initial_qp = rc->max_qp;
    if (rc->initial_qp < rc->min_qp)
        rc->initial_qp = rc->min_qp;
}
#endif

VABufferID KMB_DDI_VA::CreateVABuffer(
    const DDIExecParam& ep
    , TaskCommonPar* task)
{
    VABufferID   id = VA_INVALID_ID;
    VABufferType type = VABufferType(ep.Function);
    mfxStatus    sts;

    if (type == (VABufferType)HANTROEncROIMapBufferType)
    {
        auto roimap = (hantroCUMap*)ep.In.pData;

        mfxI32 blkSz = roimap->mbqp->BlockSize;

        uint32_t unit_size, stride;
        auto sts = vaCreateBuffer2(m_vaDisplay, m_vaContextEncode, (VABufferType)HANTROEncROIMapBufferType, roimap->width, roimap->height, &unit_size, &stride, &m_vaROIMapSurf);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);

        uint8_t* buf = 0;
        sts = vaMapBuffer(m_vaDisplay, m_vaROIMapSurf, (void **)&buf);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);

        const auto *qps = roimap->mbqp->QPmode;

        uint8_t *ptr = buf;
        mfxI32 subBlks = (blkSz * blkSz) / 64;

        if (roimap->mbqp->BlockSize == 64)
        {
            auto combine = [&ptr, subBlks](uint8_t qps, uint8_t modes)
            {
                combine_qp_mode(ptr, qps, modes, subBlks);
            };

            for (auto i = roimap->mbqp->NumQPAlloc; i > 0; --i, qps++)
                combine(qps->QP, (uint8_t)qps->Mode);
        }
        else
            roimap->mapper.MapBlocks(roimap->mbqp, ptr);

        sts = vaUnmapBuffer(m_vaDisplay, m_vaROIMapSurf);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);

        return m_vaROIMapSurf;
    }
    else if (type == VAEncMiscParameterBufferType && ((VAEncMiscParameterBuffer*)ep.In.pData)->type == (VABufferType)HANTROEncMiscParameterTypeROIMap)
    {
        auto roimap = (HANTROEncMiscParameterBufferROIMap *)((VAEncMiscParameterBuffer*)ep.In.pData)->data;
        roimap->roimap_buf_id = m_vaROIMapSurf;

        return TBase::CreateVABuffer(ep);
    }

    if (type != VAEncMiscParameterBufferType)
    {
        sts = Execute(
            VAFID_CreateBuffer
            , m_vaDisplay
            , m_vaContextEncode
            , type
            , ep.In.Size
            , std::max<mfxU32>(ep.In.Num, 1)
            , ep.In.pData
            , &id);
        MFX_CHECK(!sts, VA_INVALID_ID);

        m_vaBuffers.insert(id);

        return id;
    }

    sts = Execute(
        VAFID_CreateBuffer
        , m_vaDisplay
        , m_vaContextEncode
        , VAEncMiscParameterBufferType
        , ep.In.Size
        , 1
        , nullptr
        , &id);
    MFX_CHECK(!sts, VA_INVALID_ID);

    VAEncMiscParameterBuffer* pMP;

    sts = Execute(
        VAFID_MapBuffer
        , m_vaDisplay
        , id
        , (void**)&pMP);
    MFX_CHECK(!sts, VA_INVALID_ID);

    pMP->type = ((VAEncMiscParameterBuffer*)ep.In.pData)->type;
    mfxU8* pDst = (mfxU8*)pMP->data;
    mfxU8* pSrc = (mfxU8*)ep.In.pData + sizeof(VAEncMiscParameterBuffer);

    std::copy(pSrc, pSrc + ep.In.Size - sizeof(VAEncMiscParameterBuffer), pDst);

    if (pMP->type == VAEncMiscParameterTypeRateControl)
    {
        if (task)
            SetRCMinMaxQP(pDst, task->MinQP, task->MaxQP);
        else
            m_init_rc_id = id; // save id of the single RC buffer from PerSeq container
    }

    sts = Execute(
        VAFID_UnmapBuffer
        , m_vaDisplay
        , id);
    MFX_CHECK(!sts, VA_INVALID_ID);

    if (((VAEncMiscParameterBuffer*)ep.In.pData)->type == VAEncMiscParameterTypeRateControl && task)
    {
        // fill single RC buffer from PerSeq container with frame specific Min/Max QP
        sts = CallVA(m_callVa, VAFID_MapBuffer
            , m_vaDisplay
            , m_init_rc_id
            , (void **)&pMP);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);
        ThrowIf(pMP->type != VAEncMiscParameterTypeRateControl, MFX_ERR_UNKNOWN);

        SetRCMinMaxQP(pMP->data, task->MinQP, task->MaxQP);

        sts = CallVA(m_callVa, VAFID_UnmapBuffer
            , m_vaDisplay
            , m_init_rc_id);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);
    }

    m_vaBuffers.insert(id);

    return id;
}

}}} //namespace HEVCEHW::Linux::KMB

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
