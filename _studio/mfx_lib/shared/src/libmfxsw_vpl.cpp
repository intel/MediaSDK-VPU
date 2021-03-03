// Copyright (c) 2018-2019 Intel Corporation
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

#include <mfxvideo.h>

#if defined(MFX_ONEVPL)

#include <mfximplcaps.h>

#include <mfx_session.h>
#include <mfx_trace.h>

#include <map>
#include <functional>

template <typename T>
static T* AllocDescObject(mfxU32 NumObjects) {
    T* descObject;

    try {
        descObject = new T[NumObjects];
    }
    catch (...) {
        return nullptr;
    }

    memset(descObject, 0, sizeof(T) * NumObjects);

    return descObject;
}

template <typename T>
static void FreeDescObject(T* descObject) {
    if (descObject)
        delete[] descObject;
}

struct ImplDescriptionArray {
    mfxImplDescription implDesc; // MUST be the first element

    mfxHDL* basePtr; // pointer to the array of handles
    mfxU32 currImpl; // index of this handle in the array
    mfxU32 numImpl; // total number of implementations
};

#define DEF_STRUCT_VERSION_MAJOR 1
#define DEF_STRUCT_VERSION_MINOR 0

static const mfxStructVersion DefaultStructVersion = { 
    DEF_STRUCT_VERSION_MINOR,                       
    DEF_STRUCT_VERSION_MAJOR 
};


typedef struct mfxDecoderDescription::decoder vplDecCodec;
typedef struct mfxDecoderDescription::decoder::decprofile vplDecProfile;
typedef struct mfxDecoderDescription::decoder::decprofile::decmemdesc vplDecMemDesc;

typedef struct mfxEncoderDescription::encoder vplEncCodec;
typedef struct mfxEncoderDescription::encoder::encprofile vplEncProfile;
typedef struct mfxEncoderDescription::encoder::encprofile::encmemdesc vplEncMemDesc;

mfxStatus InitDeviceDescription(mfxDeviceDescription* Dev);
mfxStatus InitDecoderCaps(mfxDecoderDescription* Dec);
mfxStatus InitEncoderCaps(mfxEncoderDescription* Enc);


///////////////////////////////////////////
//  min width,  max width,  step
//  min height, max height, step

std::vector<mfxRange32U> gAVCDecRange = {
    {48, 4096, 2},
    {48, 2160, 2}
};

std::vector<mfxRange32U> gAVCEncRange = {
    {144, 4096, 2},
    {144, 8192, 2}
};

std::vector<mfxRange32U> gHEVCDecRange = {
    {144, 4096, 2},
    {144, 2160, 2}
};

std::vector<mfxRange32U> gHEVCEncRange = {
    {136, 8192, 2},
    {136, 4096, 2}
};

std::vector<mfxRange32U> gMJPEGDecRange = {
    {48, 16384, 2},
    {48, 16384, 2}
};

std::vector<mfxRange32U> gMJPEGEncRange = {
    {32, 16384, 2},
    {32, 16384, 2}
};

std::vector<mfxRange32U> gVP9DecRange = {
    {16, 8192, 2},
    {16, 8192, 2}
};

std::vector<mfxRange32U> gMPEG2DecRange = {
    {176, 1920, 2},
    {96,  1080, 2}
};

///////////////////////////////////////////


const std::vector<mfxU32> gVPUSurf = { 
    MFX_FOURCC_RGB4, MFX_FOURCC_RGB565, 
    MFX_FOURCC_UYVY, MFX_FOURCC_YUY2, 
    MFX_FOURCC_NV21, MFX_FOURCC_NV12, 
    MFX_FOURCC_P010, MFX_FOURCC_I010, 
    MFX_FOURCC_IYUV, MFX_FOURCC_P016
};

/*
 * [Profile] --> [ResourceType1] -> [colourspaces, ....]
 *           \
 *            --> [ResourceType2] -> [colourspaces, ....]
 */
using vpuMemDesc = std::map<mfxU32, std::map<mfxResourceType, std::vector<mfxU32>>>;
//AVC:

const vpuMemDesc gAVCMem{
    {
        MFX_PROFILE_AVC_BASELINE,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_AVC_MAIN,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_AVC_HIGH,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_AVC_HIGH10,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    }
};

//HEVC:
const vpuMemDesc gHEVCMem
{
    {
        MFX_PROFILE_HEVC_MAIN,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_HEVC_MAIN10,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf},
        }
    },
    {
        MFX_PROFILE_HEVC_MAINSP,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf},
        }
    }
};

