  
 /** @file apriltag_js.c
 *  @brief Apriltag detection to be compile with emscripten
 * 
 * Uses the apriltaf library; exposes a simple interface for a web app to
 * use apriltags once it is compiled to WASM using emscripten
 *
 *  @author Nuno Pereira; CMU (this file)
 *  @date Nov, 2019
 */

/* Copyright (C) 2013-2016, The Regents of The University of Michigan.
All rights reserved.

This software was developed in the APRIL Robotics Lab under the
direction of Edwin Olson, ebolson@umich.edu. This software may be
available under alternative licensing terms; contact the address above.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the Regents of The University of Michigan.
*/
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include "apriltag.h"
#include "tag36h11.h"
#include "tag25h9.h"
#include "tag16h5.h"
#include "tagCircle21h7.h"
#include "tagStandard41h12.h"
#include "common/getopt.h"
#include "common/image_u8.h"
#include "common/image_u8x4.h"
#include "common/pjpeg.h"
#include "common/zarray.h"
#include "emscripten.h"

// maximum size of string for each detection
#define STR_DET_LEN 350

// json format string for the 4 points
const char fmt_det_point[] = "{\"id\":%d, \"corners\": [{\"x\":%.2f,\"y\":%.2f},{\"x\":%.2f,\"y\":%.2f},{\"x\":%.2f,\"y\":%.2f},{\"x\":%.2f,\"y\":%.2f}], \"center\": {\"x\":%.2f,\"y\":%.2f} }";
const char fmt_det_point_pose[] = "{\"id\":%d, \"corners\": [{\"x\":%.2f,\"y\":%.2f},{\"x\":%.2f,\"y\":%.2f},{\"x\":%.2f,\"y\":%.2f},{\"x\":%.2f,\"y\":%.2f}], \"center\": {\"x\":%.2f,\"y\":%.2f}, \"pose\": { \"R\": [[%.2f,%.2f,%.2f],[%.2f,%.2f,%.2f],[%.2f,%.2f,%.2f]], \"t\": [[%.2f,%.2f,%.2f],[%.2f,%.2f,%.2f],[%.2f,%.2f,%.2f]] } }";

// global pointers to the tag family and detector
apriltag_family_t * g_tf;
apriltag_detector_t * g_td;

// size oand stride f the image to process
int g_width;
int g_height;
int g_stride;

// pointer to the image grayscale pixes
uint8_t * g_img_buf = NULL;

// if we are returning pose (TODO)
int g_bool_return_pose = 0;

/**
 * @brief Init the apriltag detector with given options
 *
 * @param decimate Decimate input image by this factor
 * @param sigma Apply low-pass blur to input; negative sharpens
 * @param nthreads Use this many CPU threads
 * @param refine_edges Spend more time trying to align edges of tags
 * @param return_pose Detect returns pose of detected tags (0=does no return pose; returns pose otherwise)
 * 
 * @return 0=success
 */
EMSCRIPTEN_KEEPALIVE
int init(float decimate, float sigma, int nthreads, int refine_edges, int return_pose) {
  g_tf = tag36h11_create();
  g_td = apriltag_detector_create();
  apriltag_detector_add_family_bits(g_td, g_tf, 1); // "Detect tags with up to this many bit errors.
  g_td-> quad_decimate = decimate; 
  g_td-> quad_sigma = sigma; 
  g_td-> nthreads = nthreads; 
  g_td-> debug = 0; // Enable debugging output (slow)
  g_td-> refine_edges = refine_edges;
  g_bool_return_pose = return_pose; 
  return 0;
}

/**
 * @brief Sets the given options; utilty function to change options after init
 *
 * @param decimate Decimate input image by this factor
 * @param sigma Apply low-pass blur to input; negative sharpens
 * @param nthreads Use this many CPU threads
 * @param refine_edges Spend more time trying to align edges of tags
 * @param return_pose Detect returns pose of detected tags (0=does no return pose; returns pose otherwise)
 * 
 * @return 0=success
 */
EMSCRIPTEN_KEEPALIVE
int set_options(float decimate, float sigma, int nthreads, int refine_edges, int return_pose) {
  g_td-> quad_decimate = decimate; 
  g_td-> quad_sigma = sigma; 
  g_td-> nthreads = nthreads; 
  g_td-> refine_edges = refine_edges; 
  g_bool_return_pose = return_pose; 
  return 0;
}


