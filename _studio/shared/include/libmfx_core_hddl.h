// Copyright (c) 2017-2020 Intel Corporation
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

#if defined (MFX_VA_LINUX) && defined(HDDL_UNITE)

#pragma once

#ifndef MFX_HW_VSI_TARGET
#include "libmfx_allocator_hddl.h"
#endif

#undef ERROR
#include "libmfx_core_vaapi.h"

class HDDLVideoCORE : public VAAPIVideoCORE
{
public:
    friend class FactoryCORE;

    virtual eMFXVAType   GetVAType() const override { return MFX_HW_VAAPI; };

#ifndef MFX_HW_VSI_TARGET
    virtual mfxStatus SetFrameAllocator(mfxFrameAllocator* allocator) override;
#endif

    virtual void Close() override;

    mfxStatus UpdateRemoteMemoryAfterRealloc(const mfxFrameSurface1* surface);

protected:
    HDDLVideoCORE(const mfxU32 adapterNum, const mfxU32 numThreadsAvailable, const mfxSession session = NULL);
    virtual ~HDDLVideoCORE();

#ifndef MFX_HW_VSI_TARGET
    std::unique_ptr<HDDLAdapterFrameAllocator> m_hddlAllocator;
#endif

#if defined(LINUX32) || defined(LINUX64)
    int m_fd;
#endif
};

#endif // MFX_VA_LINUX
/* EOF */
