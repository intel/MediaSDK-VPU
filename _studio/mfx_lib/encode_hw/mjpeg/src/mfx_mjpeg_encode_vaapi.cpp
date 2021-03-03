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

#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include <type_traits>

#include "mfx_mjpeg_encode_hw_utils.h"
#include "libmfx_core_factory.h"
#include "libmfx_core_interface.h"
#include "jpegbase.h"
#include "mfx_enc_common.h"

#include "mfx_mjpeg_encode_vaapi.h"
#include "libmfx_core_interface.h"

#include "mfx_mjpeg_encode_hw_utils.h"
#include "libmfx_core_vaapi.h"
#include "fast_copy.h"

using namespace MfxHwMJpegEncode;

VAAPIEncoder::VAAPIEncoder()
 : m_core(NULL)
 , m_width(0)
 , m_height(0)
 , m_caps()
 , m_vaDisplay(0)
 , m_vaContextEncode(VA_INVALID_ID)
 , m_vaConfig(VA_INVALID_ID)
 , m_qmBufferId(VA_INVALID_ID)
 , m_htBufferId(VA_INVALID_ID)
 , m_scanBufferId(VA_INVALID_ID)
 , m_ppsBufferId(VA_INVALID_ID)
#if defined(MFX_HW_KMB)
 , m_vaHantroPreprocessBufferId(VA_INVALID_ID)
#endif
{
}

VAAPIEncoder::~VAAPIEncoder()
{
    Destroy();
}

