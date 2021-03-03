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

#include "libmfx_allocator_hddl.h"

#if defined (MFX_VA_LINUX) && defined(HDDL_UNITE)

#include <va/va_drmcommon.h>

#ifndef MFX_HW_VSI_TARGET
#include <WorkloadContextC.h>
#include <WorkloadContext.h>
#endif

#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
#endif

#if defined(LINUX32) || defined(LINUX64)
#include <unistd.h>
#include <sys/syscall.h>
#include <va/va_drm.h>
#endif

#ifndef MFX_HW_VSI_TARGET

static unsigned int ConvertMfxFourccToVAFormat(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
        return VA_FOURCC_NV12;
    case MFX_FOURCC_YUY2:
        return VA_FOURCC_YUY2;
    case MFX_FOURCC_UYVY:
        return VA_FOURCC_UYVY;
    case MFX_FOURCC_YV12:
        return VA_FOURCC_YV12;
#if (MFX_VERSION >= 1028)
    case MFX_FOURCC_RGB565:
        return VA_FOURCC_RGB565;
#endif
    case MFX_FOURCC_RGB4:
        return VA_FOURCC_ARGB;
    case MFX_FOURCC_BGR4:
        return VA_FOURCC_ABGR;
    case MFX_FOURCC_RGBP:
        return VA_FOURCC_RGBP;
    case MFX_FOURCC_P8:
        return VA_FOURCC_P208;
    case MFX_FOURCC_P010:
        return VA_FOURCC_P010;
    case MFX_FOURCC_A2RGB10:
        return VA_FOURCC_ARGB;  // rt format will be VA_RT_FORMAT_RGB32_10BPP
    case MFX_FOURCC_AYUV:
        return VA_FOURCC_AYUV;
#if (MFX_VERSION >= 1027) & defined(VA_FOURCC_Y210)
    case MFX_FOURCC_Y210:
        return VA_FOURCC_Y210;
    case MFX_FOURCC_Y410:
        return VA_FOURCC_Y410;
#endif
#if (MFX_VERSION >= 1031) && defined(VA_FOURCC_Y416)
    case MFX_FOURCC_P016:
        return VA_FOURCC_P016;
    case MFX_FOURCC_Y216:
        return VA_FOURCC_Y216;
    case MFX_FOURCC_Y416:
        return VA_FOURCC_Y416;
#endif
    default:
        assert(!"unsupported fourcc");
        return 0;
    }
}

static unsigned int GetNumPlanes(HddlUnite::eRemoteMemoryFormat fourcc) {
    switch (fourcc)
    {
    case HddlUnite::eRemoteMemoryFormat::NV12:
        return 2;
    case HddlUnite::eRemoteMemoryFormat::RGB:
        return 1;
    case HddlUnite::eRemoteMemoryFormat::BGR:
        return 1;
    }

    return 1;
}

static unsigned int GetVAFourcc(HddlUnite::eRemoteMemoryFormat fourcc)
{
    switch (fourcc)
    {
    case HddlUnite::eRemoteMemoryFormat::NV12:
        return VA_FOURCC_NV12;
    case HddlUnite::eRemoteMemoryFormat::RGB:
        return VA_FOURCC_ARGB;
    case HddlUnite::eRemoteMemoryFormat::BGR:
        return VA_FOURCC_ABGR;
    }

    return 0;
}

static mfxStatus GetVAFourcc(mfxU32 fourcc, unsigned int& va_fourcc)
{
    // VP8 hybrid driver has weird requirements for allocation of surfaces/buffers for VP8 encoding
    // to comply with them additional logic is required to support regular and VP8 hybrid allocation pathes
    mfxU32 mfx_fourcc = fourcc;
    va_fourcc = ConvertMfxFourccToVAFormat(mfx_fourcc);
    if (!va_fourcc || ((VA_FOURCC_NV12 != va_fourcc) &&
        (VA_FOURCC_YV12 != va_fourcc) &&
        (VA_FOURCC_YUY2 != va_fourcc) &&
        (VA_FOURCC_ARGB != va_fourcc) &&
        (VA_FOURCC_ABGR != va_fourcc) &&
        (VA_FOURCC_RGBP != va_fourcc) &&
        (VA_FOURCC_P208 != va_fourcc) &&
        (VA_FOURCC_P010 != va_fourcc) &&
        (VA_FOURCC_YUY2 != va_fourcc) &&
#if (MFX_VERSION >= 1027) & defined(VA_FOURCC_Y210)
        (VA_FOURCC_Y210 != va_fourcc) &&
        (VA_FOURCC_Y410 != va_fourcc) &&
#endif
#if (MFX_VERSION >= 1028)
        (VA_FOURCC_RGB565 != va_fourcc) &&
#endif
#if (MFX_VERSION >= 1031) & defined(VA_FOURCC_Y416)
        (VA_FOURCC_P016 != va_fourcc) &&
        (VA_FOURCC_Y216 != va_fourcc) &&
        (VA_FOURCC_Y416 != va_fourcc) &&
#endif
        (VA_FOURCC_AYUV != va_fourcc)))
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    return MFX_ERR_NONE;
}

