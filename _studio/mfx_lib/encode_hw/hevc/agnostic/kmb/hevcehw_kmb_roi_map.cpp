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

#include "hevcehw_kmb_roi_map.h"
#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::KMB;

void VSIROIMap::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& /*global*/) -> mfxStatus
    {
        mfxExtMBQP* extMBQP = ExtBuffer::Get(par);
        
		if (extMBQP && extMBQP->QP)
        {
            bool changed = false;
            mfxI32 sizes[] = { 8, 16, 32, 64 };
            if (std::find(std::begin(sizes), std::end(sizes), extMBQP->BlockSize) == std::end(sizes))
                return MFX_ERR_UNSUPPORTED;

			mfxI32 modes[] = { MFX_MBQP_MODE_QP_VALUE, MFX_MBQP_MODE_QP_DELTA, MFX_MBQP_MODE_QP_ADAPTIVE };
			if (std::find(std::begin(modes), std::end(modes), extMBQP->Mode) == std::end(modes))
				return MFX_ERR_UNSUPPORTED;

            mfxU32 num_blocks = par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height / (extMBQP->BlockSize * extMBQP->BlockSize);
            // Check number of blocks
            if (extMBQP->NumQPAlloc < num_blocks)
                return MFX_ERR_UNSUPPORTED;

            // Check QP values
            for (mfxU32 i = 0; i < num_blocks; ++i)
			{
				mfxI32 sub_modes[] = { MFX_MBQP_MODE_QP_VALUE, MFX_MBQP_MODE_QP_DELTA };
				if (std::find(std::begin(sub_modes), std::end(sub_modes), extMBQP->QPmode[i].Mode) == std::end(sub_modes))
					return MFX_ERR_UNSUPPORTED;

                if (extMBQP->QPmode[i].Mode == MFX_MBQP_MODE_QP_VALUE)
                {
                    const mfxU8 MAX_QP_VALUE = 51;
                    if (extMBQP->QPmode[i].QP > MAX_QP_VALUE)
                    {
                        changed = true;
						extMBQP->QPmode[i].QP = MAX_QP_VALUE;
                    }
                }
				else
				{
					const mfxI8 MAX_DELTA_QP_VALUE = 31;
					const mfxI8 MIN_DELTA_QP_VALUE = -32;
					if (extMBQP->QPmode[i].DeltaQP > MAX_DELTA_QP_VALUE)
					{
						changed = true;
						extMBQP->QPmode[i].DeltaQP = MAX_DELTA_QP_VALUE;
					}
					if (extMBQP->QPmode[i].DeltaQP < MIN_DELTA_QP_VALUE)
					{
						changed = true;
						extMBQP->QPmode[i].DeltaQP = MIN_DELTA_QP_VALUE;
					}
				}
			}
            if (changed)
                return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }
        return MFX_ERR_NONE;
    });
}

void VSIROIMap::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_ConfigureTask
        , [this](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);
        auto& par = Glob::VideoParam::Get(global);

        mfxExtMBQP* mbqp = ExtBuffer::Get(task.ctrl);
        if (!mbqp)
        {
            if ((mbqp = ExtBuffer::Get(par)) != nullptr && !mbqp->QP)
                mbqp = nullptr;
        }
        if (mbqp)
        {
            std::vector<mfxI32> sizes = { 8, 16, 32, 64 };
			std::vector<mfxI32> modes = { MFX_MBQP_MODE_QP_VALUE, MFX_MBQP_MODE_QP_DELTA, MFX_MBQP_MODE_QP_ADAPTIVE };

            task.bCUQPMap |= (mbqp && mbqp->NumQPAlloc);
            task.bCUQPMap &= std::find(sizes.begin(), sizes.end(), mbqp->BlockSize) != sizes.end();
			task.bCUQPMap &= std::find(modes.begin(), modes.end(), mbqp->Mode) != modes.end();
        }
        
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
