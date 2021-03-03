## <a id='Overview'>Overview</a>

**The Intel® Media SDK for Keem Bay*** provide software development tools and libraries needed to develop media solutions either on Intel® Xeon® and Core™ processor-based platforms or directly on Keem Bay card VPU. This SDK implementation is designed for optimizing media applications by utilizing Keem Bay hardware acceleration capabilities.

This document covers product features, system requirements and known limitations. For installation procedures description please see the *&lt;sdk-extract-folder&gt;/mediasdk_kmb_user_guide.pdf*.

Follow aliases will be used in this document: 
- `Host` - IA Windows with Intel® Media SDK.
- `Target` - KeemBay(KMB) accelerator card with Yocto Linux BKC software.

## <a id='whats_new'>What’s New in Media SDK for KMB</a>

**KMB alpha release:**

- Alpha quality support of KeemBay platform.
- Supported encoders: AVC, HEVC, MJPEG.
- Supported decoders: AVC, HEVC, MJPEG.

## <a id='Host_System_Requirements'>Host System Requirements</a>

**Hardware**

- Intel® 8th Generation (Coffee Lake) Xeon® and Core™ processor-based platform.
- Intel® 10th Generation (Comet Lake) Xeon® and Core™ processor-based platform.

**Software**

- Microsoft® Windows® 10 64-bit architecture. 
- Microsoft® Windows® Server 2019 64-bit architecture. 
- Ubuntu 18.04 LTS (Bionic Beaver) x86_64

## <a id='Target_System_Requirements'>Target System Requirements</a>

**Hardware**

- Keembay VPU.

**Software**

- Yocto Linux 64-bit architecture. 

## <a id='Features'>Features</a>
Intel® Media SDK for Keem Bay included in this package implements SDK API 1.34 and contains the following components:

 Component | Supported features | Maximum supported resolution | Minimum supported resolution 
 --- | --- | --- | --- 
H.265 decoder             | Supported Profiles:<ul><li>Main</li><li>Main 10</li></ul>                                          | 4096x2160                    | 144x144                      
H.265 encoder             | Supported Profiles:<ul><li>Main</li><li>Main 10</li></ul>Supported BRC methods:<ul><li>Constant QP (CQP)</li><li>Constant Bit Rate (CBR)</li><li>Variable Bit Rate (VBR)</li><li>Software BRC</li></ul> | 8192x4096 | 136x136 
H.264 decoder             | Supported Profiles:<ul><li>Baseline</li><li>Main</li><li>High</li></ul>            | 4096x2160 | 48x48 
H.264 encoder             | Supported Profiles:<ul><li>Baseline</li><li>Main</li><li>High</li><li>High 10</li></ul>Supported BRC methods:<ul><li>Constant QP (CQP)</li><li>Constant Bit Rate (CBR)</li><li>Variable Bit Rate (VBR)</li><li>Software BRC</li></ul>            | 4096x8192 | 144x144 
MJPEG encoder             | Supported Profiles:<ul><li>Baseline mode, 8bit</li></ul>                     | 16384x16384                  | 32x32                        
MJPEG decoder             | Supported Profiles:<ul><li>Baseline mode, 8bit</li></ul>                   | 16384x16384 | 48x48 



## <a id='Package_Contents'>Package Contents</a>

 For package information please see the *&lt;sdk-extract-folder&gt;/mediasdk-user-guide.pdf*.

## <a id='Documentation'>Documentation</a>

You can find more information on how to use Intel® Media SDK in the following documentation:

* *&lt;sdk-install-dir&gt;/doc/pdf/mediasdk-man.pdf*
“Intel Media SDK Reference Manual” describes the Intel Media SDK API.


* *&lt;sdk-install-dir&gt;/doc/pdf/mediasdkusr-man.pdf*
“Intel Media SDK Extensions for User-Defined Functions” describes an API extension (aka plug-ins API) that allows seamless integration of user-defined functions in SDK pipelines.

* *&lt;sdk-install-dir&gt;/doc/pdf/mediasdkjpeg-man.pdf*
“Intel® Media SDK Reference Manual for JPEG\*/Motion JPEG” describes SDK API for JPEG* processing.