//MJPEG:
const vpuMemDesc gMJPEGMem
{
    {
        MFX_PROFILE_JPEG_BASELINE,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    }
};

//VP9:
const vpuMemDesc gVP9Mem
{
    {
        MFX_PROFILE_VP9_0,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_VP9_1,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_VP9_2,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
};

//MPEG2
const vpuMemDesc gMPEG2Mem
{
    {
        MFX_PROFILE_MPEG2_SIMPLE,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
    {
        MFX_PROFILE_MPEG2_MAIN,
        {
            {MFX_RESOURCE_VA_SURFACE,     gVPUSurf},
            {MFX_RESOURCE_SYSTEM_SURFACE, gVPUSurf},
            {MFX_RESOURCE_DMA_RESOURCE,   gVPUSurf}
        }
    },
};

///////////////////////////

struct DecoderDescs {
    mfxU32 name;
    mfxU32 level;
    vpuMemDesc memInfo;
    std::vector<mfxRange32U> range;
};

const std::initializer_list<DecoderDescs> gVPUDecoders {
     //codec,          max level,           memory layout,        min/max resolution
    {MFX_CODEC_AVC,  MFX_LEVEL_AVC_62,  std::cref(gAVCMem),   std::cref(gAVCDecRange)},
    {MFX_CODEC_HEVC, MFX_LEVEL_HEVC_62, std::cref(gHEVCMem),  std::cref(gHEVCDecRange)},
    {MFX_CODEC_JPEG, MFX_LEVEL_UNKNOWN, std::cref(gMJPEGMem), std::cref(gMJPEGDecRange)},
    {MFX_CODEC_VP9, MFX_LEVEL_UNKNOWN,  std::cref(gVP9Mem),    std::cref(gVP9DecRange)},
    {MFX_CODEC_MPEG2, MFX_LEVEL_MPEG2_MAIN, std::cref(gMPEG2Mem), std::cref(gMPEG2DecRange)}
};


struct EncoderDescs {
    mfxU32 name;
    mfxU32 level;
    mfxU32 biPred;
    vpuMemDesc memInfo;
    std::vector<mfxRange32U> range;
};

const std::initializer_list<EncoderDescs> gVPUEncoders {
     //codec,          max level,    B-slice,  memory layout,        min/max resolution
    {MFX_CODEC_AVC,  MFX_LEVEL_AVC_62,  1, std::cref(gAVCMem),   std::cref(gAVCEncRange)},
    {MFX_CODEC_HEVC, MFX_LEVEL_HEVC_62, 1, std::cref(gHEVCMem),  std::cref(gHEVCEncRange)},
    {MFX_CODEC_JPEG, MFX_LEVEL_UNKNOWN, 0, std::cref(gMJPEGMem), std::cref(gMJPEGDecRange)},
};

inline void string_copy(char * dest, const char *const_char) {
    if (sizeof(dest) < strlen(const_char) + 1)
        strncpy(dest, const_char, strlen(const_char) + 1);
}

mfxHDL* MFXQueryImplsDescription(mfxImplCapsDeliveryFormat format, mfxU32* num_impls_ptr)
{
    if (num_impls_ptr)
        *num_impls_ptr = 0;

    // only structure format is currently supported
    if (format != MFX_IMPLCAPS_IMPLDESCSTRUCTURE)
        return nullptr;

    // allocate array of mfxHDL for each implementation
    //   (currently there is just one)

    mfxU32 num_impls = 1;

    if (num_impls_ptr)
        *num_impls_ptr = num_impls;

    mfxHDL* hImpls = new mfxHDL[num_impls];
    if (!hImpls)
        return nullptr;
    memset(hImpls, 0, sizeof(mfxHDL) * num_impls);

    // allocate ImplDescriptionArray for each implementation
    // the first element must be a struct of type mfxImplDescription
    //   so the dispatcher can cast mfxHDL to mfxImplDescription, and
    //   will just be unaware of any other fields that follow
    ImplDescriptionArray* implDescArray = new ImplDescriptionArray;
    if (!implDescArray) {
        if (hImpls)
            delete[] hImpls;
        return nullptr;
    }

    // in _each_ implDescArray we allocate, save the pointer to the array of handles
    //   and the number of elements
    // MFXReleaseImplDescription can then be called on the individual
    //   handles in any order, and when the last one is freed it
    //   will delete the array of handles
    implDescArray->basePtr = hImpls;
    implDescArray->currImpl = 0;
    implDescArray->numImpl = num_impls;

    // clear everything, only allocate new structures as needed
    mfxImplDescription* implDesc = &(implDescArray->implDesc);
    memset(implDesc, 0, sizeof(mfxImplDescription));
    hImpls[0] = &(implDescArray[0]);

    implDesc->Version.Version = MFX_IMPLDESCRIPTION_VERSION;

    implDesc->Impl = MFX_IMPL_TYPE_HARDWARE;
    implDesc->AccelerationMode = MFX_ACCEL_MODE_VIA_VAAPI;

    implDesc->ApiVersion.Major = MFX_VERSION_MAJOR;
    implDesc->ApiVersion.Minor = MFX_VERSION_MINOR;

    string_copy((char*)implDesc->ImplName, "KMB VPU");
    string_copy((char*)implDesc->License, "TODO");
    string_copy((char*)implDesc->Keywords, "TODO");

    implDesc->VendorID = -1;
    implDesc->VendorImplID = 0;
    implDesc->NumExtParam = 0;

    InitDeviceDescription(&(implDesc->Dev));
    InitDecoderCaps(&(implDesc->Dec));
    InitEncoderCaps(&(implDesc->Enc));

    return hImpls;
}

mfxStatus InitDeviceDescription(mfxDeviceDescription* dev) 
{
    memset(dev, 0, sizeof(mfxDeviceDescription));

    dev->Version = DefaultStructVersion;
    string_copy((char*)dev->DeviceID, "VPU");

    dev->NumSubDevices = 1;

    return MFX_ERR_NONE;
}

mfxStatus InitDecoderCaps(mfxDecoderDescription* desc) 
{
    memset(desc, 0, sizeof(mfxDecoderDescription));

    desc->Version.Version = MFX_DECODERDESCRIPTION_VERSION;
    desc->NumCodecs = (mfxU16)gVPUDecoders.size();
    desc->Codecs = AllocDescObject<vplDecCodec>((mfxU32)gVPUDecoders.size());

    std::transform(gVPUDecoders.begin(), gVPUDecoders.end(), desc->Codecs,
        [](auto &mfxCodec) {
            const auto range = mfxCodec.range;
            vplDecCodec codec{};
            codec.CodecID = mfxCodec.name;
            codec.MaxcodecLevel = mfxCodec.level;
            codec.NumProfiles = (mfxU16)mfxCodec.memInfo.size();
            codec.Profiles = AllocDescObject<vplDecProfile>(codec.NumProfiles);

            std::transform(mfxCodec.memInfo.begin(), mfxCodec.memInfo.end(), codec.Profiles, 
                [&range](auto& mfxProf) {
                    vplDecProfile profile{};
                    profile.Profile = mfxProf.first;
                    profile.NumMemTypes = (mfxU16)mfxProf.second.size();
                    profile.MemDesc = AllocDescObject<vplDecMemDesc>(profile.NumMemTypes);

                    std::transform(mfxProf.second.begin(), mfxProf.second.end(), profile.MemDesc, 
                        [&range](auto& mfxMem) {
                            vplDecMemDesc memory{};
                            memory.Width = range[0];
                            memory.Height = range[1];
                            memory.MemHandleType = mfxMem.first;
                            memory.NumColorFormats = (mfxU16)mfxMem.second.size();
                            memory.ColorFormats = AllocDescObject<mfxU32>(memory.NumColorFormats);
                            std::copy(mfxMem.second.begin(), mfxMem.second.end(), memory.ColorFormats);

                            return memory;
                        });
                    return profile;
                });
            return codec;
        });

    return MFX_ERR_NONE;
}

mfxStatus InitEncoderCaps(mfxEncoderDescription* desc) 
{
    memset(desc, 0, sizeof(mfxEncoderDescription));

    desc->Version.Version = MFX_ENCODERDESCRIPTION_VERSION;
    desc->NumCodecs = (mfxU16)gVPUEncoders.size();
    desc->Codecs = AllocDescObject<vplEncCodec>((mfxU32)gVPUEncoders.size());

    std::transform(gVPUEncoders.begin(), gVPUEncoders.end(), desc->Codecs,
        [](auto& mfxCodec) {
            const auto range = mfxCodec.range;
            vplEncCodec codec{};
            codec.CodecID = mfxCodec.name;
            codec.MaxcodecLevel = mfxCodec.level;
            codec.BiDirectionalPrediction = mfxCodec.biPred;
            codec.NumProfiles = (mfxU16)mfxCodec.memInfo.size();
            codec.Profiles = AllocDescObject<vplEncProfile>(codec.NumProfiles);

            std::transform(mfxCodec.memInfo.begin(), mfxCodec.memInfo.end(), codec.Profiles,
                [&range](auto& mfxProf) {
                    vplEncProfile profile{};
                    profile.Profile = mfxProf.first;
                    profile.NumMemTypes = (mfxU16)mfxProf.second.size();
                    profile.MemDesc = AllocDescObject<vplEncMemDesc>(profile.NumMemTypes);

                    std::transform(mfxProf.second.begin(), mfxProf.second.end(), profile.MemDesc,
                        [&range](auto& mfxMem) {
                            vplEncMemDesc memory{};
                            memory.Width = range[0];
                            memory.Height = range[1];
                            memory.MemHandleType = mfxMem.first;
                            memory.NumColorFormats = (mfxU16)mfxMem.second.size();
                            memory.ColorFormats = AllocDescObject<mfxU32>(memory.NumColorFormats);
                            std::copy(mfxMem.second.begin(), mfxMem.second.end(), memory.ColorFormats);

                            return memory;
                        });
                    return profile;
                });
            return codec;
        });

    return MFX_ERR_NONE;
}

template<typename Desc>
void FreeCaps(Desc* desc)
{
    for (mfxU32 c = 0; c < desc->NumCodecs; c++) {
        auto codec = &(desc->Codecs[c]);
        for (mfxU32 p = 0; p < codec->NumProfiles; p++) {
            auto profile = &(codec->Profiles[p]);
            for (mfxU32 m = 0; m < profile->NumMemTypes; m++) {
                auto memdesc = &(profile->MemDesc[m]);
                FreeDescObject<mfxU32>(memdesc->ColorFormats);
            }
            FreeDescObject(profile->MemDesc);
        }
        FreeDescObject(codec->Profiles);
    }
    FreeDescObject(desc->Codecs);

    memset(desc, 0, sizeof(Desc));
}

// walk through implDesc and delete dynamically-allocated structs
mfxStatus MFXReleaseImplDescription(mfxHDL hdl) 
{
    ImplDescriptionArray* implDescArray = (ImplDescriptionArray*)hdl;
    if (!implDescArray) {
        return MFX_ERR_NULL_PTR;
    }

    mfxImplDescription* implDesc = &(implDescArray->implDesc);
    if (!implDesc) {
        return MFX_ERR_NULL_PTR;
    }

    FreeCaps<mfxDecoderDescription>(&implDesc->Dec);
    FreeCaps<mfxEncoderDescription>(&implDesc->Enc);
    memset(implDesc, 0, sizeof(mfxImplDescription));

    // remove description from the array of handles (set to null)
    // check if this was the last description to be freed - if so,
    //   delete the array of handles
    mfxHDL* hImpls = implDescArray->basePtr;
    mfxU32 currImpl = implDescArray->currImpl;
    mfxU32 numImpl = implDescArray->numImpl;

    hImpls[currImpl] = nullptr;
    delete implDescArray;

    mfxU32 idx;
    for (idx = 0; idx < numImpl; idx++) {
        if (hImpls[idx])
            break;
    }

    if (idx == numImpl)
        delete[] hImpls;

    return MFX_ERR_NONE;
}

mfxStatus MFXMemory_GetSurfaceForVPP(mfxSession session, mfxFrameSurface1** surface)
{
    return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXMemory_GetSurfaceForEncode(mfxSession session, mfxFrameSurface1** surface)
{
    return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXMemory_GetSurfaceForDecode(mfxSession session, mfxFrameSurface1** surface)
{
    return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXInitExInternal(mfxInitParam par, mfxSession* session);
mfxStatus MFXInitialize(mfxInitializationParam parVPL, mfxSession *session)
{
    mfxInitParam par = {};

    par.Implementation = MFX_IMPL_HARDWARE;
    par.Version.Major = MFX_VERSION_MAJOR;
    par.Version.Minor = MFX_VERSION_MINOR;
    par.ExternalThreads = 0;
   
    return MFXInitExInternal(par, session);
}

#endif // #if defined(MFX_ONEVPL)
