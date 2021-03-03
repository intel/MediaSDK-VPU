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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_kmb_caps.h"
#include "hevcehw_base_data.h"

using namespace HEVCEHW;
using namespace HEVCEHW::KMB;

void Caps::Query1NoCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
  /*  Push(BLK_SetDefaultsCallChain,
        [this](const mfxVideoParam&, mfxVideoParam&, StorageRW& strg) -> mfxStatus
    {
        using namespace Base;
        auto& defaults = Glob::Defaults::GetOrConstruct(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        MFX_CHECK(!bSet, MFX_ERR_NONE);

        defaults.GetMaxNumRef.Push([](
            Defaults::TChain<std::tuple<mfxU16, mfxU16>>::TExt
            , const Base::Defaults::Param& dpar)
        {
            //limited VDEnc support without HME and StreamIn 3rd reference
            static const mfxU16 nRefs[7] = { 2, 2, 2, 2, 2, 1, 1 };

            mfxU16 tu = dpar.mvp.mfx.TargetUsage;
            CheckRangeOrSetDefault<mfxU16>(tu, 1, 7, 4);
            --tu;

            return std::make_tuple(
                std::min<mfxU16>(nRefs[tu], dpar.caps.MaxNum_Reference0)
                , std::min<mfxU16>(nRefs[tu], dpar.caps.MaxNum_Reference1));
        });

        defaults.GetLowPower.Push([](
            Defaults::TGetHWDefault<mfxU16>::TExt
            , const mfxVideoParam& par
            , eMFXHWType /*hw* /)
        {
            bool bValid =
                par.mfx.LowPower
                && CheckTriState(par.mfx.LowPower);

            return mfxU16(
                !bValid * MFX_CODINGOPTION_ON
                + bValid * par.mfx.LowPower);
        });

        bSet = true;

        return MFX_ERR_NONE;
    });*/
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)