mfxU64 msdk_get_current_pid()
{
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentProcessId();
#else
    return syscall(SYS_getpid);
#endif
}

mfxU64 msdk_get_current_tid()
{
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#else
    return syscall(SYS_gettid);
#endif
}

HDDLAdapterFrameAllocator::HDDLAdapterFrameAllocator(mfxFrameAllocator& allocator, VADisplay dpy)
    : alloc(allocator)
    , m_dpy(dpy)
{
    pthis = this;
    Alloc = Alloc_;
    Lock = Lock_;
    Free = Free_;
    Unlock = Unlock_;
    GetHDL = GetHDL_;
}

HDDLAdapterFrameAllocator::~HDDLAdapterFrameAllocator()
{
    Close();
}

mfxStatus HDDLAdapterFrameAllocator::Init() {
    HddlStatusCode hddlStatus = getWorkloadContextId(msdk_get_current_pid(), msdk_get_current_tid(), &workloadId);
    if (hddlStatus != HDDL_OK)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    workload = HddlUnite::queryWorkloadContext(workloadId);
    if (!workload.get())
        return MFX_ERR_DEVICE_FAILED;

    return MFX_ERR_NONE;
}

mfxStatus HDDLAdapterFrameAllocator::Alloc_(mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    if (0 == pthis)
        return MFX_ERR_MEMORY_ALLOC;

    HDDLAdapterFrameAllocator& self = *(HDDLAdapterFrameAllocator*)pthis;
    return (*self.alloc.Alloc)(self.alloc.pthis, request, response);
}

mfxStatus HDDLAdapterFrameAllocator::Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
{
    if (0 == pthis)
        return MFX_ERR_MEMORY_ALLOC;

    HDDLAdapterFrameAllocator& self = *(HDDLAdapterFrameAllocator*)pthis;
    return (*self.alloc.Lock)(self.alloc.pthis, mid, ptr);
}

mfxStatus HDDLAdapterFrameAllocator::Unlock_(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
{
    if (0 == pthis)
        return MFX_ERR_MEMORY_ALLOC;

    HDDLAdapterFrameAllocator& self = *(HDDLAdapterFrameAllocator*)pthis;
    return (*self.alloc.Unlock)(self.alloc.pthis, mid, ptr);
}

mfxStatus HDDLAdapterFrameAllocator::Free_(mfxHDL pthis, mfxFrameAllocResponse* response)
{
    if (0 == pthis)
        return MFX_ERR_MEMORY_ALLOC;

    HDDLAdapterFrameAllocator& self = *(HDDLAdapterFrameAllocator*)pthis;
    self.Close();
    return (*self.alloc.Free)(self.alloc.pthis, response);
}

mfxStatus HDDLAdapterFrameAllocator::GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL* handle)
{
    if (0 == pthis)
        return MFX_ERR_MEMORY_ALLOC;

    HDDLAdapterFrameAllocator& self = *(HDDLAdapterFrameAllocator*)pthis;
    return self.GetFrameHDL(mid, handle);
}