mfxStatus VAAPIEncoder::CreateAuxilliaryDevice(
    VideoCORE * core,
    mfxU32      width,
    mfxU32      height,
    bool        isTemporal)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, __FUNCTION__);
    (void)isTemporal;

    m_core = core;
    VAAPIVideoCORE * hwcore = dynamic_cast<VAAPIVideoCORE *>(m_core);
    MFX_CHECK_WITH_ASSERT(hwcore != 0, MFX_ERR_DEVICE_FAILED);
    if(hwcore)
    {
        mfxStatus mfxSts = hwcore->GetVAService(&m_vaDisplay);
        MFX_CHECK_STS(mfxSts);
    }

    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);
    VAStatus vaSts;

    mfxI32 entrypointsIndx = 0;
    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> pEntrypoints(numEntrypoints);

    vaSts = vaQueryConfigEntrypoints(
                m_vaDisplay,
                VAProfileJPEGBaseline,
                &pEntrypoints[0],
                &numEntrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    bool bEncodeEnable = false;
    for( entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++ )
    {
        if( VAEntrypointEncPicture == pEntrypoints[entrypointsIndx] )
        {
            bEncodeEnable = true;
            break;
        }
    }
    if( !bEncodeEnable )
    {
        return MFX_ERR_DEVICE_FAILED;
    }
    m_width  = width;
    m_height = height;

    // set caps
    memset(&m_caps, 0, sizeof(m_caps));
    m_caps.Baseline         = 1;
    m_caps.Sequential       = 1;
    m_caps.Huffman          = 1;

#if !defined (MFX_HW_KMB)
    m_caps.NonInterleaved   = 0;
#else
    m_caps.NonInterleaved   = 1;
#endif

    m_caps.Interleaved      = 1;

    m_caps.SampleBitDepth   = 8;

    VAConfigAttrib attrib;

    attrib.type = VAConfigAttribEncJPEG;
    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          VAProfileJPEGBaseline,
                          VAEntrypointEncPicture,
                          &attrib, 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    VAConfigAttribValEncJPEG encAttribVal;
    encAttribVal.value = attrib.value;
    m_caps.MaxNumComponent = encAttribVal.bits.max_num_components;
    m_caps.MaxNumScan = encAttribVal.bits.max_num_scans;
    m_caps.MaxNumHuffTable = encAttribVal.bits.max_num_huffman_tables;
    m_caps.MaxNumQuantTable = encAttribVal.bits.max_num_quantization_tables;

    attrib.type = VAConfigAttribMaxPictureWidth;
    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          VAProfileJPEGBaseline,
                          VAEntrypointEncPicture,
                          &attrib, 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    m_caps.MaxPicWidth      = attrib.value;

    attrib.type = VAConfigAttribMaxPictureHeight;
    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          VAProfileJPEGBaseline,
                          VAEntrypointEncPicture,
                          &attrib, 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    m_caps.MaxPicHeight     = attrib.value;

#if defined(MFX_HW_KMB)
    GetMaxPicSize(m_vaDisplay, VAEntrypointEncPicture, VAProfileJPEGBaseline, m_caps.MaxPicWidth, m_caps.MaxPicHeight);
#endif

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::CreateAccelerationService(mfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, __FUNCTION__);
    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);
    VAStatus vaSts;

    mfxI32 entrypointsIndx = 0;
    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> pEntrypoints(numEntrypoints);

    MFX_CHECK_WITH_ASSERT(par.mfx.CodecProfile == MFX_PROFILE_JPEG_BASELINE, MFX_ERR_DEVICE_FAILED);

    vaSts = vaQueryConfigEntrypoints(
                m_vaDisplay,
                VAProfileJPEGBaseline,
                &pEntrypoints[0],
                &numEntrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    bool bEncodeEnable = false;
    for( entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++ )
    {
        if( VAEntrypointEncPicture == pEntrypoints[entrypointsIndx] )
        {
            bEncodeEnable = true;
            break;
        }
    }
    if( !bEncodeEnable )
    {
        return MFX_ERR_DEVICE_FAILED;
    }

    // Configuration
    VAConfigAttrib attrib[2];
    int attribNum = 0;

    attrib[attribNum].type = VAConfigAttribRTFormat;
    attribNum++;

    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          VAProfileJPEGBaseline,
                          VAEntrypointEncPicture,
                          attrib, attribNum);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if (!(attrib[0].value & VA_RT_FORMAT_YUV420) || !(attrib[0].value & VA_RT_FORMAT_YUV422) ) //to be do
        return MFX_ERR_DEVICE_FAILED;

    {
        vaSts = vaCreateConfig(
            m_vaDisplay,
            VAProfileJPEGBaseline,
            VAEntrypointEncPicture,
            NULL,
            0,
            &m_vaConfig);
    }

    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    // Encoder create
    vaSts = vaCreateContext(
        m_vaDisplay,
        m_vaConfig,
        m_width,
        m_height,
        VA_PROGRESSIVE,
        NULL,
        0,
        &m_vaContextEncode);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryBitstreamBufferInfo(mfxFrameAllocRequest& request)
{

    // request linear buffer
    request.Info.FourCC = MFX_FOURCC_P8;

    // context_id required for allocation video memory (tmp solution)
    request.AllocId = m_vaContextEncode;
    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryEncodeCaps(JpegEncCaps & caps)
{
    MFX_CHECK_NULL_PTR1(m_core);

    caps = m_caps;

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::RegisterBitstreamBuffer(mfxFrameAllocResponse& response)
{
    std::vector<ExtVASurface> * pQueue;
    mfxStatus sts;
    pQueue = &m_bsQueue;

    {
        // we should register allocated HW bitstreams and recon surfaces
        MFX_CHECK( response.mids, MFX_ERR_NULL_PTR );

        ExtVASurface extSurf = {VA_INVALID_ID, 0, 0, 0};
        VASurfaceID *pSurface = NULL;

        for (mfxU32 i = 0; i < response.NumFrameActual; i++)
        {
            sts = m_core->GetFrameHDL(response.mids[i], (mfxHDL *)&pSurface);
            MFX_CHECK_STS(sts);

            extSurf.number  = i;
            extSurf.surface = *pSurface;
            pQueue->push_back( extSurf );
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::Execute(DdiTask &task, mfxHDL surface)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, __FUNCTION__);
    VAStatus vaSts;
    ExecuteBuffers *pExecuteBuffers = task.m_pDdiData;
    ExtVASurface codedbuffer = m_bsQueue[task.m_idxBS];
    pExecuteBuffers->m_pps.coded_buf = (VABufferID)codedbuffer.surface;

    auto needVACreateBuf = [&](VABufferID &id, auto& curr, auto &novel) 
    {
        static_assert(std::is_same<decltype(curr), decltype(novel)>::value, "differetn types");

        if (id != VA_INVALID_ID)
        {
            if (!memcmp(&curr, &novel, sizeof(curr)))
            {
                return false;
            }
            CheckAndDestroyVAbuffer(m_vaDisplay, id);
        }
        memcpy(&curr, &novel, sizeof(curr));

        return true;
    };

#if defined(MFX_HW_KMB)
    {
        const auto& info = task.initParam.mfx.FrameInfo;

        bool need_crop = info.CropX || info.CropY || info.CropW != info.Width || info.CropH != info.Height;
        bool need_mono = task.surface->Info.ChromaFormat == MFX_CHROMAFORMAT_YUV400;
        if (need_crop || need_mono)
        {
            struct
            {
                VAEncMiscParameterType                          type;
                HANTROEncMiscParameterBufferEmbeddedPreprocess  pp;
            } buffer;

            memset(&buffer, 0, sizeof(buffer));
            buffer.type = (VAEncMiscParameterType)HANTROEncMiscParameterTypeEmbeddedPreprocess;

            if (need_mono)
            {
                buffer.pp.preprocess_flags.bits.embedded_preprocess_constant_chroma_is_enabled = 1;
                buffer.pp.constCr = buffer.pp.constCb =
                    task.surface->Info.BitDepthLuma == 10 ? 512 : 128;
            }
            buffer.pp.cropping_offset_x = info.CropX;
            buffer.pp.cropping_offset_y = info.CropY;
            buffer.pp.cropped_width = info.CropW;
            buffer.pp.cropped_height = info.CropH;
            if (needVACreateBuf(m_vaHantroPreprocessBufferId, m_activePP, buffer.pp))
            {
                vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAEncMiscParameterBufferType,
                    sizeof(buffer), 1, &buffer, &m_vaHantroPreprocessBufferId);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }
        }
    }
#endif

    if (needVACreateBuf(m_ppsBufferId, m_activePPS, pExecuteBuffers->m_pps))
    {
        vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAEncPictureParameterBufferType, sizeof(VAEncPictureParameterBufferJPEG), 1, &pExecuteBuffers->m_pps, &m_ppsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    if(pExecuteBuffers->m_dqt_list.size())
    {
        if (needVACreateBuf(m_qmBufferId, m_activeQM, pExecuteBuffers->m_dqt_list[0]))
        {
            // only the first dq table has been handled
            vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAQMatrixBufferType, sizeof(VAQMatrixBufferJPEG), 1, &pExecuteBuffers->m_dqt_list[0], &m_qmBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }
    if(pExecuteBuffers->m_dht_list.size())
    {
        if (needVACreateBuf(m_htBufferId, m_activeHT, pExecuteBuffers->m_dht_list[0]))
        {
            // only the first huffmn table has been handled
            vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAHuffmanTableBufferType, sizeof(VAHuffmanTableBufferJPEGBaseline), 1, &pExecuteBuffers->m_dht_list[0], &m_htBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }

    //special case: static internal payload data;
    if (!m_appBufferIds.size())
    {
        assert(pExecuteBuffers->m_payload_list.size());
        m_appBufferIds.resize(2, VA_INVALID_ID);

        VAEncPackedHeaderParameterBuffer packed_header_param_buffer{};
        packed_header_param_buffer.type = VAEncPackedHeaderRawData;
        packed_header_param_buffer.has_emulation_bytes = 0;
        packed_header_param_buffer.bit_length = pExecuteBuffers->m_payload_list[0].length * 8;

        vaSts = vaCreateBuffer(m_vaDisplay,
            m_vaContextEncode,
            VAEncPackedHeaderParameterBufferType,
            sizeof(packed_header_param_buffer),
            1,
            &packed_header_param_buffer,
            &m_appBufferIds[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        vaSts = vaCreateBuffer(m_vaDisplay,
            m_vaContextEncode,
            VAEncPackedHeaderDataBufferType,
            pExecuteBuffers->m_payload_list[0].length,
            1,
            pExecuteBuffers->m_payload_list[0].data,
            &m_appBufferIds[1]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    //user provided payload
    if(pExecuteBuffers->m_payload_list.size() > 1)
    {
        for (size_t index = 2; index < m_appBufferIds.size(); index++)
        {
            CheckAndDestroyVAbuffer(m_vaDisplay, m_appBufferIds[index]);
        }

        m_appBufferIds.resize(pExecuteBuffers->m_payload_list.size() * 2, VA_INVALID_ID);

        for( mfxU8 index = 1; index < pExecuteBuffers->m_payload_list.size(); index++)
        {
            VAEncPackedHeaderParameterBuffer packed_header_param_buffer{};
            packed_header_param_buffer.type = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = 0;
            packed_header_param_buffer.bit_length = pExecuteBuffers->m_payload_list[index].length * 8;
      
            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncPackedHeaderParameterBufferType,
                                    sizeof(packed_header_param_buffer),
                                    1,
                                    &packed_header_param_buffer,
                                    &m_appBufferIds[index * 2]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncPackedHeaderDataBufferType,
                                    pExecuteBuffers->m_payload_list[index].length,
                                    1,
                                    pExecuteBuffers->m_payload_list[index].data,
                                    &m_appBufferIds[index * 2 + 1]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }
    if(pExecuteBuffers->m_scan_list.size() == 1)
    {
        if (needVACreateBuf(m_scanBufferId, m_activeScanList, pExecuteBuffers->m_scan_list[0]))
        {
            vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAEncSliceParameterBufferType, sizeof(VAEncSliceParameterBufferJPEG), 1, &pExecuteBuffers->m_scan_list[0], &m_scanBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }
    else
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENCODE|JPEG|PACKET_START|", "%d|%d", m_vaContextEncode, 0);
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
        vaSts = vaBeginPicture(m_vaDisplay, m_vaContextEncode, *(VASurfaceID*)surface);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID*)&m_ppsBufferId, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        if (m_qmBufferId != VA_INVALID_ID)
        {
            vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID*)&m_qmBufferId, 1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        if (m_htBufferId != VA_INVALID_ID)
        {
            vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID*)&m_htBufferId, 1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

#if defined(MFX_HW_KMB)
        if (m_vaHantroPreprocessBufferId != VA_INVALID_ID)
        {
            vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID*)&m_vaHantroPreprocessBufferId, 1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
#endif

        if (m_appBufferIds.size())
        {
            for (mfxU8 index = 0; index < m_appBufferIds.size(); index++)
            {
                vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID*)&m_appBufferIds[index], 1);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }
        }

        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID*)&m_scanBufferId, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
        vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENCODE|JPEG|PACKET_END|", "%d|%d", m_vaContextEncode, 0);

    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback = {VA_INVALID_ID, 0, 0, 0};
        currentFeedback.number  = task.m_statusReportNumber;
        currentFeedback.surface = *(VASurfaceID*)surface;
        currentFeedback.idxBs   = task.m_idxBS;
        currentFeedback.size    = 0;

        m_feedbackCache.push_back( currentFeedback );
    }
    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryStatus(DdiTask & task)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, __FUNCTION__);
    VAStatus vaSts;
    bool isFound = false;
    VASurfaceID waitSurface;
    mfxU32 waitIdxBs;
    mfxU32 waitSize;
    mfxU32 indxSurf;
    UMC::AutomaticUMCMutex guard(m_guard);

    for( indxSurf = 0; indxSurf < m_feedbackCache.size(); indxSurf++ )
    {
        ExtVASurface currentFeedback = m_feedbackCache[ indxSurf ];
        if( currentFeedback.number == task.m_statusReportNumber )
        {
            waitSurface = currentFeedback.surface;
            waitIdxBs   = currentFeedback.idxBs;
            waitSize    = currentFeedback.size;
            isFound  = true;
            break;
        }
    }

    if( !isFound )
    {
        return MFX_ERR_UNKNOWN;
    }

    // find used bitstream
    VABufferID codedBuffer;
    if( waitIdxBs < m_bsQueue.size())
    {
        codedBuffer = m_bsQueue[waitIdxBs].surface;
    }
    else
    {
        return MFX_ERR_UNKNOWN;
    }
    if (waitSurface != VA_INVALID_SURFACE) // Not skipped frame
    {
        VASurfaceStatus surfSts = VASurfaceSkipped;


        m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);
        guard.Unlock();

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Enc vaSyncSurface");
            vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
            // it could happen that decoding error will not be returned after decoder sync
            // and will be returned at subsequent encoder sync instead
            // just ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
            if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
                vaSts = VA_STATUS_SUCCESS;
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        surfSts = VASurfaceReady;

        switch (surfSts)
        {
            case VASurfaceReady:
                VACodedBufferSegment *codedBufferSegment;
                mfxU8* dst;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "Enc vaMapBuffer");
                    vaSts = vaMapBuffer(
                        m_vaDisplay,
                        codedBuffer,
                        (void **)(&codedBufferSegment));

                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }
                task.m_bsDataLength = codedBufferSegment->size;
                
                if (task.m_bsDataLength + task.bs->DataOffset + task.bs->DataLength > task.bs->MaxLength) 
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "Enc vaUnmapBuffer");
                    vaSts = vaUnmapBuffer(m_vaDisplay, codedBuffer);
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                    return MFX_ERR_NOT_ENOUGH_BUFFER;
                }

                dst = task.bs->Data + task.bs->DataOffset + task.bs->DataLength;
                memcpy(dst, codedBufferSegment->buf, codedBufferSegment->size);

                task.bs->DataLength += task.m_bsDataLength;

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "Enc vaUnmapBuffer");
                    vaSts = vaUnmapBuffer( m_vaDisplay, codedBuffer );
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }

                return MFX_ERR_NONE;

            case VASurfaceRendering:
            case VASurfaceDisplaying:
                return MFX_WRN_DEVICE_BUSY;

            case VASurfaceSkipped:
            default:
                assert(!"bad feedback status");
                return MFX_ERR_DEVICE_FAILED;
        }
    }
    else
    {
        task.m_bsDataLength = waitSize;
        m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);
    }

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::UpdateBitstream(
    mfxMemId       MemId,
    DdiTask      & task)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, __FUNCTION__);
    mfxU8      * bsData    = task.bs->Data + task.bs->DataOffset + task.bs->DataLength;
    mfxSize     roi       = {(int)task.m_bsDataLength, 1};
    mfxFrameData bitstream = { };

    if (task.m_bsDataLength + task.bs->DataOffset + task.bs->DataLength > task.bs->MaxLength)
        return MFX_ERR_NOT_ENOUGH_BUFFER;

    m_core->LockFrame(MemId, &bitstream);
    MFX_CHECK(bitstream.Y != 0, MFX_ERR_LOCK_MEMORY);

    mfxStatus sts = FastCopy::Copy(
        bsData, task.m_bsDataLength,
        (uint8_t *)bitstream.Y, task.m_bsDataLength,
        roi, COPY_VIDEO_TO_SYS);
    assert(sts == MFX_ERR_NONE);

    task.bs->DataLength += task.m_bsDataLength;
    m_core->UnlockFrame(MemId, &bitstream);

    return sts;
}

mfxStatus VAAPIEncoder::DestroyBuffers()
{
    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_qmBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_htBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_scanBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_ppsBufferId);
    std::ignore = MFX_STS_TRACE(sts);

#if defined(MFX_HW_KMB)
    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_vaHantroPreprocessBufferId);
    std::ignore = MFX_STS_TRACE(sts);
#endif

    for (size_t index = 0; index < m_appBufferIds.size(); index++)
    {
        sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_appBufferIds[index]);
        std::ignore = MFX_STS_TRACE(sts);
    }
    m_appBufferIds.clear();

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::Destroy()
{
    m_bsQueue.clear();
    m_feedbackCache.clear();
    DestroyBuffers();

    if( m_vaContextEncode )
    {
        VAStatus vaSts = vaDestroyContext( m_vaDisplay, m_vaContextEncode );
        std::ignore = MFX_STS_TRACE(vaSts);
        m_vaContextEncode = 0;
    }

    if( m_vaConfig )
    {
        VAStatus vaSts = vaDestroyConfig( m_vaDisplay, m_vaConfig );
        std::ignore = MFX_STS_TRACE(vaSts);
        m_vaConfig = 0;
    }
    return MFX_ERR_NONE;
}

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA_LINUX)
