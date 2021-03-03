// Copyright (c) 2017 Intel Corporation
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

#ifndef __VAAPI_EXT_INTERFACE_H__
#define __VAAPI_EXT_INTERFACE_H__

#include "mfx_config.h"



#define VA_CODED_BUF_STATUS_HW_TEAR_DOWN 0x4000



#if defined(MFX_HW_KMB_TARGET) || defined(MFX_VA_LINUX)

/** RG16: packed 5/6-bit RGB.
 *
 * Each pixel is a two-byte little-endian value.
 * Red, green and blue are found in bits 15:11, 10:5, 4:0 respectively.
 */
#define VA_FOURCC_RGB565        0x36314752

 /** Y210: packed 10-bit YUV 4:2:2.
  *
  * Eight bytes represent a pair of pixels.  Each sample is a two-byte little-endian value,
  * with the bottom six bits ignored.  The samples are in the order Y, U, Y, V.
  */
#define VA_FOURCC_Y210          0x30313259

  /** Y410: packed 10-bit YUVA 4:4:4.
   *
   * Each pixel is a four-byte little-endian value.
   * A, V, Y, U are found in bits 31:30, 29:20, 19:10, 9:0 respectively.
   */
#define VA_FOURCC_Y410          0x30313459

   /** Y216: packed 16-bit YUV 4:2:2.
    *
    * Eight bytes represent a pair of pixels.  Each sample is a two-byte little-endian value.
    * The samples are in the order Y, U, Y, V.
    */
#define VA_FOURCC_Y216          0x36313259
    /** Y416: packed 16-bit YUVA 4:4:4.
     *
     * Each pixel is a set of four samples, each of which is a two-byte little-endian value.
     * The samples are in the order A, V, Y, U.
     */
#define VA_FOURCC_Y416          0x36313459
     /**
      * 10-bit Pixel RGB formats.
      */
#define VA_FOURCC_A2R10G10B10   0x30335241 /* VA_FOURCC('A','R','3','0') */

/** \brief Average VBR
 *  Average variable bitrate control algorithm focuses on overall encoding
 *  quality while meeting the specified target bitrate, within the accuracy
 *  range, after a convergence period.
 *  bits_per_second in VAEncMiscParameterRateControl is target bitrate for AVBR.
 *  Convergence is specified in the unit of frame.
 *  window_size in VAEncMiscParameterRateControl is equal to convergence for AVBR.
 *  Accuracy is in the range of [1,100], 1 means one percent, and so on. 
 *  target_percentage in VAEncMiscParameterRateControl is equal to accuracy for AVBR. */
#define VA_RC_AVBR                      0x00000800

#define VA_RT_FORMAT_YUV420_10	0x00000100	///< YUV 4:2:0 10-bit.
#define VA_RT_FORMAT_YUV422_10	0x00000200	///< YUV 4:2:2 10-bit.
#define VA_RT_FORMAT_YUV444_10	0x00000400	///< YUV 4:4:4 10-bit.
#define VA_RT_FORMAT_YUV420_12	0x00001000	///< YUV 4:2:0 12-bit.
#define VA_RT_FORMAT_YUV422_12	0x00002000	///< YUV 4:2:2 12-bit.
#define VA_RT_FORMAT_YUV444_12	0x00004000	///< YUV 4:4:4 12-bit.

#endif
#endif // __VAAPI_EXT_INTERFACE_H__