mfxStatus HDDLAdapterFrameAllocator::GetFrameHDL(mfxMemId mid, mfxHDL* handle) {
    mfxStatus sts = (*alloc.GetHDL)(alloc.pthis, mid, handle);
    if (sts != MFX_ERR_NONE || !handle)
        return sts;

    HddlUnite::RemoteMemory* rm = static_cast<HddlUnite::RemoteMemory*>(*handle);

    auto entry = surfaceIdMap.find(mid);

    if (entry == surfaceIdMap.end() || entry->second.rm != rm || 
        entry->second.dmaBuf != rm->getDmaBufFd()) {
        VASurfaceID s = VA_INVALID_SURFACE;

        if (rm->getDmaBufFd() == 0) {
            s = CreateSurface(rm);
        } else {
            s = CreateSurfaceFromRemoteMemory(rm);
        }

        if (s == VA_INVALID_SURFACE)
            return MFX_ERR_DEVICE_FAILED;

        if (surfaceIdMap[mid].surf != VA_INVALID_SURFACE) {
            vaDestroySurfaces(m_dpy, &surfaceIdMap[mid].surf, 1);
        }

        surfaceIdMap[mid].surf = s;
        surfaceIdMap[mid].rm = rm;
        surfaceIdMap[mid].dmaBuf = rm->getDmaBufFd();
    }

    VASurfaceID* s = &surfaceIdMap[mid].surf;
    *handle = s;
    return MFX_ERR_NONE;
}

mfxStatus HDDLAdapterFrameAllocator::UpdateRemoteMemoryAfterRealloc(const mfxFrameSurface1* surface) {
    mfxHDL handle;

    mfxMemId mid = surface->Data.MemId;
    mfxStatus sts = (*alloc.GetHDL)(alloc.pthis, mid, &handle);
    if (sts != MFX_ERR_NONE)
        return sts;

    HddlUnite::RemoteMemory* rm = static_cast<HddlUnite::RemoteMemory*>(handle);

    if (surfaceIdMap.find(mid) == surfaceIdMap.end())
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    RMEntry &entry = surfaceIdMap[mid];

    if (!entry.needUpdateDMA)
        return MFX_ERR_NONE;

    sts = ExportSurface(entry.surf, rm);
    if (sts != MFX_ERR_NONE)
        return sts;

    entry.needUpdateDMA = false;
    entry.dmaBuf = rm->getDmaBufFd();
    return MFX_ERR_NONE;
}

mfxStatus HDDLAdapterFrameAllocator::Close() {
    for (auto& s : surfaceIdMap) {
        VASurfaceID surf = s.second.surf;
        vaDestroySurfaces(m_dpy, &surf, 1);
    }

    surfaceIdMap.clear();
    return MFX_ERR_NONE;
}

