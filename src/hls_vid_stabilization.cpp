#include "hls_vid_stabilization.h"

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> S2mm[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Mm2s[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Shift_Y,
  ap_uint<Type_Width<D_FP_T_>::Value> Alpha,
  ap_uint<Type_Width<D_FP_T_>::Value> Beta,
  ap_uint<Type_Width<D_FP_T_>::Value> Center_X,
  ap_uint<Type_Width<D_FP_T_>::Value> Center_Y
){
#pragma HLS INTERFACE m_axi port=S2mm bundle=myaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_
#pragma HLS INTERFACE m_axi port=Mm2s bundle=myaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_

#pragma HLS INTERFACE s_axilite bundle=ctrl port=return
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Width offset=0x10
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Height offset=0x18
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Shift_Y offset=0x20
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Alpha offset=0x28
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Beta offset=0x30
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Center_X offset=0x38
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Center_Y offset=0x40
#pragma HLS INTERFACE s_axilite bundle=ctrl port=S2mm offset=0x48
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Mm2s offset=0x50

#pragma HLS STABLE variable=Width
#pragma HLS STABLE variable=Height
#pragma HLS STABLE variable=Shift_Y
#pragma HLS STABLE variable=Alpha
#pragma HLS STABLE variable=Beta
#pragma HLS STABLE variable=Center_X
#pragma HLS STABLE variable=Center_Y

  D_FP_T_ Mat_[2][3];
  const auto Width_ {Width};
  const auto Height_ {Height};
  const auto Alpha_ {Alpha};
  const auto Beta_ {Beta};
  const auto Center_X_ {Center_X};
  const auto Center_Y_ {Center_Y};
  const auto Shift_Y_ {Shift_Y};

  calcGeoMatrix<D_FP_T_,D_MAX_COLS_,D_MAX_ROWS_>(
    Alpha_,Beta_,Center_X_,Center_Y_,Width_,Height_,Shift_Y_,Mat_
  );
  rotateFrame<D_FP_T_,D_MAX_STRIDE_,D_MAX_COLS_,D_MAX_ROWS_,D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>(
    S2mm,Mm2s,Width_,Height_,Mat_
  );
}
