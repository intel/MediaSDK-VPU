DISCONTINUATION OF PROJECT

This project will no longer be maintained by Intel.

Intel has ceased development and contributions including, but not limited to, maintenance, bug fixes, new releases, or updates, to this project.  

Intel no longer accepts patches to this project.

If you have an ongoing need to use this project, are interested in independently developing it, or would like to maintain patches for the open source software community, please create your own fork of this project.  

Contact: webadmin@linux.intel.com
# Intel® Media SDK
Intel® Media SDK for Intel Vision Processing Units (VPUs)  provides a plain C API to access hardware-accelerated video decode, encode and filtering on VPU graphics hardware platforms. Implementation written in C++ 11.

**Supported VPUs**: KeemBay<br>
**Supported video encoders**: HEVC, AVC, JPEG<br>
**Supported video decoders**: HEVC, AVC, JPEG

# Dependencies
* [LibVA](https://github.com/01org/libva/) - an implementation for VA-API (Video Acceleration API).
* [VAAPI Bypass](https://github.com/intel/vaapi-bypass/) - suite of components with an IA host libVA backend and Intel VPU remote proxy application.

# Table of contents

  * [License](#license)
  * [Documentation](#documentation)
  * [Tutorials](#tutorials)
  * [Products which use Media SDK](#products-which-use-media-sdk)
  * [System requirements](#system-requirements)
  * [How to build](#how-to-build)
    * [Build steps](#build-steps)
    * [Enabling Instrumentation and Tracing Technology (ITT)](#enabling-instrumentation-and-tracing-technology-itt)
  * [Recommendations](#recommendations)
  * [See also](#see-also)

# License
Intel Media SDK is licensed under MIT license. See [LICENSE](./LICENSE) for details.

# Documentation

To get copy of Media SDK documentation use Git* with [LFS](https://git-lfs.github.com/) support.

Please find full documentation under the [./doc](./doc) folder. Key documents:
* [Media SDK User Guide](./doc/mediasdk-user-guide.md)
* [Media SDK Manual](./doc/mediasdk-man.md)
* Additional Per-Codec Manuals:
  * [Media SDK JPEG Manual](./doc/mediasdkjpeg-man.md)

Generic samples information is available in [Media Samples Guide](./doc/samples/Media_Samples_Guide_Linux.md)

Linux Samples Readme Documents:
* [Sample Multi Transcode](./doc/samples/readme-multi-transcode_linux.md)
* [Sample Decode](./doc/samples/readme-decode_linux.md)
* [Sample Encode](./doc/samples/readme-encode_linux.md)

Visit our [Github Wiki](https://github.com/intel/MediaSDK-VPU/wiki) for the detailed setting and building instructions, runtime tips and other information.

# Tutorials

* [Tutorials Overview](./doc/tutorials/mediasdk-tutorials-readme.md)
* [Tutorials Command Line Reference](./doc/tutorials/mediasdk-tutorials-cmd-reference.md)

# Products which use Media SDK

Use Media SDK via popular frameworks:
* [FFmpeg](http://ffmpeg.org/) via [ffmpeg-qsv](https://trac.ffmpeg.org/wiki/Hardware/QuickSync) plugins
* [GStreamer](https://gstreamer.freedesktop.org/) via plugins set included
  into [gst-plugins-bad](https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad)

Learn best practises and borrow fragments for final solutions:
* https://github.com/intel/media-delivery
  * This collection of samples demonstrates best practices to achieve optimal video quality and
    performance for content delivery networks. Check out the demo, recommended command
    lines and quality and performance measuring tools.

Get customized Media SDK products:
* [Intel Media SDK for GEN graphics](https://github.com/Intel-Media-SDK/MediaSDK)
* [Intel Media Server Studio](https://software.intel.com/en-us/intel-media-server-studio)
* [Intel Media SDK for Embedded Linux](https://software.intel.com/en-us/media-sdk/choose-download/embedded-iot)


# Host System Requirements

**Hardware**
* Intel® 8th Generation (Coffee Lake) Xeon® and Core™ processor-based platform
* Intel® 10th Generation (Comet Lake) Xeon® and Core™ processor-based platform

**Software**
* Microsoft® Windows® 10 x86-64
* Microsoft® Windows® Server 2019 x86-64
* Ubuntu 18.04 LTS (Bionic Beaver) x86_64

# Target System Requirements

**Hardware**
* Keembay VPU

**Software**
* Yocto Linux aarch64

# How to build

## Build steps

Get sources with the following Git* command (pay attention that to get full Media SDK sources bundle it is required to have Git* with [LFS](https://git-lfs.github.com/) support):
```sh
git clone https://github.com/intel/MediaSDK-VPU msdk
cd msdk
```

To configure and build Media SDK install cmake version 3.13 or later and run the following commands:
```sh
mkdir build && cd build
cmake ..
make
make install
```
Media SDK depends on a number of packages which are identified and checked for the proper version during configuration stage. Please, make sure to install these packages to satisfy Media SDK requirements. After successful configuration 'make' will build Media SDK binaries and samples. The following cmake configuration options can be used to customize the build:

| Option | Values | Description |
| ------ | ------ | ----------- |
| API | master\|latest\|major.minor | Build mediasdk library with specified API. 'latest' will enable experimental features. 'master' will configure the most recent available published API (default: master). |
| ENABLE_ITT | ON\|OFF | Enable ITT (VTune) instrumentation support (default: OFF) |
| ENABLE_TEXTLOG | ON\|OFF | Enable textlog trace support (default: OFF) |
| ENABLE_STAT | ON\|OFF | Enable stat trace support (default: OFF) |
| BUILD_ALL | ON\|OFF | Build all the BUILD_* targets below (default: OFF) |
| BUILD_RUNTIME | ON\|OFF | Build mediasdk runtime, library and plugins (default: ON) |
| BUILD_SAMPLES | ON\|OFF | Build samples (default: ON) |
| BUILD_TESTS | ON\|OFF | Build unit tests (default: OFF) |
| MFX_VSI_HDDL | ON\|OFF | Enable or disable HDDL support (default: ON) |
| USE_SSH_FOR_BYPASS| ON\|OFF | ON - Use SSH protocol to checkout external project dependencies<br>OFF - HTTPS one (default: OFF) |

The following cmake settings can be used to adjust search path locations for some components Media SDK build may depend on:

| Setting | Values | Description |
| ------- | ------ | ----------- |
| CMAKE_ITT_HOME | Valid system path | Location of ITT installation, takes precendence over CMAKE_VTUNE_HOME (by default not defined) |
| CMAKE_VTUNE_HOME | Valid system path | Location of VTune installation (default: /opt/intel/vtune_amplifier) |

Visit our [Github Wiki](https://github.com/intel/MediaSDK-VPU/wiki) for advanced topics on setting and building Media SDK.

## Enabling Instrumentation and Tracing Technology (ITT)

To enable the Instrumentation and Tracing Technology (ITT) API you need to:
* Either install [Intel® VTune™ Amplifier](https://software.intel.com/en-us/intel-vtune-amplifier-xe)
* Or manually build an open source version (see [IntelSEAPI](https://github.com/01org/IntelSEAPI/tree/master/ittnotify) for details)

and configure Media SDK with the -DENABLE_ITT=ON. In case of VTune it will be searched in the default location (/opt/intel/vtune_amplifier). You can adjust ITT search path with either CMAKE_ITT_HOME or CMAKE_VTUNE_HOME.

Once Media SDK was built with ITT support, enable it in a runtime creating per-user configuration file ($HOME/.mfx_trace) or a system wide configuration file (/etc/mfx_trace) with the following content:
```sh
Output=0x10
```
# Configuration
Please refer to [Media SDK User Guide](./doc/mediasdk-user-guide.md).

# Recommendations

* In case of GCC compiler it is strongly recommended to use GCC version 6 or later since that's the first GCC version which has non-experimental support of C++11 being used in Media SDK.

# See also

Intel Media SDK: https://software.intel.com/en-us/media-sdk
