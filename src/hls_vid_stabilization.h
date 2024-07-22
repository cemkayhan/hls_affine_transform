#ifndef HLS_VID_STABILIZATION_H_INCLUDE_GUARD_
#define HLS_VID_STABILIZATION_H_INCLUDE_GUARD_

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "axi_vid_bus_width.h"
#include "bit_width.h"
#include "type_width.h"
#include "hls_math.h"
#include "fptfunction.h"

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> srcAxi[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> dstAxi[(D_MAX_STRIDE_/D_MM_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  float rotMat00, float rotMat01, float rotMat02,
  float rotMat10, float rotMat11, float rotMat12
);

#endif
