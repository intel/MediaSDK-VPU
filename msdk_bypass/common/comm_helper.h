/*
 * Copyright (c) 2020 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//!
//! \file    comm_helper.h
//! \brief   Interface of communication helper classes
//! \details Provide easier implementation of upper level code.
//!

#pragma once

extern "C" {
#include "gen_comm.h"
}

#include "mfxdefs.h"
#include "mfxcommon.h"
#include "mfxstructures.h"

#include <list>
#include <vector>
#include <stdexcept>

//!
//! \brief   Communcation settings initialization
//! \return  void
//!          Return nothing
//!

class RuntimeError : public std::runtime_error
{
protected:
    uint32_t    code_ = MFX_ERR_UNKNOWN;
public:
    explicit RuntimeError(const char* msg) : std::runtime_error(msg) {}
    explicit RuntimeError(uint32_t code, const char* msg = "") : std::runtime_error(msg), code_(code) {}
    virtual ~RuntimeError() {}
    void SetCode(uint32_t code) { code_ = code; }
    uint32_t GetCode() { return code_; }
};

void Throw(const char* msg);
void Throw(uint32_t code, const char* msg);

using mfxDataPacket = std::vector<uint8_t>;

struct PayloadHeader
{
    inline PayloadHeader(uint32_t id = 0, uint32_t sz = 0) : functionID(id), size(sz) {}
    union {
        HDDLVAFunctionID id;
        uint32_t    functionID;
        uint32_t    status; 
    };
    uint32_t    size;
};

const   size_t INVALID_PAYLOAD_POS = -1;

class PayloadHelper
{
protected:
    PayloadHeader           head_;
    size_t                  pos_;
    mfxDataPacket&          payload_;
public:
    explicit  PayloadHelper(mfxDataPacket& payload) :  pos_(0), payload_(payload) {}
    virtual ~PayloadHelper() {}
    size_t Pos()            { return pos_; }
    size_t Size()           { return payload_.size(); }
    size_t Seek(size_t pos)
    {
        if (pos > payload_.size())
            Throw("Seek after data");
        std::swap(pos_, pos);
        return pos;
    }
    size_t Skip(size_t bytes)
    {
        return Seek(pos_ + bytes);
    }
    void Clear()
    {
        payload_.clear();
        pos_ = 0;
    }
};

class PayloadWriter : public PayloadHelper
{
protected:
    void UpdateHeader() { memcpy(&payload_.at(0), &head_, sizeof(head_)); }
public:
    explicit PayloadWriter(mfxDataPacket& payload, uint32_t id = 0);
    void SetStatus(uint32_t id) { SetId(id); }

    void SetId(uint32_t id)
    {
        head_.functionID = id;
        UpdateHeader();
    }

    void Write(const void* addr, size_t size);

    template<typename T>
    void WriteArray(const T* addr, size_t size) {
        unsigned char available = !!addr;
        Write(available);
        if (available)
            Write(addr, sizeof(T) * size);
    }

    template<typename T>
    void Write(const T& data) { Write(&data, sizeof(data)); }

    template<typename T>
    void WriteStruct(const T* data) {
        Write<void*>((void*)data);
        if (data)
            Write(*data);
    }

    void WriteExtBuffers(mfxExtBuffer** ExtParam, mfxU16 NumExtParam);
    void WriteExtBuffer(mfxExtBuffer* buf);
};

/*template<>
inline size_t PayloadWriter::Write(const mfxBitstream& bs) {

    size_t sz = Write(bs.DataOffset);
    sz += Write(bs.DataLength);
    sz += Write(bs.PicStruct);
    sz += Write(bs.FrameType);

    return sz;
}*/

template<>
inline void PayloadWriter::Write(const mfxBitstream& bs) {

    Write(bs.DecodeTimeStamp);
    Write(bs.TimeStamp);
    Write(bs.DataOffset);
    Write(bs.DataLength);
    Write(bs.MaxLength);

    Write(bs.PicStruct);
    Write(bs.FrameType);
    Write(bs.DataFlag);

    WriteExtBuffers(bs.ExtParam, bs.NumExtParam);
}

template<>
inline void PayloadWriter::Write(const mfxPayload& par) {
    Write(&par, sizeof(par));
    Write(par.Data, par.BufSize);
}

