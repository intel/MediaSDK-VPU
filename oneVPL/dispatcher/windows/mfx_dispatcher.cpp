/*############################################################################
  # Copyright (C) 2012-2020 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#include "windows/mfx_dispatcher.h"
#include "windows/mfx_dispatcher_log.h"
#include "windows/mfx_load_dll.h"

#include <assert.h>

#include <string.h>
#include <windows.h>

#include <algorithm>
#include "vpl/mfxadapter.h"
#include "windows/mfx_dxva2_device.h"
#include "windows/mfx_vector.h"
#include "windows/mfxvideo++.h"

#pragma warning(disable : 4355)

MFX_DISP_HANDLE::MFX_DISP_HANDLE(const mfxVersion requiredVersion)
        : _mfxSession(),
          apiVersion(requiredVersion) {
    actualApiVersion.Version = 0;
    implType                 = MFX_LIB_SOFTWARE;
    impl                     = MFX_IMPL_SOFTWARE;
    loadStatus               = MFX_ERR_NOT_FOUND;
    dispVersion.Major        = MFX_DISPATCHER_VERSION_MAJOR;
    dispVersion.Minor        = MFX_DISPATCHER_VERSION_MINOR;
    storageID                = 0;
    implInterface            = MFX_IMPL_HARDWARE_ANY;

    hModule = (mfxModuleHandle)0;

} // MFX_DISP_HANDLE::MFX_DISP_HANDLE(const mfxVersion requiredVersion)

MFX_DISP_HANDLE::~MFX_DISP_HANDLE(void) {
    Close();

} // MFX_DISP_HANDLE::~MFX_DISP_HANDLE(void)

mfxStatus MFX_DISP_HANDLE::Close(void) {
    mfxStatus mfxRes;

    mfxRes = UnLoadSelectedDLL();

    // need to reset dispatcher state after unloading dll
    if (MFX_ERR_NONE == mfxRes) {
        implType                         = MFX_LIB_SOFTWARE;
        impl                             = MFX_IMPL_SOFTWARE;
        loadStatus                       = MFX_ERR_NOT_FOUND;
        dispVersion.Major                = MFX_DISPATCHER_VERSION_MAJOR;
        dispVersion.Minor                = MFX_DISPATCHER_VERSION_MINOR;
        *static_cast<_mfxSession*>(this) = _mfxSession();
        hModule                          = (mfxModuleHandle)0;
    }

    return mfxRes;

} // mfxStatus MFX_DISP_HANDLE::Close(void)

mfxStatus MFX_DISP_HANDLE::LoadSelectedDLL(const wchar_t* pPath,
                                           eMfxImplType reqImplType,
                                           mfxIMPL reqImpl,
                                           mfxIMPL reqImplInterface,
                                           mfxInitParam& par) {
    mfxStatus mfxRes = MFX_ERR_NONE;

    // check error(s)
    if ((MFX_LIB_SOFTWARE != reqImplType) && (MFX_LIB_HARDWARE != reqImplType)) {
        DISPATCHER_LOG_ERROR(
            (("implType == %s, should be either MFX_LIB_SOFTWARE ot MFX_LIB_HARDWARE\n"),
             DispatcherLog_GetMFXImplString(reqImplType).c_str()));
        loadStatus = MFX_ERR_ABORTED;
        return loadStatus;
    }
    // only exact types of implementation is allowed
    if (
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        !(reqImpl & MFX_IMPL_EXTERNAL_THREADING) &&
#endif
        (MFX_IMPL_SOFTWARE != reqImpl) && (MFX_IMPL_HARDWARE != reqImpl) &&
        (MFX_IMPL_HARDWARE2 != reqImpl) && (MFX_IMPL_HARDWARE3 != reqImpl) &&
        (MFX_IMPL_HARDWARE4 != reqImpl)) {
        DISPATCHER_LOG_ERROR((("invalid implementation impl == %s\n"),
                              DispatcherLog_GetMFXImplString(impl).c_str()));
        loadStatus = MFX_ERR_ABORTED;
        return loadStatus;
    }
    // only mfxExtThreadsParam is allowed
    if (par.NumExtParam) {
        if ((par.NumExtParam > 1) || !par.ExtParam) {
            loadStatus = MFX_ERR_ABORTED;
            return loadStatus;
        }
        if ((par.ExtParam[0]->BufferId != MFX_EXTBUFF_THREADS_PARAM) ||
            (par.ExtParam[0]->BufferSz != sizeof(mfxExtThreadsParam))) {
            loadStatus = MFX_ERR_ABORTED;
            return loadStatus;
        }
    }

    // close the handle before initialization
    Close();

    // save the library's type
    this->implType      = reqImplType;
    this->impl          = reqImpl;
    this->implInterface = reqImplInterface;

    {
        assert(hModule == (mfxModuleHandle)0);
        DISPATCHER_LOG_BLOCK(("invoking LoadLibrary(%S)\n", pPath));

        // load the DLL into the memory
        hModule = MFX::mfx_dll_load(pPath);

        if (hModule) {
            int i;

            DISPATCHER_LOG_OPERATION({
                wchar_t modulePath[1024];
                GetModuleFileNameW((HMODULE)hModule,
                                   modulePath,
                                   sizeof(modulePath) / sizeof(modulePath[0]));
                DISPATCHER_LOG_INFO((("loaded module %S\n"), modulePath))
            });

            // load video functions: pointers to exposed functions
            for (i = 0; i < eVideoFuncTotal; i += 1) {
                mfxFunctionPointer pProc =
                    (mfxFunctionPointer)MFX::mfx_dll_get_addr(hModule, APIFunc[i].pName);
                if (pProc) {
                    // function exists in the library,
                    // save the pointer.
                    callTable[i] = pProc;
                }
                else {
                    // The library doesn't contain the function
                    DISPATCHER_LOG_WRN((("Can't find API function \"%s\"\n"), APIFunc[i].pName));
                    if (apiVersion.Version >= APIFunc[i].apiVersion.Version) {
                        DISPATCHER_LOG_ERROR((("\"%s\" is required for API %u.%u\n"),
                                              APIFunc[i].pName,
                                              apiVersion.Major,
                                              apiVersion.Minor));
                        mfxRes = MFX_ERR_UNSUPPORTED;
                        break;
                    }
                }
            }

            // if version >= 2.0, load these functions as well
            if (apiVersion.Major >= 2) {
                for (i = 0; i < eVideoFunc2Total; i += 1) {
                    mfxFunctionPointer pProc =
                        (mfxFunctionPointer)MFX::mfx_dll_get_addr(hModule, APIVideoFunc2[i].pName);
                    if (pProc) {
                        // function exists in the library,
                        // save the pointer.
                        callVideoTable2[i] = pProc;
                    }
                    else {
                        // The library doesn't contain the function
                        DISPATCHER_LOG_ERROR((("\"%s\" is required for API %u.%u\n"),
                                              APIVideoFunc2[i].pName,
                                              apiVersion.Major,
                                              apiVersion.Minor));
                        mfxRes = MFX_ERR_UNSUPPORTED;
                        break;
                    }
                }
            }
        }
        else {
            DISPATCHER_LOG_WRN((("can't find DLL: GetLastErr()=0x%x\n"), GetLastError()))
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }

    // initialize the loaded DLL
    if (MFX_ERR_NONE == mfxRes) {
        mfxVersion version(apiVersion);

        if (version.Major >= 2) {
            // for API >= 2.0 call MFXInitialize instead of MFXInitEx
            int tableIndex                  = eMFXInitialize;
            mfxFunctionPointer pFunc        = callVideoTable2[tableIndex];
            mfxInitializationParam initPar2 = {};

            if (reqImplType == MFX_LIB_HARDWARE) {
#ifdef MFX_HW_KMB
                initPar2.AccelerationMode = MFX_ACCEL_MODE_VIA_VAAPI;
#else
                // hardware - D3D11 by default
                if (reqImplInterface == MFX_IMPL_VIA_D3D9)
                    initPar2.AccelerationMode = MFX_ACCEL_MODE_VIA_D3D9;
                else
                    initPar2.AccelerationMode = MFX_ACCEL_MODE_VIA_D3D11;
#endif
            }
            else {
                // software
                initPar2.AccelerationMode = MFX_ACCEL_MODE_NA;
            }

            mfxRes = (*(mfxStatus(MFX_CDECL*)(mfxInitializationParam, mfxSession*))pFunc)(initPar2,
                                                                                          &session);
        }
        else {
            // only support MFXInitEx for 1.x
            int tableIndex           = eMFXInitEx;
            mfxFunctionPointer pFunc = callTable[tableIndex];

            {
                DISPATCHER_LOG_BLOCK(("MFXInitEx(%s,ver=%u.%u,ExtThreads=%d,session=0x%p)\n",
                                      DispatcherLog_GetMFXImplString(impl | implInterface).c_str(),
                                      apiVersion.Major,
                                      apiVersion.Minor,
                                      par.ExternalThreads,
                                      &session));

                mfxInitParam initPar = par;
                // adjusting user parameters
                initPar.Implementation = impl | implInterface;
                initPar.Version        = version;
                mfxRes =
                    (*(mfxStatus(MFX_CDECL*)(mfxInitParam, mfxSession*))pFunc)(initPar, &session);
            }
        }

        if (MFX_ERR_NONE != mfxRes) {
            DISPATCHER_LOG_WRN((("library can't be load. MFXInit returned %s \n"),
                                DispatcherLog_GetMFXStatusString(mfxRes)))
        }
        else {
            mfxRes = MFXQueryVersion((mfxSession)this, &actualApiVersion);

            if (MFX_ERR_NONE != mfxRes) {
                DISPATCHER_LOG_ERROR(
                    (("MFXQueryVersion returned: %d, skiped this library\n"), mfxRes))
            }
            else {
                DISPATCHER_LOG_INFO((("MFXQueryVersion returned API: %d.%d\n"),
                                     actualApiVersion.Major,
                                     actualApiVersion.Minor))
                //special hook for applications that uses sink api to get loaded library path
                DISPATCHER_LOG_LIBRARY(("%p", hModule));
                DISPATCHER_LOG_INFO(("library loaded succesfully\n"))
            }
        }
    }

    loadStatus = mfxRes;
    return mfxRes;

} // mfxStatus MFX_DISP_HANDLE::LoadSelectedDLL(const wchar_t *pPath, eMfxImplType implType, mfxIMPL impl)

mfxStatus MFX_DISP_HANDLE::UnLoadSelectedDLL(void) {
    mfxStatus mfxRes = MFX_ERR_NONE;

    // close the loaded DLL
    if (session) {
        /* check whether it is audio session or video */
        int tableIndex = eMFXClose;
        mfxFunctionPointer pFunc;
        pFunc = callTable[tableIndex];

        mfxRes = (*(mfxStatus(MFX_CDECL*)(mfxSession))pFunc)(session);
        if (MFX_ERR_NONE == mfxRes) {
            session = (mfxSession)0;
        }

        DISPATCHER_LOG_INFO((("MFXClose(0x%x) returned %d\n"), session, mfxRes));
        // actually, the return value is required to pass outside only.
    }

    // it is possible, that there is an active child session.
    // can't unload library in that case.
    if ((MFX_ERR_UNDEFINED_BEHAVIOR != mfxRes) && (hModule)) {
        // unload the library.
        if (!MFX::mfx_dll_free(hModule)) {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        hModule = (mfxModuleHandle)0;
    }

    return mfxRes;

} // mfxStatus MFX_DISP_HANDLE::UnLoadSelectedDLL(void)

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
static mfxStatus InitDummySession(mfxU32 adapter_n, MFXVideoSession& dummy_session) {
    mfxInitParam initPar;
    memset(&initPar, 0, sizeof(initPar));

    initPar.Version.Major = 1;
    initPar.Version.Minor = 0;

    switch (adapter_n) {
        case 0:
            initPar.Implementation = MFX_IMPL_HARDWARE;
            break;
        case 1:
            initPar.Implementation = MFX_IMPL_HARDWARE2;
            break;
        case 2:
            initPar.Implementation = MFX_IMPL_HARDWARE3;
            break;
        case 3:
            initPar.Implementation = MFX_IMPL_HARDWARE4;
            break;

        default:
            // try searching on all display adapters
            initPar.Implementation = MFX_IMPL_HARDWARE_ANY;
            break;
    }

    initPar.Implementation |= MFX_IMPL_VIA_D3D11;

    return dummy_session.InitEx(initPar);
}