/**
 * @brief Creates/changes size of the image buffer where we receive the images to process; only returns the pointer if size did not change
 *
 * @param width Width of the image
 * @param height Height of the image
 * @param stride How many pixels per row (=width typically)
 * 
 * @return the pointer to the image buffer 
 *
 * @warning caller of detect is responsible for putting *grayscale* image pixels in this buffer
 */
EMSCRIPTEN_KEEPALIVE
uint8_t * set_img_buffer(int width, int height, int stride) {
  if (g_img_buf != NULL) {
    if (g_width == width && g_height == height) return g_img_buf;
    free(g_img_buf);
    g_width = width;
    g_height = height;
    g_img_buf = (uint8_t * ) malloc(width * height);
  } else {
    g_width = width;
    g_height = height;
    g_stride = stride;
    g_img_buf = (uint8_t * ) malloc(width * height);
  }
  return g_img_buf;
}

/**
 * @brief Releases resources
 *
 * @return 0=success
 */
EMSCRIPTEN_KEEPALIVE
int destroy() {
  apriltag_detector_destroy(g_td);
  tag36h11_destroy(g_tf);
  if (g_img_buf != NULL) free(g_img_buf);
  return 0;
}

/**
 * @brief Detect tags in image stored in the buffer (g_img_buf)
 *
 * @return json string with id, corners, center, and pose of each detected tag; must be release by caller
 *
 * @warning caller is responsible for putting *grayscale* image pixels in the input buffer (g_img_buf)
 * @warning caller must release the returned buffer (json string) by calling destroy_buffer()
 */
EMSCRIPTEN_KEEPALIVE
uint8_t * detect() {
  image_u8_t im = {
    .width = g_width,
    .height = g_height,
    .stride = g_stride,
    .buf = g_img_buf
  };

  zarray_t * detections = apriltag_detector_detect(g_td, & im);

  if (zarray_size(detections) == 0) {
    int * buffer = malloc(sizeof(int));
    buffer[0] = 0;
    return (uint8_t *)buffer;
  }

  // TODO: if pose == 1; get pose ..

  int str_det_len = zarray_size(detections) * STR_DET_LEN;
  int * buffer = malloc(str_det_len + sizeof(int));
  char * str_det = ((char * ) buffer) + sizeof(int);
  char * str_tmp_det = malloc(STR_DET_LEN);
  int llen = str_det_len - 1;
  strcpy(str_det, "[ ");
  llen -= 2; //"[ "
  for (int i = 0; i < zarray_size(detections); i++) {
    apriltag_detection_t * det;
    zarray_get(detections, i, & det);
    int c;
    if (g_bool_return_pose == 0) {
      c = snprintf(str_tmp_det, STR_DET_LEN, fmt_det_point, det->id, det->p[0][0], det->p[0][1], det->p[1][0], det->p[1][1], det->p[2][0], det->p[2][1], det->p[3][0], det->p[3][1], det->c[0], det->c[0]);
    } else {
      // TODO: if pose == 1; return pose ..
      c = snprintf(str_tmp_det, STR_DET_LEN, fmt_det_point_pose, det->id, det->p[0][0], det->p[0][1], det->p[1][0], det->p[1][1], det->p[2][0], det->p[2][1], det->p[3][0], det->p[3][1], det->c[0], det->c[0], 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
    if (i > 0) {
      strncat(str_det, ", ", llen);
      llen -= 2;
      strncat(str_det, str_tmp_det, llen);
      llen -= c;
    } else {
      strncat(str_det, str_tmp_det, llen);
      llen -= c;
    }
  }
  free(str_tmp_det);
  strncat(str_det, " ]", llen);
  str_det[str_det_len - 1] = '\0'; // make sure it is null-terminated

  apriltag_detections_destroy(detections);

  buffer[0] = strlen(str_det);
  return (uint8_t *)buffer;
}

/**
 * @brief Allocates memory for 'bytes_size' bytes
 *
 * @param bytes_size How many bytes to allocate
 * 
 * @return pointer allocated
 */
EMSCRIPTEN_KEEPALIVE
uint8_t * create_buffer(int byte_size) {
  return malloc(byte_size);
}

/**
 * @brief Releases memory previously allocated
 *
 * @param p pointer to buffer
 */
EMSCRIPTEN_KEEPALIVE
void destroy_buffer(uint8_t * p) {
  free(p);
}