template<>
inline void PayloadWriter::Write(const mfxEncodeCtrl& par) {
    Write(&par, sizeof(par));
    WriteExtBuffers(par.ExtParam, par.NumExtParam);
}

template<>
inline void PayloadWriter::Write(const mfxVideoParam& par) {

    Write(par.AllocId);
    Write(par.AsyncDepth);
    Write(par.mfx);
    Write(par.Protected);
    Write(par.IOPattern);

    WriteExtBuffers(par.ExtParam, par.NumExtParam);
}

inline void PayloadWriter::WriteExtBuffers(mfxExtBuffer** ExtParam, mfxU16 NumExtParam) {
    Write(ExtParam);
    Write(NumExtParam);
    if (!ExtParam || !NumExtParam)
        return;

    for (; ExtParam && NumExtParam; NumExtParam--, ExtParam++)
    {
        Write(*ExtParam);
        if (!*ExtParam)
            continue;

        if ((*ExtParam)->BufferSz < sizeof(mfxExtBuffer))
            (*ExtParam)->BufferSz = sizeof(mfxExtBuffer);

        Write((*ExtParam)->BufferSz);
        WriteExtBuffer(*ExtParam);
    }

    return;
}

inline void PayloadWriter::WriteExtBuffer(mfxExtBuffer* buf) {
    Write(buf, buf->BufferSz);
    switch (buf->BufferId)
    {
    case MFX_EXTBUFF_MBQP:
    {
        mfxExtMBQP* mbqp = reinterpret_cast<mfxExtMBQP*>(buf);
        Write(mbqp->Mode);
        Write(mbqp->BlockSize);
        Write(mbqp->NumQPAlloc);
        auto sz = mbqp->Mode == MFX_MBQP_MODE_QP_ADAPTIVE ? sizeof(mfxQPandMode) : 1;
        Write(mbqp->QP, mbqp->NumQPAlloc * sz);
    }
    break;
    case MFX_EXTBUFF_ENCODER_IPCM_AREA:
    {
        mfxExtEncoderIPCMArea* ipcm = reinterpret_cast<mfxExtEncoderIPCMArea*>(buf);
        Write(ipcm->NumArea);
        Write(ipcm->Area, ipcm->NumArea * sizeof(ipcm->Area[0]));
    }
    break;
    case MFX_EXTBUFF_CODING_OPTION_SPSPPS:
    {
        mfxExtCodingOptionSPSPPS* spspps = reinterpret_cast<mfxExtCodingOptionSPSPPS*>(buf);
        Write(spspps->SPSId);
        Write(spspps->PPSId);
        Write(spspps->SPSBufSize);
        Write(spspps->PPSBufSize);
        Write(spspps->SPSBuffer);
        Write(spspps->PPSBuffer);
        Write(spspps->SPSBuffer, spspps->SPSBufSize);
        Write(spspps->PPSBuffer, spspps->PPSBufSize);
    }
    break;
    }
}

#if defined(MFX_ONEVPL)
template<>
inline size_t PayloadWriter::Write(const mfxDecoderDescription::decoder::decprofile::decmemdesc& data) {
    size_t sz = Write(&data, sizeof(data));
    return sz + WriteArray(data.ColorFormats, data.NumColorFormats);
}

