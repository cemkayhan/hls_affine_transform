#ifndef HLS_AFFINE_TRANSFORM_H_INCLUDE_GUARD_
#define HLS_AFFINE_TRANSFORM_H_INCLUDE_GUARD_

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "axi_vid_bus_width.h"
#include "bit_width.h"
#include "utils/x_hls_utils.h"

void D_TOP_
(
  ap_uint<D_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidWrAxi[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)],
  ap_uint<D_CHANNELS_*D_DEPTH_*D_MM_PPC_> affRdAxi[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)],
  ap_uint<D_CHANNELS_*D_DEPTH_*D_MM_PPC_> affWrAxi[(D_MAX_COLS_/D_MM_PPC_)*(D_MAX_ROWS_)],
  ap_uint<D_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidRdAxi[(D_MAX_COLS_/D_MM_PPC_)*(D_MAX_ROWS_)],

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> >& srcStream,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> >& dstStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height,
  ap_uint<32> rotMat00, ap_uint<32> rotMat01, ap_uint<32> rotMat02,
  ap_uint<32> rotMat10, ap_uint<32> rotMat11, ap_uint<32> rotMat12
);

#endif
