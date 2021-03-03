# <a id='Introduction'>Introduction</a>

This guide describes necessary steps to enable KMB VPU backend in media sdk pipelines. Follow aliases will be used in this guide: 
- `Host` - IA Windows, IA Linux with Intel® Media SDK.
- `Target` - KeemBay(KMB) accelerator card with Yocto Linux BKC software.

# <a id='Package_contents'>Package contents</a>

MediaSDK package contains following directories and files:

**IA Windows package**

- `bin` - Host libraries, samples and example script.
- `content` - Sample video streams.
- `sources` - All needed source code to develop custom applications on top of the Media SDK: API headers, Dispatcher, Sample applications, LibVA. 
- `doc` - Development and user documentation.
- `lib` - Import libraries.

**IA Linux | Yocto Linux BKC standalone package**

- `bin` - Samples and example script.
- `lib` - Media SDK libraries.
- `content` - Sample video streams.
- `sources` - Sample applications sources. 
- `doc` - Development and user documentation.
- `include` - API headers.

# <a id='Windows_Setup'>IA Windows host configuration</a>
## <a id='System_Requirements'>System Requirements</a>

`XLink driver` - Make sure driver is installed on the Host, and Target is discoverable in device manager.
`HDDL Unite` - HDDL service provide necessary functionality to communicate between Host and Target.

## <a id='Conn_configuration_steps'>Configuration</a>

**Target configuration**

- Login onto target. There are several ways to do it. Either using serial port via UART or using SSH network connection.
    Connection parameters for Putty via serial port:
    - Connection type: *Serial*
    - Serial line: *COM3*
    - Speed: *115200*
<br>
- Init VPU KMD, UMD and run HDDLUnite service:

    ```
    # enable video kernel mode driver
    /etc/intel-hddlunite/env.sh
    /bin/bash /opt/intel/hddlunite/initdevice.sh
    /sbin/modprobe vpumgr
    /bin/bash -c "/bin/echo $VPU_FIRMWARE_FILE > /sys/devices/platform/soc/soc\:vpusmm/fwname"
  
    # enable video user mode driver
    source /opt/intel/codec-unify/unify.sh

    # configure HDDLUnite
    source /opt/intel/hddlunite/env_B0.sh
    ulimit -n 65532

    # run HDDLUnite
    /opt/intel/hddlunite/bin/hddl_device_service&
    ```

**Host  configuration**

- Locate HDDL installation directory, then open command line and set environment variable KMB_INSTALL_DIR pointed on it: <br>`set KMB_INSTALL_DIR=<hddl_directory>` (Or enable it on system level).
- cd in `KMB_INSTALL_DIR\bin` directory and run `hddl_scheduler_service.exe`
- Open command line, cd to `KMB_INSTALL_DIR\bin` directory and set HDDL service mode: `SetHDDLMode.exe -m b`

