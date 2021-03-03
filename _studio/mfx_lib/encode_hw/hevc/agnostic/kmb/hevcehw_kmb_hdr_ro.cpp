// Copyright (c) 2019-2020 Intel Corporation
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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_HW_KMB)

#include "hevcehw_kmb_hdr_ro.h"
#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::KMB;

/* disable while no suport at init
void VSIHdrResend::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_SPSPPS_RESEND].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtInsertHeaders*)pSrc;
        auto& buf_dst = *(mfxExtInsertHeaders*)pDst;
        MFX_COPY_FIELD(SPS);
        MFX_COPY_FIELD(PPS);
    });
}
*/

void VSIHdrResend::SetInherited(ParamInheritance& par)
{
    par.m_ebInheritDefault[MFX_EXTBUFF_INSERT_HEADERS].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        auto& src = *(const mfxExtInsertHeaders*)pSrc;
        auto& dst = *(mfxExtInsertHeaders*)pDst;

        InheritOption(src.SPS, dst.SPS);
        InheritOption(src.PPS, dst.PPS);
    });
}

void VSIHdrResend::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& global) -> mfxStatus
    {
		mfxExtInsertHeaders* extInsertHeaders = ExtBuffer::Get(par);
        MFX_CHECK(extInsertHeaders, MFX_ERR_NONE);

        auto& caps = Glob::EncodeCaps::Get(global);

        mfxU32 changed = 0;

        if (CheckTriState(extInsertHeaders->SPS))
            changed = true;
        if (CheckTriState(extInsertHeaders->PPS))
            changed = true;

        if (extInsertHeaders->SPS && !caps.ResendSPS)
        {
			extInsertHeaders->SPS = MFX_CODINGOPTION_OFF;
            changed = true;
        }
        if (extInsertHeaders->PPS && !caps.ResendPPS)
        {
			extInsertHeaders->PPS = MFX_CODINGOPTION_OFF;
            changed = true;
        }

        return changed ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM : MFX_ERR_NONE;
    });
}

void VSIHdrResend::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaults
        , [](mfxVideoParam& par, StorageW& /*strg*/, StorageRW&)
    {
		mfxExtInsertHeaders* extInsertHeaders = ExtBuffer::Get(par);
        MFX_CHECK(extInsertHeaders, MFX_ERR_NONE);

        SetDefault<mfxU16>(extInsertHeaders->SPS, MFX_CODINGOPTION_OFF);
        SetDefault<mfxU16>(extInsertHeaders->PPS, MFX_CODINGOPTION_OFF);

        return MFX_ERR_NONE;
    });
}

void VSIHdrResend::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_ConfigureTask
        , [this](
            StorageW& 
            , StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        const mfxExtInsertHeaders*  ParSetUpdate = ExtBuffer::Get(task.ctrl);

        if (ParSetUpdate)
        {
            task.InsertHeaders |= (INSERT_VPS | INSERT_SPS) * IsOn(ParSetUpdate->SPS);
            task.InsertHeaders |= INSERT_PPS * IsOn(ParSetUpdate->PPS);
        }
        
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
