set BYPASS_BATCH_MODE=1

rem avc decoder
sample_decode.exe h264 -i420 -i ../content/test_stream.264 -o out.yuv

rem hevc decoder
sample_decode.exe h265 -i420 -i ../content/test_stream.265 -o out.yuv

rem jpeg decoder
sample_decode.exe jpeg -i420 -i ../content/test_stream.mjpeg -o out.yuv

rem avc encoder
sample_encode h264 -hw -i ../content/test_stream_352x288.yuv -w 352 -h 288 -o avc.bin -b 10000 -f 30 -u 4

rem hevc encoder
sample_encode h265 -hw -i ../content/test_stream_352x288.yuv -w 352 -h 288 -o hevc.bin -b 10000 -f 30 -u 4

rem jpeg encoder
sample_encode jpeg -hw -i ../content/test_stream_352x288.yuv -w 352 -h 288 -o mjpeg.bin -q 80

rem HDDL RemoteMemory export/import 
simple_transcode_hddl.exe -hw -f 30/1 -b 500 ../content/test_stream.264 avc_trans.bin
