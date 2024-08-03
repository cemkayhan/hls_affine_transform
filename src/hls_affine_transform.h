#ifndef HLS_AFFINE_TRANSFORM_H_INCLUDE_GUARD_
#define HLS_AFFINE_TRANSFORM_H_INCLUDE_GUARD_

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "hls_math.h"
#include "axi_vid_bus_width.h"
#include "utils/x_hls_utils.h"

#include "pow2.h"
#include "bit_width.h"
#include "type_width.h"
#include "fpt_func.h"
#include "floor_func.h"

#include <assert.h>

#define D_AFFRDAXI_DEPTH_ ((D_MAX_STRIDE_/D_MM_PPC_)*(2*D_BLOCK_SIZE_+D_MAX_ROWS_+2*D_BLOCK_SIZE_))
#define D_VIDWRAXI_DEPTH_ ((D_MAX_STRIDE_/D_MM_PPC_)*(2*D_BLOCK_SIZE_+D_MAX_ROWS_+2*D_BLOCK_SIZE_))
#define D_AFFWRAXI_DEPTH_ ((D_MAX_STRIDE_/D_MM_PPC_)*(2*D_BLOCK_SIZE_+D_MAX_ROWS_+2*D_BLOCK_SIZE_))
#define D_VIDRDAXI_DEPTH_ ((D_MAX_STRIDE_/D_MM_PPC_)*(2*D_BLOCK_SIZE_+D_MAX_ROWS_+2*D_BLOCK_SIZE_))

void D_TOP_
(
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidWrAxi[D_VIDWRAXI_DEPTH_],
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> affRdAxi[D_AFFRDAXI_DEPTH_],
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> affWrAxi[D_AFFWRAXI_DEPTH_],
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidRdAxi[D_VIDRDAXI_DEPTH_],

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> >& srcStream,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> >& dstStream,

  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height,

  ap_uint<Bit_Width<D_MAX_COLS_>::Value> padWidth,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> padHeight,

  ap_uint<Type_Width<D_FP_T_>::Value> rotMat00, ap_uint<Type_Width<D_FP_T_>::Value> rotMat01, ap_uint<Type_Width<D_FP_T_>::Value> rotMat02,
  ap_uint<Type_Width<D_FP_T_>::Value> rotMat10, ap_uint<Type_Width<D_FP_T_>::Value> rotMat11, ap_uint<Type_Width<D_FP_T_>::Value> rotMat12
);

#endif