static inline bool is_iGPU(const mfxAdapterInfo& adapter_info) {
    return adapter_info.Platform.MediaAdapterType == MFX_MEDIA_INTEGRATED;
}

static inline bool is_dGPU(const mfxAdapterInfo& adapter_info) {
    return adapter_info.Platform.MediaAdapterType == MFX_MEDIA_DISCRETE;
}

// This function implies that iGPU has higher priority
static inline mfxI32 iGPU_priority(const void* ll, const void* rr) {
    const mfxAdapterInfo& l = *(reinterpret_cast<const mfxAdapterInfo*>(ll));
    const mfxAdapterInfo& r = *(reinterpret_cast<const mfxAdapterInfo*>(rr));

    if (is_iGPU(l) && is_iGPU(r) || is_dGPU(l) && is_dGPU(r))
        return 0;

    if (is_iGPU(l) && is_dGPU(r))
        return -1;

    // The only combination left is_dGPU(l) && is_iGPU(r))
    return 1;
}

static void RearrangeInPriorityOrder(const mfxComponentInfo& info,
                                     MFX::MFXVector<mfxAdapterInfo>& vec) {
    {
        // Move iGPU to top priority
        qsort(vec.data(), vec.size(), sizeof(mfxAdapterInfo), &iGPU_priority);
    }
}