VASurfaceID HDDLAdapterFrameAllocator::CreateSurfaceFromRemoteMemory(const HddlUnite::RemoteMemory* rm) {
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!rm)
        return VA_INVALID_SURFACE;

    if (!rm->getMemoryDesc().checkValid())
        return VA_INVALID_SURFACE;

    int handleFD = rm->getDmaBufFd();

    HddlUnite::RemoteMemoryDesc desc = rm->getMemoryDesc();

    VASurfaceID surface = VA_INVALID_SURFACE;

    unsigned int va_fourcc = GetVAFourcc(desc.m_format);
    if (va_fourcc == 0)
        return VA_INVALID_SURFACE;

    VASurfaceAttribExternalBuffers ext_buf = { 0 };
    ext_buf.pixel_format = va_fourcc;
    ext_buf.width = desc.m_width;
    ext_buf.height = desc.m_height;
    ext_buf.num_planes = GetNumPlanes(desc.m_format);

    for (int i = 0; i < 3; i++) {
        ext_buf.pitches[i] = desc.m_widthStride;
    }

    ext_buf.offsets[0] = 0;
    ext_buf.offsets[1] = desc.m_widthStride * desc.m_heightStride;

    ext_buf.data_size = desc.getDataSize();

    uintptr_t extbuf_handle = handleFD;
    ext_buf.buffers = &extbuf_handle;
    ext_buf.num_buffers = 1;
    ext_buf.flags &= ~VA_SURFACE_EXTBUF_DESC_ENABLE_TILING;
    ext_buf.private_data = NULL;

    VASurfaceAttrib attrib[2] = {};
    attrib[0].type = VASurfaceAttribExternalBufferDescriptor;
    attrib[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib[0].value.type = VAGenericValueTypePointer;
    attrib[0].value.value.p = &ext_buf;

    attrib[1].type = VASurfaceAttribMemoryType;
    attrib[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib[1].value.type = VAGenericValueTypeInteger;
    attrib[1].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;

    VAStatus sts = vaCreateSurfaces(
        m_dpy,
        VA_RT_FORMAT_YUV420,
        desc.m_width, desc.m_height,
        &surface,
        1,
        attrib, 2);

    if (sts != VA_STATUS_SUCCESS)
        return surface;
    return surface;
}

VASurfaceID HDDLAdapterFrameAllocator::CreateSurface(const HddlUnite::RemoteMemory* rm) {
    if (!rm)
        return MFX_ERR_NULL_PTR;

    if (!rm->getMemoryDesc().checkValid())
        return MFX_ERR_INVALID_VIDEO_PARAM;

    HddlUnite::RemoteMemoryDesc desc = rm->getMemoryDesc();

    mfxStatus mfx_res = MFX_ERR_NONE;

    VASurfaceID surface = VA_INVALID_SURFACE;
    unsigned int va_fourcc = GetVAFourcc(desc.m_format);
    if (va_fourcc == 0)
        return VA_INVALID_SURFACE;

    unsigned int format;
    VASurfaceAttrib attrib[2];
    int attrCnt = 0;

    attrib[attrCnt].type = VASurfaceAttribPixelFormat;
    attrib[attrCnt].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib[attrCnt].value.type = VAGenericValueTypeInteger;
    attrib[attrCnt++].value.value.i = va_fourcc;
    format = va_fourcc;

    if (va_fourcc == VA_FOURCC_NV12)
    {
        format = VA_RT_FORMAT_YUV420;
    }
    else if ((va_fourcc == VA_FOURCC_UYVY) || (va_fourcc == VA_FOURCC_YUY2))
    {
        format = VA_RT_FORMAT_YUV422;
    }

    VAStatus va_res = vaCreateSurfaces(m_dpy,
        format,
        desc.m_width, desc.m_height,
        &surface,
        1,
        &attrib[0], attrCnt);

    mfx_res = (va_res == VA_STATUS_SUCCESS) ? MFX_ERR_NONE : MFX_ERR_DEVICE_FAILED;

    if (va_res != VA_STATUS_SUCCESS)
        return surface;
    return surface;
}

mfxStatus HDDLAdapterFrameAllocator::ExportSurface(VASurfaceID surface, HddlUnite::RemoteMemory * rm) {
#if 1 // DMABUF_FROM_VAEXPORT
    VADRMPRIMESurfaceDescriptor desc;
    VAStatus va_res = vaExportSurfaceHandle(m_dpy, surface, VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
        VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_COMPOSED_LAYERS, &desc);

    mfxStatus mfx_res = (va_res == VA_STATUS_SUCCESS) ? MFX_ERR_NONE : MFX_ERR_DEVICE_FAILED;

    if (MFX_ERR_NONE != mfx_res)
        return mfx_res;

    if (rm->getDmaBufFd() != 0) {
        releaseVASurfaceDmaBufFd(workloadId, rm->getDmaBufFd());
    }

    HddlUnite::RemoteMemoryDesc memoryDesc = rm->getMemoryDesc();

    if (!workload || !desc.layers[0].pitch[0])
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    memoryDesc.m_widthStride = desc.layers[0].pitch[0];
    memoryDesc.m_heightStride = desc.layers[0].offset[1] / desc.layers[0].pitch[0];

    rm->update(*workload.get(), memoryDesc, desc.objects[0].fd);

#else
    VAImage image;
    VAStatus va_res = vaDeriveImage(m_dpy, surface, &image);
    mfxStatus mfx_res = va_to_mfx_status(va_res);

    if (MFX_ERR_NONE != mfx_res)
        return mfx_res;

    VABufferInfo buffer_info;
    buffer_info.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    va_res = vaAcquireBufferHandle(m_dpy, image.buf, &buffer_info);
    if (MFX_ERR_NONE != mfx_res) {
        vaDestroyImage(m_dpy, image.image_id);
        return MFX_ERR_DEVICE_FAILED;
    }

    dmaHandle = (int)buffer_info.handle;
    size = buffer_info.mem_size;
#endif

    return MFX_ERR_NONE;
}
#endif // #ifndef MFX_HW_VSI_TARGET

#endif
/* EOF */
