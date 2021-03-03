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

#include "hevcehw_kmb_pp_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void HEVCEHW::Linux::KMB::VSIPreProcessor::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_PatchDDITask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& video_param = Glob::VideoParam::Get(global);
        auto& task = Task::Common::Get(s_task);
        mfxFrameInfo* info = &video_param.mfx.FrameInfo;
        if (task.pSurfReal)
            info = &task.pSurfReal->Info;

        bool need_grey = video_param.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV400;
        bool need_crop = (info->CropX || info->CropY || info->CropW != info->Width || info->CropH != info->Height);
        if (need_grey || need_crop)
        {
            memset(&m_vsi_pp, 0, sizeof(m_vsi_pp));
            m_vsi_pp.type = (VAEncMiscParameterType)HANTROEncMiscParameterTypeEmbeddedPreprocess;
            if (need_crop)
            {
                m_vsi_pp.pp.cropping_offset_x = info->CropX;
                m_vsi_pp.pp.cropping_offset_y = info->CropY;
                m_vsi_pp.pp.cropped_width = info->CropW;
                m_vsi_pp.pp.cropped_height = info->CropH;
            }
            if (need_grey)
            {
                m_vsi_pp.pp.preprocess_flags.bits.embedded_preprocess_constant_chroma_is_enabled = 1;
                m_vsi_pp.pp.constCr = m_vsi_pp.pp.constCb =
                        video_param.mfx.FrameInfo.BitDepthLuma == 10 ? 512 : 128;
            }
            DDIExecParam xPar;
            xPar.Function = VAEncMiscParameterBufferType;
            xPar.In.pData = &m_vsi_pp;
            xPar.In.Size = sizeof(m_vsi_pp);
            xPar.In.Num = 1;

            Glob::DDI_SubmitParam::Get(global).push_back(xPar);
        }
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