static mfxStatus PrepareAdaptersInfo(const mfxComponentInfo* info,
                                     MFX::MFXVector<mfxAdapterInfo>& vec,
                                     mfxAdaptersInfo& adapters) {
    // No suitable adapters on system to handle user's workload
    if (vec.empty()) {
        adapters.NumActual = 0;
        return MFX_ERR_NOT_FOUND;
    }

    if (info) {
        RearrangeInPriorityOrder(*info, vec);
    }

    mfxU32 num_to_copy = (std::min)(mfxU32(vec.size()), adapters.NumAlloc);
    for (mfxU32 i = 0; i < num_to_copy; ++i) {
        adapters.Adapters[i] = vec[i];
    }

    adapters.NumActual = num_to_copy;

    if (vec.size() > adapters.NumAlloc) {
        return MFX_WRN_OUT_OF_RANGE;
    }

    return MFX_ERR_NONE;
}

static inline bool QueryAdapterInfo(mfxU32 adapter_n, mfxU32& VendorID, mfxU32& DeviceID) {
    MFX::DXVA2Device dxvaDevice;

    if (!dxvaDevice.InitD3D9(adapter_n) && !dxvaDevice.InitDXGI1(adapter_n))
        return false;

    VendorID = dxvaDevice.GetVendorID();
    DeviceID = dxvaDevice.GetDeviceID();

    return true;
}

