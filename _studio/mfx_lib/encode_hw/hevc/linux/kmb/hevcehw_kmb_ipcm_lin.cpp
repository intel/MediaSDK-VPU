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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_HW_KMB) && defined(MFX_VA_LINUX)

#include "hevcehw_kmb_ipcm.h"
#include "hevcehw_kmb_ipcm_lin.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::KMB;
using namespace HEVCEHW::Base;
using namespace HEVCEHW::Linux::Base;

HEVCEHW::Linux::KMB::IPCM::IPCM(mfxU32 FeatureId)
    : HEVCEHW::KMB::IPCM(FeatureId)
{
}

HEVCEHW::Linux::KMB::IPCM::~IPCM()
{
}

void HEVCEHW::Linux::KMB::IPCM::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_PatchDDITask
        , [this](StorageW& global, StorageW& /*s_task*/) -> mfxStatus
    {
        
        const uint32_t rect_count = static_cast<uint32_t>(m_vaIPCM.size());
        const uint32_t buf_size = sizeof(VAEncMiscParameterBuffer) + sizeof(HANTROEncMiscParameterBufferIPCM) + rect_count * sizeof(HANTRORectangle);
        if (rect_count)
        {
            m_va_buf.resize(buf_size, 0);
            VAEncMiscParameterBuffer *misc_param = (VAEncMiscParameterBuffer*)&m_va_buf[0];
            misc_param->type = (VAEncMiscParameterType)HANTROEncMiscParameterTypeIPCM;
            auto ipcm_param = (HANTROEncMiscParameterBufferIPCM *)misc_param->data;
            ipcm_param->num_ipcm = rect_count;
            if (ipcm_param->num_ipcm)
            {
                ipcm_param->ipcm = (HANTRORectangle*)((mfxU8*)misc_param + sizeof(VAEncMiscParameterBuffer) + sizeof(HANTROEncMiscParameterBufferIPCM));
                memcpy(ipcm_param->ipcm, m_vaIPCM.data(), ipcm_param->num_ipcm * sizeof(HANTRORectangle));
            }

            DDIExecParam xPar;
            xPar.Function = VAEncMiscParameterBufferType;
            xPar.In.pData = &m_va_buf[0];
            xPar.In.Size = buf_size;
            xPar.In.Num = 1;

            Glob::DDI_SubmitParam::Get(global).push_back(xPar);
        }
        return MFX_ERR_NONE;
    });
}

// Need to reserve space for IPCM rectangles in video memory buffer
struct HANTROEncMiscParameterBufferIPCMExt : HANTROEncMiscParameterBufferIPCM
{
    HANTRORectangle ipcm_rect[HEVCEHW::KMB::IPCM::MAX_NUM_IPCM];
};


void Linux::KMB::IPCM::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_SetCallChains
        , [this](StorageRW& global, StorageRW& /*local*/) -> mfxStatus
    {
        auto& vaPacker = VAPacker::CC::Get(global);

        vaPacker.AddPerPicMiscData[(VAEncMiscParameterType)HANTROEncMiscParameterTypeIPCM].Push([this](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& global
            , const StorageR& s_task
            , std::list<std::vector<mfxU8>>& data)
        {
            auto ipcm = GetRTExtBuffer<mfxExtEncoderIPCMArea>(global, s_task);
            bool bNeedIPCM = !!ipcm.NumArea;

            if (bNeedIPCM)
            {
                auto& caps = Glob::EncodeCaps::Get(global);
                auto& par = Glob::VideoParam::Get(global);
                CheckAndFixIPCM(caps, par, ipcm);
                bNeedIPCM = !!ipcm.NumArea;
            }

            if (bNeedIPCM)
            {
                auto& vaIPCM = AddVaMisc<HANTROEncMiscParameterBufferIPCMExt>((VAEncMiscParameterType)HANTROEncMiscParameterTypeIPCM, data);

                auto   MakeVAEncIPCM = [](const RectData& rect) -> HANTRORectangle
                {
                    HANTRORectangle ipcm_rect = {};
                    ipcm_rect.x = static_cast<int16_t>(rect.Left);
                    ipcm_rect.y = static_cast<int16_t>(rect.Top);
                    ipcm_rect.width = static_cast<uint16_t>(rect.Right - rect.Left);
                    ipcm_rect.height = static_cast<uint16_t>(rect.Bottom - rect.Top);
                    return ipcm_rect;
                };

                m_vaIPCM.resize(ipcm.NumArea);

                vaIPCM.num_ipcm = ipcm.NumArea;
                vaIPCM.ipcm     = m_vaIPCM.data();

                std::transform(ipcm.Area, ipcm.Area + ipcm.NumArea, vaIPCM.ipcm, MakeVAEncIPCM);
            }

            return bNeedIPCM;
        });
            
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)

