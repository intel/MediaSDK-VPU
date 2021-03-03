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

#include "hevcehw_base_va_lin.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base_iddi_packer.h"

#include "hevcehw_kmb_roi_map_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;
using namespace HEVCEHW::Linux;
using namespace HEVCEHW::Linux::Base;

namespace HEVCEHW {
namespace Linux {
namespace KMB {

#if (0) // Keep for debug
    static void PrintMBQP(mfxU8* mbqp, size_t hor_block_count, size_t ver_block_count, const char* file)
    {
        FILE* fi = fopen(file, "w");
        if (fi)
        {

            for (auto v = 0; v < ver_block_count; ++v)
            {
                for (auto h = 0; h < hor_block_count; ++h)
                    fprintf(fi, "0x%02x ", mbqp[v * hor_block_count + h]);
                fprintf(fi, "\n");
            }
            fclose(fi);
        }
    }
#endif

    static void RecalculateBlocks(mfxQPandMode* mbqp, mfxU32 hor_block_count, mfxU32 ver_block_count, mfxU32 rep,
        mfxU8* dst, mfxU32 extra_right = 0)
    {
        uint8_t* ptr = dst;
        auto    stride = hor_block_count;
        mfxU32  ce = 8 / rep;

        for (mfxU32 v = 0; v < ver_block_count; v += ce)
        {
            for (mfxU32 h = 0; h < hor_block_count; h += ce)
            {
                auto sv = v;
                for (auto vc = 0; vc < 8; )
                {
                    auto start = v * stride;
                    unsigned int c = 0;
                    for (; c < ce; ++c)
                    {
                        if (v < ver_block_count && h + c < hor_block_count)
                            combine_qp_mode(ptr, mbqp[start + c + h].QP, mbqp[start + c + h].Mode, rep);
                        else
                            combine_qp_mode(ptr, 0, 0, rep);
                    }
                    vc++;
                    if (rep > 1)
                    {
                        uint8_t* src = ptr - 8;
                        for (c = 1; c < rep; ++c, ++vc, ptr += 8)
                            memcpy(ptr, src, 8);
                    }
                    v++;
                }
                v = sv;
            }
            ptr += extra_right;
        }
    }

    inline mfxU32 align_size_64(mfxU32  size)
    {
        auto sz = size >> 6;
        if (size & 0x3f)
            sz++;
        return sz << 6;
    }

mfxStatus ROIMapBlockMapper::Init(mfxU32 width, mfxU32 height, mfxU32 orig_block_size)
{
    m_hor_block_count = (width) / orig_block_size;
    if (width % orig_block_size)
        m_hor_block_count++;
    m_ver_block_count = (height) / orig_block_size;
    if (height % orig_block_size)
        m_ver_block_count++;
    auto aligned_width = align_size_64(width);
    auto aligned_height = align_size_64(height);
    m_block_count = (aligned_width >> 3) * (aligned_height >> 3);
    m_rep = orig_block_size == 8 ? 1 : (orig_block_size == 16 ? 2 : 4);
    return MFX_ERR_NONE;
}

mfxStatus ROIMapBlockMapper::MapBlocks(mfxExtMBQP* qp, uint8_t* dst)
{
    //PrintMBQP(qp->QP, m_hor_block_count, m_ver_block_count, "src.data");
    RecalculateBlocks(qp->QPmode, m_hor_block_count, m_ver_block_count, m_rep, dst);
    //PrintMBQP(dst, m_hor_block_count * m_rep, m_ver_block_count * m_rep, "dst.data");
    return MFX_ERR_NONE;
}

}
}
} //namespace HEVCEHW::Linux::KMB

void HEVCEHW::Linux::KMB::VSIROIMap::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_PatchDDITask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        
        auto& task = Task::Common::Get(s_task);
        auto& par = Glob::VideoParam::Get(global);

        mfxExtMBQP* mbqp = ExtBuffer::Get(task.ctrl);

        if (!mbqp)
        {
            mbqp = ExtBuffer::Get(par);
            if (mbqp && !mbqp->NumQPAlloc)
                mbqp = nullptr;
        }

		if (mbqp)
        {
            //buffer
            m_roiMap.width = task.pSurfIn->Info.CropW;
            m_roiMap.height = task.pSurfIn->Info.CropH;

            auto idx = mbqp->BlockSize == 64 ? 0 : 3;
            m_roiMap.mbqp = mbqp;

			if (idx) 
                m_roiMap.mapper.Init(m_roiMap.width, m_roiMap.height, mbqp->BlockSize); // block size != 64

            auto& glob_par = Glob::DDI_SubmitParam::Get(global);
            DDIExecParam xPar;
            xPar.Function = HANTROEncROIMapBufferType;
            xPar.In.pData = &m_roiMap;
            xPar.In.Size = sizeof(m_roiMap);
            xPar.In.Num = 1;
            glob_par.push_back(xPar);

            //metadata
            memset(&m_vsi_rm, 0, sizeof(m_vsi_rm));
            m_vsi_rm.type = (VAEncMiscParameterType)HANTROEncMiscParameterTypeROIMap;
            m_vsi_rm.rm.roimap_buf_id = 0;
            m_vsi_rm.rm.roi_flags.bits.roimap_is_enabled = 1;
            m_vsi_rm.rm.roi_flags.bits.roimap_block_unit = idx;
            m_vsi_rm.rm.roi_flags.bits.ipcmmap_is_enabled = 0; //not in POR for KMB
            m_vsi_rm.rm.roi_flags.bits.pcm_loop_filter_disabled_flag = 0;

            DDIExecParam xPar1;
            xPar1.Function = VAEncMiscParameterBufferType;
            xPar1.In.pData = &m_vsi_rm;
            xPar1.In.Size = sizeof(m_vsi_rm);
            xPar1.In.Num = 1;

            glob_par.push_back(xPar1);
        }
       
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
