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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) 

#include "hevcehw_base.h"
#include "hevcehw_base_data.h"
#include "hevcehw_kmb_roi_map.h"

#include "va_hantro/va_hantro.h"

namespace HEVCEHW
{
namespace Linux
{
namespace KMB
{
    inline void combine_qp_mode(uint8_t*& ptr, uint8_t qps, uint8_t modes, uint32_t count)
    {
        uint8_t mode = modes == MFX_MBQP_MODE_QP_VALUE;
        uint8_t val = mode | (qps << 1);
        val &= 0x7f; //unset ipcm bit
        memset(ptr, val, count);
        ptr += count;
    };

    class ROIMapBlockMapper
    {
        mfxU32 m_ver_block_count = 0;
        mfxU32 m_hor_block_count = 0;
        mfxU32 m_rep = 0;
        mfxU32 m_block_count = 0;
    public:
        mfxStatus Init(mfxU32 width, mfxU32 height, mfxU32 block_size);
        mfxU32    GetBlockCount() { return m_block_count; }
        mfxStatus MapBlocks(mfxExtMBQP* qp, uint8_t* dst);
    };

    struct hantroCUMap
    {
        mfxI32 width = 0;
        mfxI32 height = 0;

        mfxExtMBQP *mbqp = nullptr;
        ROIMapBlockMapper mapper;
    };

    class VSIROIMap
        : public HEVCEHW::KMB::VSIROIMap
    {
    public:

        VSIROIMap(mfxU32 FeatureId)
            : HEVCEHW::KMB::VSIROIMap(FeatureId)
        {}

    protected:
        virtual void SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push) override;

        struct
        {
            VAEncMiscParameterType              type;
            HANTROEncMiscParameterBufferROIMap  rm;
        } m_vsi_rm = {};

        hantroCUMap m_roiMap;
    };

} //KMB
} //Linux
} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
