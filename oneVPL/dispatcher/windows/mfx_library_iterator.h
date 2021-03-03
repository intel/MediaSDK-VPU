/*############################################################################
  # Copyright (C) 2012-2020 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#ifndef SRC_DISPATCHER_WINDOWS_MFX_LIBRARY_ITERATOR_H_
#define SRC_DISPATCHER_WINDOWS_MFX_LIBRARY_ITERATOR_H_

#include <vpl/mfxvideo.h>

#if !defined(MEDIASDK_UWP_DISPATCHER)
    #include "mfx_win_reg_key.h"
#endif

#include "windows/mfx_dispatcher.h"

namespace MFX {

// declare desired storage ID
enum {
    MFX_UNKNOWN_KEY       = -1,
    MFX_CURRENT_USER_KEY  = 0,
    MFX_LOCAL_MACHINE_KEY = 1,
    MFX_APP_FOLDER        = 2,
    MFX_PATH_MSDK_FOLDER  = 3,
    MFX_STORAGE_ID_FIRST  = MFX_CURRENT_USER_KEY,
    MFX_STORAGE_ID_LAST   = MFX_PATH_MSDK_FOLDER
};

// Try to initialize using given implementation type. Select appropriate type automatically in case of MFX_IMPL_VIA_ANY.
// Params: adapterNum - in, pImplInterface - in/out, pVendorID - out, pDeviceID - out
mfxStatus SelectImplementationType(const mfxU32 adapterNum,
                                   mfxIMPL *pImplInterface,
                                   mfxU32 *pVendorID,
                                   mfxU32 *pDeviceID);

const mfxU32 msdk_disp_path_len = 1024;

class MFXLibraryIterator {
public:
    // Default constructor
    MFXLibraryIterator(void);
    // Destructor
    ~MFXLibraryIterator(void);

    // Initialize the iterator
    mfxStatus Init(eMfxImplType implType,
                   mfxIMPL implInterface,
                   const mfxU32 adapterNum,
                   int storageID);

    // Get the next library path
    mfxStatus SelectDLLVersion(wchar_t *pPath,
                               size_t pathSize,
                               eMfxImplType *pImplType,
                               mfxVersion minVersion);

    // Return interface type on which Intel adapter was found (if any): D3D9 or D3D11
    mfxIMPL GetImplementationType();

    // Retrun registry subkey name on which dll was selected after sucesfull call to selectDllVesion
    bool GetSubKeyName(wchar_t *subKeyName, size_t length) const;

    int GetStorageID() const {
        return m_StorageID;
    }

protected:
    // Release the iterator
    void Release(void);

    // Initialize the registry iterator
    mfxStatus InitRegistry(eMfxImplType implType,
                           mfxIMPL implInterface,
                           const mfxU32 adapterNum,
                           int storageID);
    // Initialize the app/module folder iterator
    mfxStatus InitFolder(eMfxImplType implType,
                         mfxIMPL implInterface,
                         const mfxU32 adapterNum,
                         const wchar_t *path,
                         const int storageID);

    eMfxImplType m_implType; // Required library implementation
    mfxIMPL m_implInterface; // Required interface (D3D9, D3D11)

    mfxU32 m_vendorID; // (mfxU32) property of used graphic card
    mfxU32 m_deviceID; // (mfxU32) property of used graphic card
    bool m_bIsSubKeyValid;
    wchar_t m_SubKeyName[MFX_MAX_REGISTRY_KEY_NAME]; // registry subkey for selected module loaded
    int m_StorageID;

#if !defined(MEDIASDK_UWP_DISPATCHER)
    WinRegKey m_baseRegKey; // (WinRegKey) main registry key
#endif

    mfxU32 m_lastLibIndex; // (mfxU32) index of previously returned library
    mfxU32 m_lastLibMerit; // (mfxU32) merit of previously returned library

    wchar_t m_path[msdk_disp_path_len]; //NOLINT(runtime/arrays)

private:
    // unimplemented by intent to make this class non-copyable
    MFXLibraryIterator(const MFXLibraryIterator &);
    void operator=(const MFXLibraryIterator &);
};

} // namespace MFX

#endif // SRC_DISPATCHER_WINDOWS_MFX_LIBRARY_ITERATOR_H_