After the steps above VPU driver will be configured and connection will be established between Host and Target. Please refer to [troubleshooting](#Healthy_check) to check if driver is configured correctly.

**MediaSDK configuration**

- Locate MediaSDK installation directory, open command line and add directory `<MediaSDK-package>\bin` to PATH environment variable (Or enable it on system level).

Now everything is set and ready to use. Please read the instructions in the [Running the samples](#Run_Samples) section about how-to run real encoding/decoding.

**Miscellaneous: How to permanently enable environment variable on system level**
```
    Right click 'My Computer' and select 'Properties'.
    Click 'Advanced System Settings'.
    Click 'Advanced' tab.
    Click 'Environment Variables...'.
    Click 'New...'
    Type variable and it's value.  
```

# <a id='Linux_Setup'>IA Linux host configuration</a>
## <a id='System_Requirements'>System Requirements</a>

`XLink driver` - Make sure driver is installed on the Host, and the target is initialized on Linux host boot.
`HDDL Unite` - HDDL service provide necessary functionality to communicate between Host and Target.

## <a id='Conn_configuration_steps'>Configuration</a>

**Target configuration**

- Login onto target. There are several ways to do it. Either using serial port via UART or using SSH network connection.

    Example command to connect via serial port:
    ```
    sudo picocom /dev/ttyUSB0 -b 115200
    ```

- Init VPU KMD, UMD and run HDDLUnite service:

    ```
    # enable video kernel mode driver
    /etc/intel-hddlunite/env.sh
    /bin/bash /opt/intel/hddlunite/initdevice.sh
    /sbin/modprobe vpumgr
    /bin/bash -c "/bin/echo $VPU_FIRMWARE_FILE > /sys/devices/platform/soc/soc\:vpusmm/fwname"
  
    # enable video user mode driver
    source /opt/intel/codec-unify/unify.sh

    # configure HDDLUnite
    source /opt/intel/hddlunite/env_B0.sh
    ulimit -n 65532

    # run HDDLUnite
    /opt/intel/hddlunite/bin/hddl_device_service&
    ```

**Host configuration**
- cd to hddlunite directory and setup service

    ```
    cd <hddl-install-dir>

    #run HDDL service
    source ./env_host.sh
    cd ./bin
    ./hddl_scheduler_service&

    #set service mode
    ./SetHDDLMode.exe -m b`
    ```

After the steps abouve VPU driver will be configured and connection will be established between Host and Target.

Please refer to [troubleshooting](#Healthy_check) to check if driver is configured correctly.

**MediaSDK configuration**

Add mediasdk lib directory to the dynamic shared libraries search path and bin directory to the executable files search path
```
export LD_LIBRARY_PATH=<MediaSDK-package>/lib:${LD_LIBRARY_PATH}
export PATH=<MediaSDK-package>/bin:${PATH}
```

Now everything is set and ready to use. Please read the instructions in the [Running the samples](#Run_Samples) section about how-to run real encoding/decoding.

# <a id='Yocto_Linux_Setup'>Yocto Linux standalone configuration</a>

- Init VPU KMD, UMD:

    ```
    # enable video kernel mode driver
    /etc/intel-hddlunite/env.sh
    /bin/bash /opt/intel/hddlunite/initdevice.sh
    /sbin/modprobe vpumgr
    /bin/bash -c "/bin/echo $VPU_FIRMWARE_FILE > /sys/devices/platform/soc/soc\:vpusmm/fwname"
  
    # enable video user mode driver
    source /opt/intel/codec-unify/unify.sh
    ```
    
- Check that driver is enabled and functional. See [troubleshooting](#Healthy_check) for details.
- Extract the MediaSDK package on Target.
- Add mediasdk lib directory to the dynamic shared libraries search path and bin directory to the executable files search path

    ```
    export LD_LIBRARY_PATH=<MediaSDK-package>/lib:${LD_LIBRARY_PATH}
    export PATH=<MediaSDK-package>/bin:${PATH}
    ```
    
Now everything is set and ready to use. Please read the instructions in the [Running the samples](#Run_Samples) section about how-to run real encoding/decoding.


# <a id='Run_Samples'>Running the samples</a> 

Samples are located in `bin` directory along with `run_examples` script which demonstrates vairous sample usage scenarios.

- Decoding Sample demonstrates how to use the SDK API to create a sample console application that performs decoding of various video compression formats. Below is the example of a command-line to execute decoding sample:

```C
	sample_decode h264 -i input.h264 -o output.yuv -hw
```

- Encoding Sample demonstrates how to use the SDK API to create a simple console application that performs encoding of an uncompressed video stream according to a specific video compression standard. Below is the example of a command-line to execute encoding sample:

```C
	sample_encode h264 -i input.yuv -o output.h264 -w 720 -h 480 -b 10000 -f 30 -u 4 -d3d -hw
```

- Transcoding Sample demonstrates how to use SDK API to create a console application that performs the transcoding (decoding and encoding) of a video stream from one compressed video format to another, with optional data transfer of uncompressed video. Below is the example of a command-line to execute transcoding sample:

```C
	sample_multi_transcode.exe -i::h264 input.264 -o::h265 out.h265 -w 480 -h 320 
```

# Troubleshooting

## <a id='Healthy_check'>VPU driver healthy check
- Type `vainfo` command. It should produce something like:

```
    error: can't connect to X server!
    libva info: VA-API version 1.1.0
    libva info: va_getDriverName() returns 0
    libva info: User requested driver 'hantro'
    libva info: Trying to open /home/root/hantro_drv_video.so
    libva info: Found init function __vaDriverInit_1_1
    libva info: va_openDriver() returns 0
    vainfo: VA-API version: 1.1 (libva 2.1.0)
    vainfo: Driver version: Vsi hantro driver for VSI Hantro Encoder VC8000E V6.2 /Decoder VC8000D  - 0.0.0
    vainfo: Supported profile and entrypoints
        VAProfileH264ConstrainedBaseline: VAEntrypointVLD
        VAProfileH264ConstrainedBaseline: VAEntrypointEncSlice
        VAProfileH264Main                          : VAEntrypointVLD
        VAProfileH264Main                          : VAEntrypointEncSlice
        VAProfileH264High                          : VAEntrypointVLD
```
<br>
If `vainfo` produces errors instead of list of supported codecs check that kernel and user mode driver are enabled.