## <a id='Known_Limitations'>Known Limitations</a>

This release is subject to the following known limitations:
* **General**
    - Software fallbacks are no longer supported.
    - VPP is not supported.  Colourspace conversion is not supported as well. Source frame should be converted by an application to a colour format directly supported by hardware implementation.

- **API**:

    - Please use *Query* functions to check feature availability on any given machine at runtime. Availability of features depends on hardware capabilities as well as driver version.

- **Performance**:
    - For current release performance instability may be observed.

- **HEVC decode:**

    - 

- **HEVC encode:**
    - FEI is not supported.
    - GOP structure limitations: max number of references is 2, max distance between two reference frames is 8. B-ref pyramid is not supported.
    - ICQ, VCM, QVBR  BRC methods aren’t supported. LookAhead BRC modes aren’t supported.

- **H.264 decode:**
    - Interlace is not supported. Output video frames will be corrupted in case of fields in the stream.

- **H.264 encode:**

    * Interlace is not supported. 
    * FEI is not supported.
    * MFE is not supported.
    * GOP structure limitations: max number of references is 2, max distance between two reference frames is 8. B-ref pyramid is not supported.

    * There is only one quality level. Regardless Target Usage option set, behavior will be the same.
    * ICQ, VCM, QVBR  BRC methods aren’t supported. LookAhead BRC modes aren’t supported.

- **JPEG/MJPEG decode and encode** support only the below feature set:

    - Baseline mode only
        - DCT based
        - 8 - bit samples
        - sequential
        - loadable 2 AC and 2 DC Huffman tables
        - 2 loadable quantization matrixes
    - No extended, lossless and hierarchical modes
        - no 12-bit samples
        - no progressive
        - no arithmetic coding
        - no 4 AC and 4 DC Huffman tables
    - Decoder doesn't support multi-scan pictures.
    - Encoder doesn't support non-interleaved scans.

- **Reliability:**

    - Current release hasn't been validated on corrupted streams. Instability may be observed.

- **Hardware Device Error Handling**

    - Application should treat *MFX_ERR_ABORTED* status returned from *MFXVideoCORE_SyncOperation()* as *MFX_ERR_DEVICE_FAILED* and run recovery procedure as described in Hardware Device Error Handling section of the SDK manual.

- **Misc**

    - Due to specifics of GPU Copy implementation it is required to close/destroy SDK associated resources (including VADisplay and frame surfaces) only after MFXClose call.
    - Encode quality may be different (non-bit exact) between Hardware generations.

## <a id='GPU_Hang_Reporting_And_Recovery'>GPU Hang Reporting And Recovery</a>

Intel® Media SDK supports reporting of GPU hangs occurred during SDK operations. This feature allows application to establish proper GPU hang recovery procedure without the need for additional monitoring of the system.

**GPU hang reporting procedure:** if GPU hang occurred during HW operation, SDK returns status *MFX_ERR_GPU_HANG* from any *SyncOperation()* call which synchronizes SDK workload affected by hang. In addition SDK rejects to accept new frames and returns *MFX_ERR_GPU_HANG* status from any subsequent call of *EncodeFrameAsync()*, *DecodeFrameAsync()*, *RunFrameVPPAsync()*. It’s available for H.264, H.265 Video decoders and encoders.

**GPU hang recovery procedure:** it is recommended to process *MFX_ERR_DEVICE_FAILED*, *MFX_ERR_GPU_HANG* and *MFX_ERR_ABORTED* uniformly using the full reset procedure described in “Hardware Device Error Handling” of SDK manual. (I.e. recreate all resources: acceleration device, frames memory, SDK sessions, SDK components).

Informative: usually it takes SDK a few seconds to detect and report GPU hang. During this time all *SyncOperation()* calls for tasks affected by GPU hang will return status *MFX_WRN_IN_EXECUTION*. SDK will report the hang with status *MFX_ERR_GPU_HANG* only after GPU hang is detected and HW recovery mechanism is started by driver.
