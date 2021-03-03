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

#include "hevcehw_kmb_roi_lin.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::KMB;

void Linux::KMB::ROI::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_SetCallChains
        , [this](StorageRW& global, StorageRW& /*local*/) -> mfxStatus
    {
        auto& vaPacker = Base::VAPacker::CC::Get(global);

        vaPacker.AddPerPicMiscData[VAEncMiscParameterTypeROI].Push([this](
            Base::VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& global
            , const StorageR& s_task
            , std::list<std::vector<mfxU8>>& data)
        {
            auto roi = Base::GetRTExtBuffer<mfxExtEncoderROI>(global, s_task);
            bool bNeedROI = !!roi.NumROI;

            if (bNeedROI)
            {
                auto& caps = Base::Glob::EncodeCaps::Get(global);
                auto& par = Base::Glob::VideoParam::Get(global);
                CheckAndFixROI(caps, par, roi);
                bNeedROI = !!roi.NumROI;
            }

            if (bNeedROI)
            {
                struct vsi_roi_type
                {
                    HANTROEncMiscParameterBufferROI     roi;
                    HANTROEncROI                        rec[MAX_NUM_ROI];
                };

                auto& vsi_roi = Base::AddVaMisc<vsi_roi_type>((VAEncMiscParameterType)HANTROEncMiscParameterTypeROI, data);

                vsi_roi.roi.min_delta_qp = -31;
                vsi_roi.roi.max_delta_qp = 31;
                vsi_roi.roi.num_roi = roi.NumROI;
                vsi_roi.roi.roi = vsi_roi.rec;

                auto   MakeVAEncROI = [&roi](const RectData& extRect)
                {
                    HANTROEncROI rect = {};
                    rect.roi_rectangle.x = static_cast<int16_t>(extRect.Left);
                    rect.roi_rectangle.y = static_cast<int16_t>(extRect.Top);
                    rect.roi_rectangle.width = static_cast<uint16_t>(extRect.Right - extRect.Left);
                    rect.roi_rectangle.height = static_cast<uint16_t>(extRect.Bottom - extRect.Top);
                    rect.roi_value = int8_t(extRect.DeltaQP);
                    rect.type = static_cast<int8_t>(roi.ROIMode);
                    return rect;
                };

                std::transform(roi.ROI, roi.ROI + roi.NumROI, vsi_roi.rec, MakeVAEncROI);
            }

            return bNeedROI;
        });


        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