mfxStatus MFXQueryAdaptersDecode(mfxBitstream* bitstream,
                                 mfxU32 codec_id,
                                 mfxAdaptersInfo* adapters) {
    if (!adapters || !bitstream)
        return MFX_ERR_NULL_PTR;

    MFX::MFXVector<mfxAdapterInfo> obtained_info;

    mfxU32 adapter_n = 0, VendorID, DeviceID;

    mfxComponentInfo input_info;
    memset(&input_info, 0, sizeof(input_info));
    input_info.Type                     = mfxComponentType::MFX_COMPONENT_DECODE;
    input_info.Requirements.mfx.CodecId = codec_id;

    for (;;) {
        if (!QueryAdapterInfo(adapter_n, VendorID, DeviceID))
            break;

        ++adapter_n;

        if (VendorID != INTEL_VENDOR_ID)
            continue;

        // Check if requested capabilities are supported
        MFXVideoSession dummy_session;

        mfxStatus sts = InitDummySession(adapter_n - 1, dummy_session);
        if (sts != MFX_ERR_NONE) {
            continue;
        }

        mfxVideoParam stream_params, out;
        memset(&out, 0, sizeof(out));
        memset(&stream_params, 0, sizeof(stream_params));
        out.mfx.CodecId = stream_params.mfx.CodecId = codec_id;

        sts = MFXVideoDECODE_DecodeHeader(dummy_session.operator mfxSession(),
                                          bitstream,
                                          &stream_params);

        if (sts != MFX_ERR_NONE) {
            continue;
        }

        sts = MFXVideoDECODE_Query(dummy_session.operator mfxSession(), &stream_params, &out);

        if (sts !=
            MFX_ERR_NONE) // skip MFX_ERR_UNSUPPORTED as well as MFX_WRN_INCOMPATIBLE_VIDEO_PARAM
            continue;

        mfxAdapterInfo info;
        memset(&info, 0, sizeof(info));
        sts = MFXVideoCORE_QueryPlatform(dummy_session.operator mfxSession(), &info.Platform);

        if (sts != MFX_ERR_NONE) {
            continue;
        }

        //info.Platform.DeviceId = DeviceID;
        info.Number = adapter_n - 1;

        obtained_info.push_back(info);
    }

    return PrepareAdaptersInfo(&input_info, obtained_info, *adapters);
}

