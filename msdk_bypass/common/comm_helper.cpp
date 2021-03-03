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
//! \file    comm_helper.cpp
//! \brief   Implementation of communication helper classes
//! \details Provide easier implementation of upper level code.
//!

#include "comm_helper.h"
#include "hddl_va_shim_common.h"

void Throw(const char* msg)
{
    if (msg && *msg)
    {
        SHIM_NORMAL_MESSAGE("%s", msg);
    }
    throw RuntimeError(msg);
}

void Throw(uint32_t code, const char* msg)
{
    if (msg && *msg)
    {
        SHIM_NORMAL_MESSAGE("%s", msg);
    }
    throw RuntimeError(code, msg);
}

PayloadWriter::PayloadWriter(mfxDataPacket& payload, uint32_t id) : PayloadHelper(payload)
{
    head_.functionID = id;
    pos_ = sizeof(head_);
    payload_.resize(pos_);
    head_.size = (uint32_t)payload_.size();
    UpdateHeader();
}

void PayloadWriter::Write(const void* addr, size_t size)
{
    if (!size)
        return;

    size_t sz = payload_.size() - pos_;
    if (sz < size)
        payload_.resize(payload_.size() + size - sz);
    memcpy(&payload_.at(pos_), addr, size);
    head_.size = (uint32_t)payload_.size();
    memcpy(&payload_.at(0), &head_, sizeof(head_));
    pos_ += size;
}

void PayloadReader::Read(void* data, size_t size)
{
    if (!size)
        return;

    if (pos_ + size > payload_.size())
        Throw("No data to read");
    memcpy(data, &payload_.at(pos_), size);
    pos_ += size;
}

