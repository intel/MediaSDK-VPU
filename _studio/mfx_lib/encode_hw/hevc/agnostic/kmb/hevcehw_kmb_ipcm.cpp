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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_HW_KMB)

#include "hevcehw_kmb_ipcm.h"
#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;
using namespace HEVCEHW::KMB;

IPCM::~IPCM()
{

}

mfxStatus IPCM::CheckAndFixIPCM(
    ENCODE_CAPS_HEVC const & /*caps*/
    , mfxVideoParam const & /*par*/
    , mfxExtEncoderIPCMArea& ipcm)
{
    mfxStatus sts = MFX_ERR_NONE, rsts = MFX_ERR_NONE;
    mfxU32 changed = 0, invalid = 0;

    auto IsInvalidRect = [](const IPCM::RectData& rect)
    {
        return ((rect.Left >= rect.Right) || (rect.Top >= rect.Bottom));
    };

    invalid += ipcm.NumArea > MAX_NUM_IPCM;
    invalid += !ipcm.Area;

    if (ipcm.Area)
    {
        mfxU16 numValidIPCM = mfxU16(std::remove_if(ipcm.Area, ipcm.Area + ipcm.NumArea, IsInvalidRect) - ipcm.Area);
        changed += numValidIPCM < ipcm.NumArea;
        if (!invalid)
            ipcm.NumArea = numValidIPCM;
    }

    sts = GetWorstSts(
        mfxStatus(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM * !!changed)
        , mfxStatus(MFX_ERR_UNSUPPORTED * !!invalid));

    return GetWorstSts(sts, rsts);
}

void IPCM::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_ENCODER_IPCM_AREA].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtEncoderIPCMArea*)pSrc;
        auto& dst = *(mfxExtEncoderIPCMArea*)pDst;

        dst.NumArea = src.NumArea;
        dst.Area = src.Area;
    });
}

void IPCM::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [this](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& global) -> mfxStatus
    {
        mfxExtEncoderIPCMArea* pIPCM = ExtBuffer::Get(par);
        MFX_CHECK(pIPCM && pIPCM->NumArea && pIPCM->Area, MFX_ERR_NONE);

        auto& caps = Glob::EncodeCaps::Get(global);
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
        bool changed = false;

        auto sts = CheckAndFixIPCM(caps, par, *pIPCM);
        MFX_CHECK(sts >= MFX_ERR_NONE && pCO3, sts);

        return GetWorstSts(sts, mfxStatus(changed * MFX_WRN_INCOMPATIBLE_VIDEO_PARAM));
    });
}

void IPCM::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaults
        , [this](mfxVideoParam& par, StorageW& strg, StorageRW&)
    {
        auto& defaults = Glob::Defaults::Get(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        if (!bSet)
        {
            defaults.GetPPS.Push([](
                Defaults::TGetPPS::TExt prev
                , const Defaults::Param& defPar
                , const Base::SPS& sps
                , Base::PPS& pps)
            {
                auto sts = prev(defPar, sps, pps);

                return sts;
            });

            bSet = true;
        }

		mfxExtEncoderIPCMArea* pIPCM = ExtBuffer::Get(par);
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3 && pIPCM && pIPCM->NumArea && pIPCM->Area, MFX_ERR_NONE);

        return MFX_ERR_NONE;
    });
}

void IPCM::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_SetPPS
        , [this](StorageRW& /*strg*/, StorageRW&) -> mfxStatus
    {
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)