mfxStatus MFXQueryAdapters(mfxComponentInfo* input_info, mfxAdaptersInfo* adapters) {
    if (!adapters)
        return MFX_ERR_NULL_PTR;

    MFX::MFXVector<mfxAdapterInfo> obtained_info;
    //obtained_info.reserve(adapters->NumAdaptersAlloc);

    mfxU32 adapter_n = 0, VendorID, DeviceID;

    for (;;) {
        if (!QueryAdapterInfo(adapter_n, VendorID, DeviceID))
            break;

        ++adapter_n;

        if (VendorID != INTEL_VENDOR_ID)
            continue;

        // Check if requested capabilities are supported
        MFXVideoSession dummy_session;

        mfxStatus sts = InitDummySession(adapter_n - 1, dummy_session);
        if (sts != MFX_ERR_NONE) {
            continue;
        }

        // If input_info is NULL just return all Intel adapters and information about them
        if (input_info) {
            mfxVideoParam out;
            memset(&out, 0, sizeof(out));

            switch (input_info->Type) {
                case mfxComponentType::MFX_COMPONENT_ENCODE: {
                    out.mfx.CodecId = input_info->Requirements.mfx.CodecId;

                    sts = MFXVideoENCODE_Query(dummy_session.operator mfxSession(),
                                               &input_info->Requirements,
                                               &out);
                } break;
                case mfxComponentType::MFX_COMPONENT_DECODE: {
                    out.mfx.CodecId = input_info->Requirements.mfx.CodecId;

                    sts = MFXVideoDECODE_Query(dummy_session.operator mfxSession(),
                                               &input_info->Requirements,
                                               &out);
                } break;
                case mfxComponentType::MFX_COMPONENT_VPP: {
                    sts = MFXVideoVPP_Query(dummy_session.operator mfxSession(),
                                            &input_info->Requirements,
                                            &out);
                } break;
                default:
                    sts = MFX_ERR_UNSUPPORTED;
            }
        }

        if (sts !=
            MFX_ERR_NONE) // skip MFX_ERR_UNSUPPORTED as well as MFX_WRN_INCOMPATIBLE_VIDEO_PARAM
            continue;

        mfxAdapterInfo info;
        memset(&info, 0, sizeof(info));
        sts = MFXVideoCORE_QueryPlatform(dummy_session.operator mfxSession(), &info.Platform);

        if (sts != MFX_ERR_NONE) {
            continue;
        }

        //info.Platform.DeviceId = DeviceID;
        info.Number = adapter_n - 1;

        obtained_info.push_back(info);
    }

    return PrepareAdaptersInfo(input_info, obtained_info, *adapters);
}

mfxStatus MFXQueryAdaptersNumber(mfxU32* num_adapters) {
    if (!num_adapters)
        return MFX_ERR_NULL_PTR;

    mfxU32 intel_adapter_count = 0, VendorID, DeviceID;

    for (mfxU32 cur_adapter = 0;; ++cur_adapter) {
        if (!QueryAdapterInfo(cur_adapter, VendorID, DeviceID))
            break;

        if (VendorID == INTEL_VENDOR_ID)
            ++intel_adapter_count;
    }

    *num_adapters = intel_adapter_count;

    return MFX_ERR_NONE;
}

#endif // (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
