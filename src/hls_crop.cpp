#include "hls_crop.h"
#include "hls_math.h"

template<typename FP_T_,int MAX_COLS_,int MAX_ROWS_>
void calcGeoMatrix(
  FP_T_ Angle,FP_T_ Scale,
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  FP_T_ Mat[2][3]
){
#pragma HLS INLINE
#pragma HLS ALLOCATION operation instances=fmul limit=1
#pragma HLS ALLOCATION operation instances=hmul limit=1
#pragma HLS ALLOCATION operation instances=hsub limit=1
#pragma HLS ALLOCATION operation instances=hdiv limit=1

  const auto Mypi_ {FP_T_ {3.141592653589793238462f}};
  //const auto Alpha_ {FP_T_ {Scale*hls::cosf(Angle*Mypi_/FP_T_ {180})}};
  const auto Alpha_ {FP_T_ {Scale*hls::half_cos(Angle*Mypi_/FP_T_ {180})}};
  //const auto Beta_ {FP_T_ {Scale*hls::sinf(Angle*Mypi_/FP_T_ {180})}};
  const auto Beta_ {FP_T_ {Scale*hls::half_sin(Angle*Mypi_/FP_T_ {180})}};

  Mat[0][0]=Alpha_;
  Mat[0][1]=-Beta_;
  Mat[0][2]=(FP_T_(1)-Alpha_)*(Width/2)+Beta_*(Height/2);
  Mat[1][0]=Beta_;
  Mat[1][1]=Alpha_;
  Mat[1][2]=(FP_T_(1)-Alpha_)*(Height/2)-Beta_*(Width/2);
}

template<typename FP_T_,int MAX_STRIDE_,int MAX_COLS_,int MAX_ROWS_,int DEPTH_,int PPC_>
void rotateFrame 
(
  ap_uint<2*DEPTH_*PPC_> S2mm[(MAX_STRIDE_/PPC_)*MAX_ROWS_],
  ap_uint<2*DEPTH_*PPC_> Mm2s[(MAX_STRIDE_/PPC_)*MAX_ROWS_],
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  FP_T_ Mat[2][3]
){
#pragma HLS INLINE

  loopRows: for(auto Y_=0;Y_<Height;++Y_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_ max=MAX_ROWS_

    loopCols: for(auto X_=0;X_<Width/PPC_;++X_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/PPC_ max=MAX_COLS_/PPC_
#pragma HLS PIPELINE II=PPC_

      FP_T_ Mm2s_X_[PPC_];
      FP_T_ Mm2s_Y_[PPC_];
      loopCalcXY: for(auto J=0;J<PPC_;++J){
#pragma HLS UNROLL
        Mm2s_X_[J]=Mat[0][0]*(X_*PPC_+J)+Mat[0][1]*Y_+Mat[0][2];
        Mm2s_Y_[J]=Mat[1][0]*(X_*PPC_+J)+Mat[1][1]*Y_+Mat[1][2];
      }
 
      ap_uint<2*DEPTH_*PPC_> Mm2s_;
      loopReadSrc: for(auto J=0;J<PPC_;++J){
#pragma HLS UNROLL
        if(Mm2s_X_[J]>=FP_T_(0)&&Mm2s_X_[J]<FP_T_(Width)&&Mm2s_Y_[J]>=FP_T_(0)&&Mm2s_Y_[J]<FP_T_(Height)){
          const auto S2mm_ {S2mm[(MAX_STRIDE_/PPC_)*ap_uint<Bit_Width<MAX_ROWS_>::Value> (Mm2s_Y_[J])+ap_uint<Bit_Width<MAX_COLS_>::Value> (Mm2s_X_[J])]};
          Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J);
        } else {
          Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=0;
        }
      }
  
      Mm2s[(MAX_STRIDE_/PPC_)*Y_+X_]=Mm2s_;
    }
  }
}

void D_TOP_
(
  ap_uint<2*D_DEPTH_*D_PPC_> S2mm[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<2*D_DEPTH_*D_PPC_> Mm2s[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  D_FP_T_ Angle,
  D_FP_T_ Scale
){
#pragma HLS INTERFACE m_axi port=S2mm bundle=myaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_
#pragma HLS INTERFACE m_axi port=Mm2s bundle=myaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_

#pragma HLS INTERFACE s_axilite bundle=ctrl port=return
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Width offset=0x10
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Height offset=0x18
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Angle offset=0x20
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Scale offset=0x28
#pragma HLS INTERFACE s_axilite bundle=ctrl port=S2mm offset=0x30
#pragma HLS INTERFACE s_axilite bundle=ctrl port=Mm2s offset=0x38

#pragma HLS STABLE variable=Width
#pragma HLS STABLE variable=Height
#pragma HLS STABLE variable=Angle
#pragma HLS STABLE variable=Scale

  D_FP_T_ Mat_[2][3];
  const auto Width_ {Width};
  const auto Height_ {Height};
  const auto Angle_ {Angle};
  const auto Scale_ {Scale};

  calcGeoMatrix<D_FP_T_,D_MAX_COLS_,D_MAX_ROWS_>(
    Angle_,Scale_,Width_,Height_,Mat_
  );
  rotateFrame<D_FP_T_,D_MAX_STRIDE_,D_MAX_COLS_,D_MAX_ROWS_,D_DEPTH_,D_PPC_>(
    S2mm,Mm2s,Width_,Height_,Mat_
  );
}