template<>
inline size_t PayloadWriter::Write(const mfxDecoderDescription::decoder::decprofile& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumMemTypes; i++) {
        sz += Write(data.MemDesc[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxDecoderDescription::decoder& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumProfiles; i++) {
        sz += Write(data.Profiles[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxDecoderDescription& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumCodecs; i++) {
        sz += Write(data.Codecs[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxEncoderDescription::encoder::encprofile::encmemdesc& data) {
    size_t sz = Write(&data, sizeof(data));
    return sz + WriteArray(data.ColorFormats, data.NumColorFormats);
}

template<>
inline size_t PayloadWriter::Write(const mfxEncoderDescription::encoder::encprofile& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumMemTypes; i++) {
        sz += Write(data.MemDesc[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxEncoderDescription::encoder& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumProfiles; i++) {
        sz += Write(data.Profiles[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxEncoderDescription& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumCodecs; i++) {
        sz += Write(data.Codecs[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxVPPDescription::filter::memdesc::format& data) {
    size_t sz = Write(&data, sizeof(data));
    return sz + WriteArray(data.OutFormats, data.NumOutFormat);
}

template<>
inline size_t PayloadWriter::Write(const mfxVPPDescription::filter::memdesc& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumInFormats; i++) {
        sz += Write(data.Formats[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxVPPDescription::filter& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumMemTypes; i++) {
        sz += Write(data.MemDesc[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxVPPDescription& data) {
    size_t sz = Write(&data, sizeof(data));
    for (int i = 0; i < data.NumFilters; i++) {
        sz += Write(data.Filters[i]);
    }
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxDeviceDescription& data) {
    size_t sz = Write(&data, sizeof(data)) + WriteArray(data.SubDevices, data.NumSubDevices);
    return sz;
}

template<>
inline size_t PayloadWriter::Write(const mfxImplDescription& data) {
    size_t sz = Write(&data, sizeof(data));
    sz += Write(data.Dev);
    sz += Write(data.Dec);
    sz += Write(data.Enc);
    sz += Write(data.VPP);
    return sz;
}
#endif // #if defined(MFX_ONEVPL)

class PayloadReader : public PayloadHelper
{
public:
    explicit PayloadReader(const mfxDataPacket& payload) : PayloadHelper(const_cast<mfxDataPacket&>(payload))
    {
        if (payload_.size() >= sizeof(head_)) {
            head_ = *((const PayloadHeader *)payload_.data());
            pos_ = sizeof(head_);
        }
    }

    ~PayloadReader() {
        Close();
    }

    mfxStatus GetStatus() { return mfxStatus(head_.status); }
    uint32_t GetId() { return uint32_t(head_.functionID); }

    void Read(void* data, size_t sz);

    template<typename T>
    void Read(T& data) { Read(&data, sizeof(data)); }

    template<typename T>
    void Peek(T& data) { Read(&data, sizeof(data)); Seek(pos_ - sizeof(T)); }

    template<typename T>
    void ReadArray(T*& addr, size_t size) {
        unsigned char available = 0;
        addr = 0;
        Read(available);
        if (!available)
            return;

        addr = new T[size];
        Read(addr, sizeof(T) * size);
    }

    template<typename T>
    void ReadStruct(T* &data) {
        void* ptr = 0;
        Read<void*>(ptr);
        if (ptr) {
            if (!data)
                data = Alloc<T>(1);
            Read(*data);
        }
        else
            data = 0;
    }

    inline void ReadExtBuffers(mfxExtBuffer** &ExtParam, mfxU16 &NumExtParam);
    mfxStatus ReadExtBuffer(mfxExtBuffer* & buf);

    std::list<mfxU8*> dataCache;

protected:

    template<typename T>
    T* Alloc(size_t size) {
        T* ptr = new T[size];
        std::fill_n(ptr, size, T());
        dataCache.push_back((mfxU8 *)ptr);
        return ptr;
    }

    void Close() {
        for (auto& p : dataCache)
            delete[] p;
        dataCache.clear();
    }
};

template<>
inline void PayloadReader::Read(mfxBitstream& bs) {

    Read(bs.DecodeTimeStamp);
    Read(bs.TimeStamp);
    Read(bs.DataOffset);
    Read(bs.DataLength);
    Read(bs.MaxLength);

    Read(bs.PicStruct);
    Read(bs.FrameType);
    Read(bs.DataFlag);

    ReadExtBuffers(bs.ExtParam, bs.NumExtParam);
}

template<>
inline void PayloadReader::Read(mfxPayload& par) {
    mfxPayload p;
    Read(&p, sizeof(p));

    par.NumBit = p.NumBit;
    par.Type = p.Type;
    par.CtrlFlags = p.CtrlFlags;
    par.BufSize = p.BufSize;

    if (!par.Data) {
        par.Data = Alloc<mfxU8>(par.BufSize);
    }

    Read(par.Data, par.BufSize);
}

template<>
inline void PayloadReader::Read(mfxEncodeCtrl& par) {
    Read(&par, sizeof(par));
    par.ExtParam = 0;
    par.NumExtParam = 0;
    par.Payload = 0;
    par.NumPayload = 0;
    ReadExtBuffers(par.ExtParam, par.NumExtParam);
}

template<>
inline void PayloadReader::Read(mfxVideoParam& par) {

    Read(par.AllocId);
    Read(par.AsyncDepth);
    Read(par.mfx);
    Read(par.Protected);
    Read(par.IOPattern);

    ReadExtBuffers(par.ExtParam, par.NumExtParam);
}

inline void PayloadReader::ReadExtBuffers(mfxExtBuffer**& ExtParam, mfxU16 &NumExtParam) {

    mfxExtBuffer** ext;
    Read(ext);

    mfxU16 num;
    Read(num);
    
    if (!ext)
        ExtParam = 0;

    NumExtParam = num;

    if (!ext || !num)
        return;

    if (!ExtParam)
        ExtParam = Alloc<mfxExtBuffer*>(num);

    ext = ExtParam;
    for (; num; num--, ext++)
    {
        mfxExtBuffer* e;
        Read(e);
        if (!e)
            continue;

        ReadExtBuffer(*ext);
    }

    return;
}

inline mfxStatus PayloadReader::ReadExtBuffer(mfxExtBuffer* & buf)
{
    mfxU32 size;
    Read(size);

    bool isExist = !!buf;
    mfxExtBuffer* temp = 0;
    if (!buf) {
        buf = (mfxExtBuffer*)Alloc<mfxU8>(size);
        temp = buf;
    } else {
        temp = (mfxExtBuffer*)Alloc<mfxU8>(size);
    }

    mfxExtBuffer header;
    Read(header);
    memcpy(buf, &header, sizeof(header));
    Read((uint8_t*)temp + sizeof(header), header.BufferSz - sizeof(header));

    switch (buf->BufferId)
    {
    case MFX_EXTBUFF_MBQP:
    {
        mfxExtMBQP* mbqp = reinterpret_cast<mfxExtMBQP*>(buf);
        Read(mbqp->Mode);
        Read(mbqp->BlockSize);
        Read(mbqp->NumQPAlloc);
        auto sz = mbqp->Mode == MFX_MBQP_MODE_QP_ADAPTIVE ? sizeof(mfxQPandMode) : 1;
        if (!isExist)
            mbqp->QP = Alloc<mfxU8>(mbqp->NumQPAlloc * sz);
        Read(mbqp->QP, mbqp->NumQPAlloc * sz);
    }
    break;
    case MFX_EXTBUFF_ENCODER_IPCM_AREA:
    {
        mfxExtEncoderIPCMArea* ipcm = reinterpret_cast<mfxExtEncoderIPCMArea*>(buf);
        Read(ipcm->NumArea);
        if (!isExist)
            ipcm->Area = (mfxExtEncoderIPCMArea::area *)Alloc<mfxU8>(ipcm->NumArea * sizeof(ipcm->Area[0]));
        Read(ipcm->Area, ipcm->NumArea * sizeof(ipcm->Area[0]));
    }
    break;
    case MFX_EXTBUFF_CODING_OPTION_SPSPPS:
    {
        mfxExtCodingOptionSPSPPS* spspps = reinterpret_cast<mfxExtCodingOptionSPSPPS*>(buf);
        Read(spspps->SPSId);
        Read(spspps->PPSId);

        Read(spspps->SPSBufSize);
        Read(spspps->PPSBufSize);

        mfxU8* spsBuf = 0;
        Read(spsBuf);
        mfxU8* ppsBuf = 0;
        Read(ppsBuf);

        if (!isExist) {
            if (spsBuf)
                spspps->SPSBuffer = Alloc<mfxU8>(spspps->SPSBufSize);
            if (ppsBuf)
                spspps->PPSBuffer = Alloc<mfxU8>(spspps->PPSBufSize);
        }

        if (spsBuf)
            Read(spspps->SPSBuffer, spspps->SPSBufSize);
        else
            spspps->SPSBuffer = 0;

        if (ppsBuf)
            Read(spspps->PPSBuffer, spspps->PPSBufSize);
        else
            spspps->PPSBuffer = 0;
    }
    break;
    default:
        std::copy((uint8_t*)temp + sizeof(header),
            (uint8_t*)temp + sizeof(header) + header.BufferSz - sizeof(header),
            (uint8_t*)buf + sizeof(header));
        break;
    }
    return MFX_ERR_NONE;
}

#if defined(MFX_ONEVPL)
template<>
inline size_t PayloadReader::Read(mfxDecoderDescription::decoder::decprofile::decmemdesc& data) {
    size_t sz = Read(&data, sizeof(data));
    return sz + ReadArray(data.ColorFormats, data.NumColorFormats);
}

template<>
inline size_t PayloadReader::Read(mfxDecoderDescription::decoder::decprofile& data) {
    size_t sz = Read(&data, sizeof(data));
    data.MemDesc = new mfxDecoderDescription::decoder::decprofile::decmemdesc[data.NumMemTypes];
    for (int i = 0; i < data.NumMemTypes; i++) {
        sz += Read(data.MemDesc[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxDecoderDescription::decoder& data) {
    size_t sz = Read(&data, sizeof(data));
    data.Profiles = new mfxDecoderDescription::decoder::decprofile[data.NumProfiles];
    for (int i = 0; i < data.NumProfiles; i++) {
        sz += Read(data.Profiles[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxDecoderDescription& data) {
    size_t sz = Read(&data, sizeof(data));
    data.Codecs = new mfxDecoderDescription::decoder[data.NumCodecs];
    for (int i = 0; i < data.NumCodecs; i++) {
        sz += Read(data.Codecs[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxEncoderDescription::encoder::encprofile::encmemdesc& data) {
    size_t sz = Read(&data, sizeof(data));
    return sz + ReadArray(data.ColorFormats, data.NumColorFormats);
}

template<>
inline size_t PayloadReader::Read(mfxEncoderDescription::encoder::encprofile& data) {
    size_t sz = Read(&data, sizeof(data));
    data.MemDesc = new mfxEncoderDescription::encoder::encprofile::encmemdesc[data.NumMemTypes];
    for (int i = 0; i < data.NumMemTypes; i++) {
        sz += Read(data.MemDesc[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxEncoderDescription::encoder& data) {
    size_t sz = Read(&data, sizeof(data));
    data.Profiles = new mfxEncoderDescription::encoder::encprofile[data.NumProfiles];
    for (int i = 0; i < data.NumProfiles; i++) {
        sz += Read(data.Profiles[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxEncoderDescription& data) {
    size_t sz = Read(&data, sizeof(data));
    data.Codecs = new mfxEncoderDescription::encoder[data.NumCodecs];
    for (int i = 0; i < data.NumCodecs; i++) {
        sz += Read(data.Codecs[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxVPPDescription::filter::memdesc::format& data) {
    size_t sz = Read(&data, sizeof(data));
    return sz + ReadArray(data.OutFormats, data.NumOutFormat);
}

template<>
inline size_t PayloadReader::Read(mfxVPPDescription::filter::memdesc& data) {
    size_t sz = Read(&data, sizeof(data));
    data.Formats = new mfxVPPDescription::filter::memdesc::format[data.NumInFormats];
    for (int i = 0; i < data.NumInFormats; i++) {
        sz += Read(data.Formats[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxVPPDescription::filter& data) {
    size_t sz = Read(&data, sizeof(data));
    data.MemDesc = new mfxVPPDescription::filter::memdesc[data.NumMemTypes];
    for (int i = 0; i < data.NumMemTypes; i++) {
        sz += Read(data.MemDesc[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxVPPDescription& data) {
    size_t sz = Read(&data, sizeof(data));
    data.Filters = new mfxVPPDescription::filter[data.NumFilters];
    for (int i = 0; i < data.NumFilters; i++) {
        sz += Read(data.Filters[i]);
    }
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxDeviceDescription& data) {
    size_t sz = Read(&data, sizeof(data)) + ReadArray(data.SubDevices, data.NumSubDevices);
    return sz;
}

template<>
inline size_t PayloadReader::Read(mfxImplDescription& data) {
    size_t sz = Read(&data, sizeof(data));
    sz += Read(data.Dev);
    sz += Read(data.Dec);
    sz += Read(data.Enc);
    sz += Read(data.VPP);
    return sz;
}
#endif // #if defined(MFX_ONEVPL)


//EOF
