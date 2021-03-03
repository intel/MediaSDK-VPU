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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_VA_LINUX) && defined(MFX_HW_KMB)

#include "hevcehw_kmb.h"
#include "hevcehw_base_lin.h"
#include "hevcehw_base_va_lin.h"
#include "hevcehw_kmb_roi_map_lin.h"

namespace HEVCEHW
{
namespace Linux
{
namespace KMB
{
    class KMB_DDI_VA : public ::HEVCEHW::Linux::Base::DDI_VA
    {
        using TBase = HEVCEHW::Linux::Base::DDI_VA;

        // using HEVCEHW::Base::IDDI::Func; doesn't block warning C4250
        // so below are defined private inline redirections to block the warnings
        inline void    SetTraceName(std::string&& name)         { HEVCEHW::Base::IDDI::SetTraceName(std::move(name)); }
        inline const   BlockTracer::TFeatureTrace*  GetTrace()  { return HEVCEHW::Base::IDDI::GetTrace(); }

        inline void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) { TBase::Query1NoCaps(blocks, Push); }
        inline void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) { TBase::Query1WithCaps(blocks, Push); }
        inline void InitExternal(const FeatureBlocks& blocks, TPushIE Push) { TBase::InitExternal(blocks, Push); }
        inline void InitAlloc(const FeatureBlocks& blocks, TPushIA Push)    { TBase::InitAlloc(blocks, Push); }
        inline void SubmitTask(const FeatureBlocks& blocks, TPushST Push)   { TBase::SubmitTask(blocks, Push); }
        inline void QueryTask(const FeatureBlocks& blocks, TPushQT Push)    { TBase::QueryTask(blocks, Push); }
        inline void ResetState(const FeatureBlocks& blocks, TPushRS Push)   { TBase::ResetState(blocks, Push); }

    public:
        enum {
            VAFID_CreateBuffer2 = VAFID_SyncSurface + 1
        };

    public:
        KMB_DDI_VA(mfxU32 FeatureId)
            : DDI_VA(FeatureId)
        {
            SetTraceName("KMB_DDI_VA");
            
            m_ddiExec[(TBase::VAFID)VAFID_CreateBuffer2] = CallDefault(&vaCreateBuffer2);
        
        }

    protected:
        VABufferID  m_vaROIMapSurf = VA_INVALID_ID;
        VABufferID m_init_rc_id = VA_INVALID_ID;

 
        virtual VABufferID CreateVABuffer(
            const DDIExecParam& ep
            , TaskCommonPar* task = nullptr);

    };


    class MFXVideoENCODEH265_HW
        : public HEVCEHW::KMB::MFXVideoENCODEH265_HW
        , virtual protected FeatureBlocks
    {
    public:
        using TBaseImpl = HEVCEHW::KMB::MFXVideoENCODEH265_HW;

        MFXVideoENCODEH265_HW(
            VideoCORE& core
            , mfxStatus& status
            , eFeatureMode mode = eFeatureMode::INIT);

        virtual mfxStatus Init(mfxVideoParam *par) override;
    };
} //KMB
} //Linux
}// namespace HEVCEHW

#endif
