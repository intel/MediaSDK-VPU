// Copyright (c) 2013-2020 Intel Corporation
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

#include "mfx_hevc_dec_plugin.h"
#include "mfx_h265_encode_plugin_hw.h"
#ifndef MFX_HW_KMB
#include "mfx_h265_encode_plugin_hw.h"
#include "mfx_vp8_dec_plugin.h"
#include "mfx_vp9_dec_plugin.h"
#include "mfx_vp9_encode_plugin_hw.h"
#include "mfx_camera_plugin.h"
#endif

#if defined (WIN64) || defined(LINUX64)
#include "mfx_hevc_enc_plugin.h"
#endif //WIN64

MSDK_PLUGIN_API(MFXDecoderPlugin*) mfxCreateDecoderPlugin() {
    return 0;
}

MSDK_PLUGIN_API(mfxStatus) CreatePlugin(mfxPluginUID uid, mfxPlugin* plugin) {

    if(std::memcmp(uid.Data, MFXHEVCDecoderPlugin::g_HEVCDecoderGuid.Data, sizeof(uid.Data)) == 0)
        return MFXHEVCDecoderPlugin::CreateByDispatcher(uid, plugin);
    else if (std::memcmp(uid.Data, MfxHwH265Encode::MFX_PLUGINID_HEVCE_HW.Data, sizeof(uid.Data)) == 0)
        return MfxHwH265Encode::Plugin::CreateByDispatcher(uid, plugin);
#ifndef MFX_HW_KMB
    else if(std::memcmp(uid.Data, MFXVP8DecoderPlugin::g_VP8DecoderGuid.Data, sizeof(uid.Data)) == 0)
        return MFXVP8DecoderPlugin::CreateByDispatcher(uid, plugin);
    else if(std::memcmp(uid.Data, MFXVP9DecoderPlugin::g_VP9DecoderGuid.Data, sizeof(uid.Data)) == 0)
        return MFXVP9DecoderPlugin::CreateByDispatcher(uid, plugin);
    else if(std::memcmp(uid.Data, MFX_PLUGINID_VP9E_HW.Data, sizeof(uid.Data)) == 0)
        return MfxHwVP9Encode::Plugin::CreateByDispatcher(uid, plugin);
    else if(std::memcmp(uid.Data, MFXCamera_Plugin::g_Camera_PluginGuid.Data, sizeof(uid.Data)) == 0)
        return MFXCamera_Plugin::CreateByDispatcher(uid, plugin);
#if (defined (WIN64) || defined(LINUX64)) && defined(AS_HEVCE_PLUGIN)
    else if (std::memcmp(uid.Data, MFXHEVCEncoderPlugin::g_HEVCEncoderGuid.Data, sizeof(uid.Data)) == 0)
        return MFXHEVCEncoderPlugin::CreateByDispatcher(uid, plugin);
#if defined(AS_HEVCE_DP_PLUGIN)
    else if (std::memcmp(uid.Data, MFXHEVCEncoderDPPlugin::g_HEVCEncoderDPGuid.Data, sizeof(uid.Data)) == 0)
        return MFXHEVCEncoderDPPlugin::CreateByDispatcher(uid, plugin);
#endif
#endif
#endif //#if (defined (WIN64) || defined(LINUX64)) && defined(AS_HEVCE_PLUGIN)
    else return MFX_ERR_NOT_FOUND;
}
