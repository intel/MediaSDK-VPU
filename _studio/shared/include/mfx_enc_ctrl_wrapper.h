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

#ifndef __MFX_ENC_CTRL_WRAPPER_H__
#define __MFX_ENC_CTRL_WRAPPER_H__

#include <map>
#include <vector>
#include "mfxbrc.h"
#include "mfx_common.h"

template<typename TB>
TB* Alloc(size_t size)
{
    auto ptr = new TB[size];
    std::fill_n(ptr, size, TB());
    return ptr;
}

template<typename T>
class ExtBufHolder : public T
{
public:
    ExtBufHolder() : T()
    {}

    ~ExtBufHolder()
    {
        ClearBuffers();

        delete[] m_ext_buf;
        m_ext_buf = nullptr;
    }

    ExtBufHolder(const ExtBufHolder& ref)
    {
        *this = ref;
    }

    ExtBufHolder(const T& ref)
    {
        *this = ref;
    }

    ExtBufHolder& operator=(const ExtBufHolder& ref)
    {
        if (this == &ref)
            return *this;
        const T* src_base = &ref;
        return operator=(*src_base);
    }

    ExtBufHolder(ExtBufHolder&& ref)
    {
        *this = std::move(ref);
    }

    ExtBufHolder& operator= (const ExtBufHolder&& ref)
    {
        const T* src_base = &ref;
        operator=(*src_base);
        ref.~ExtBufHolder();
        return *this;
    }

    ExtBufHolder& operator=(const T& ref)
    {
        if (this == &ref)
            return *this;
      
        // copy content of main structure type T
        T* dst_base = this;
        const T* ref_base = &ref;
        *dst_base = *ref_base;

        ClearBuffers();

        if (ref.NumExtParam)
        {
            if (!m_ext_buf)
            {
                m_ext_buf = Alloc<mfxExtBuffer*>(64);
            }

            for (size_t i = 0; i < ref.NumExtParam; ++i)
            {
                AddExtBuffer(ref.ExtParam[i]);
            }
        }

        return *this;
    }

private:

    mfxExtBuffer* AddExtBuffer(mfxExtBuffer *src)
    {
        if (!src || !src->BufferSz || !src->BufferId)
            throw std::logic_error("AddExtBuffer: wrong size or id!");
        /*
        auto it = FindExtBuffer(id);

        if (it)
            return it;
        */
        auto buf = (mfxExtBuffer*) Alloc<mfxU8>(src->BufferSz);

        BuildExtBuffer(buf, src);

        m_ext_buf[m_sz++] = buf;
  
        RefreshBuffers();
        return buf;
    }

    void RefreshBuffers()
    {
        this->NumExtParam = m_sz;
        this->ExtParam = m_ext_buf;
    }

    void ClearBuffers()
    {
        if (!m_ext_buf)
            return;

        auto beg = m_ext_buf;
        for (auto end = beg + m_sz; beg != end; beg++)
            if (*beg)
                ClearExtBuffer(*beg);


        m_sz = 0;
        /*
        delete[] m_ext_buf;
        m_ext_buf = nullptr;
        */
        RefreshBuffers();
    }

    void BuildExtBuffer(mfxExtBuffer* dst_, const mfxExtBuffer* src_)
    {
        memcpy((void*)dst_, (void*)src_, src_->BufferSz);

        auto buildExtSPSPPS = [](mfxExtBuffer* dst_, const mfxExtBuffer* src_) -> void {
            auto dst = (mfxExtCodingOptionSPSPPS*)dst_;
            auto src = (const mfxExtCodingOptionSPSPPS*)src_;

            if (src->SPSBufSize)
            {
                dst->SPSBuffer = Alloc<mfxU8>(src->SPSBufSize);
                memcpy(dst->SPSBuffer, src->SPSBuffer, src->SPSBufSize);
            }

            if (src->PPSBufSize)
            {
                dst->PPSBuffer = Alloc<mfxU8>(src->SPSBufSize);
                memcpy(dst->PPSBuffer, src->PPSBuffer, src->PPSBufSize);
            }
        };

        auto buildExtMBQP = [](mfxExtBuffer* dst_, const mfxExtBuffer* src_) -> void {
            auto dst = (mfxExtMBQP*)dst_;
            auto src = (const mfxExtMBQP*)src_;

            auto sz = src->Mode == MFX_MBQP_MODE_QP_ADAPTIVE ? sizeof(mfxQPandMode) : 1;

            if (!src->NumQPAlloc)
                return;

            dst->QP = Alloc<mfxU8>(src->NumQPAlloc * sz);
            memcpy(dst->QP, src->QP, src->NumQPAlloc * sz);
        };

        auto buildExtIPCM = [](mfxExtBuffer* dst_, const mfxExtBuffer* src_) -> void {
            auto dst = (mfxExtEncoderIPCMArea*)dst_;
            auto src = (const mfxExtEncoderIPCMArea*)src_;

            auto sz = src->NumArea * sizeof(src->Area[0]);

            if (!sz)
                return;

            dst->Area = (mfxExtEncoderIPCMArea::area*)Alloc<mfxU8>(sz);
            memcpy(dst->Area, src->Area, sz);
        };

        using fp = void(mfxExtBuffer*, const mfxExtBuffer*);

        std::map<int32_t, fp*> v = {
            {MFX_EXTBUFF_CODING_OPTION_SPSPPS, buildExtSPSPPS },
            {MFX_EXTBUFF_MBQP, buildExtMBQP },
            {MFX_EXTBUFF_ENCODER_IPCM_AREA, buildExtIPCM },
        };

        if (v.find(src_->BufferId) != v.end())
            v[src_->BufferId](dst_, src_);
    }

    void ClearExtBuffer(mfxExtBuffer* buf_)
    {
        auto ClearExtSPSPPS = [](mfxExtBuffer* buf_) -> void {
            auto dst = (mfxExtCodingOptionSPSPPS*)buf_;

            if (dst && dst->SPSBufSize && dst->SPSBuffer)
                delete[] dst->SPSBuffer;

            if (dst && dst->PPSBufSize && dst->PPSBuffer)
                delete[] dst->PPSBuffer;
        };

        auto ClearExtMBQP = [](mfxExtBuffer* buf_) -> void {
            auto dst = (mfxExtMBQP*)buf_;

            if (dst && dst->NumQPAlloc && dst->QP)
                delete[] dst->QP;
        };

        auto ClearExtIPCM = [](mfxExtBuffer* buf_) -> void {
            auto dst = (mfxExtEncoderIPCMArea*)buf_;

            if (dst && dst->NumArea && dst->Area)
                delete[] dst->Area;
        };

        using fp = void(mfxExtBuffer*);

        std::map<int32_t, fp*> v = {
            {MFX_EXTBUFF_CODING_OPTION_SPSPPS, ClearExtSPSPPS },
            {MFX_EXTBUFF_MBQP, ClearExtMBQP },
            {MFX_EXTBUFF_ENCODER_IPCM_AREA, ClearExtIPCM },
        };

        if (v.find(buf_->BufferId) != v.end())
            v[buf_->BufferId](buf_);

        delete[] buf_;
    }

    mfxExtBuffer**  m_ext_buf = nullptr;
    mfxI32 m_sz = 0;
};

using mfxEncodeCtrlWrap = ExtBufHolder<mfxEncodeCtrl>;


#endif // __MFX_ENC_CTRL_WRAPPER_H__
