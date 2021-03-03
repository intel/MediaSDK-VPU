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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) 

#include "hevcehw_kmb_hdr_ro_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void HEVCEHW::Linux::KMB::VSIHdrResend::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_PatchDDITask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        
        auto& task = Task::Common::Get(s_task);

        memset(&m_vsi_ro, 0, sizeof(m_vsi_ro));
        m_vsi_ro.type = (VAEncMiscParameterType)HANTROConfigAttribEncPackedHeaderOutputControl;

        if (task.InsertHeaders)
        {
            m_vsi_ro.ro.phoc_flags.bits.vps_reoutput_enable = !!(task.InsertHeaders & INSERT_VPS);
            m_vsi_ro.ro.phoc_flags.bits.sps_reoutput_enable = !!(task.InsertHeaders & INSERT_SPS);
            m_vsi_ro.ro.phoc_flags.bits.pps_reoutput_enable = !!(task.InsertHeaders & INSERT_PPS);
            m_vsi_ro.ro.phoc_flags.bits.aud_output_enable = !!(task.InsertHeaders & INSERT_AUD);
            m_vsi_ro.ro.phoc_flags.bits.sei_output_enable = !!(task.InsertHeaders & INSERT_SEI);
        }

        DDIExecParam xPar;
        xPar.Function = VAEncMiscParameterBufferType;
        xPar.In.pData = &m_vsi_ro;
        xPar.In.Size = sizeof(m_vsi_ro);
        xPar.In.Num = 1;

        Glob::DDI_SubmitParam::Get(global).push_back(xPar);

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
