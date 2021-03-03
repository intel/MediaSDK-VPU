export LD_LIBRARY_PATH=../lib:${LD_LIBRARY_PATH}

# avc decoder
./sample_decode h264 -i420 -i ../content/test_stream.264 -o out.yuv

# hevc decoder
./sample_decode h265 -i420 -i ../content/test_stream.265 -o out.yuv

# jpeg decoder
./sample_decode jpeg -i420 -i ../content/test_stream.mjpeg -o out.yuv

# avc encoder
./sample_encode h264 -hw -i ../content/test_stream_352x288.yuv -w 352 -h 288 -o avc.bin -b 10000 -f 30 -u 4

# hevc encoder
./sample_encode h265 -hw -i ../content/test_stream_352x288.yuv -w 352 -h 288 -o hevc.bin -b 10000 -f 30 -u 4

# jpeg encoder
./sample_encode jpeg -hw -i ../content/test_stream_352x288.yuv -w 352 -h 288 -o mjpeg.bin -q 80