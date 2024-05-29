#ifndef HLS_VID_STABILIZATION_H_INCLUDE_GUARD_
#define HLS_VID_STABILIZATION_H_INCLUDE_GUARD_

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "axi_vid_bus_width.h"
#include "bit_width.h"
#include "type_width.h"
#include "hls_math.h"
#include "sinfunction.h"
#include "cosfunction.h"
#include "fptfunction.h"
#include "pifunction.h"

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> S2mm[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Mm2s[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Shift_Y,
  ap_uint<Type_Width<D_FP_T_>::Value> Angle,
  ap_uint<Type_Width<D_FP_T_>::Value> Scale
);

template<typename FP_T_,int MAX_COLS_,int MAX_ROWS_>
inline static void calcGeoMatrix(
  ap_uint<Type_Width<FP_T_>::Value> Angle,
  ap_uint<Type_Width<FP_T_>::Value> Scale,
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Shift_Y,
  FP_T_ Mat[2][3]
){
#pragma HLS INLINE
#pragma HLS ALLOCATION operation instances=fmul limit=1
#pragma HLS ALLOCATION operation instances=fsub limit=1
#pragma HLS ALLOCATION operation instances=fdiv limit=1
#pragma HLS ALLOCATION operation instances=hmul limit=1
#pragma HLS ALLOCATION operation instances=hsub limit=1
#pragma HLS ALLOCATION operation instances=hdiv limit=1

  const auto Scale_ {Fpt_Func(Scale)};
  const auto Angle_ {Fpt_Func(Angle)};
  const auto Mypi_ {Pi_Func<FP_T_>()};

  const auto Alpha_ {FP_T_ {Scale_*Cos_Func(FP_T_ {Angle_*Mypi_/FP_T_ {180}})}};
  const auto Beta_ {FP_T_ {Scale_*Sin_Func(FP_T_ {Angle_*Mypi_/FP_T_ {180}})}};

  Mat[0][0]=Alpha_;
  Mat[0][1]=-Beta_;
  Mat[0][2]=(ap_uint<1> {1}-Alpha_)*(static_cast<FP_T_>(Width)/FP_T_ {2})+Beta_*(static_cast<FP_T_>(Height)/FP_T_ {2});
  Mat[1][0]=Beta_;
  Mat[1][1]=Alpha_;
  Mat[1][2]=(ap_uint<1> {1}-Alpha_)*(static_cast<FP_T_>(Height)/FP_T_ {2})-Beta_*(static_cast<FP_T_>(Width)/FP_T_ {2});
  Mat[1][2]+=Shift_Y;
}

template<typename FP_T_,int MAX_STRIDE_,int MAX_COLS_,int MAX_ROWS_,int COLOR_CHANNELS_,int DEPTH_,int PPC_>
inline static void rotateFrame
(
  ap_uint<COLOR_CHANNELS_*DEPTH_*PPC_> S2mm[(MAX_STRIDE_/PPC_)*MAX_ROWS_],
  ap_uint<COLOR_CHANNELS_*DEPTH_*PPC_> Mm2s[(MAX_STRIDE_/PPC_)*MAX_ROWS_],
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  FP_T_ Mat[2][3]
){
#pragma HLS INLINE

  loopRows: for(auto Y_ {ap_uint<Bit_Width<MAX_ROWS_>::Value> {0}};Y_<Height;++Y_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_ max=MAX_ROWS_

    loopCols: for(auto X_ {ap_uint<Bit_Width<MAX_COLS_/PPC_>::Value> {0}};X_<Width/PPC_;++X_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/PPC_ max=MAX_COLS_/PPC_
#pragma HLS PIPELINE II=PPC_

      FP_T_ Mm2s_X_[PPC_];
      FP_T_ Mm2s_Y_[PPC_];
      loopCalcXY: for(auto J_ {0};J_<PPC_;++J_){
#pragma HLS UNROLL
        Mm2s_X_[J_]=Mat[0][0]*static_cast<FP_T_>(X_*PPC_+J_)+Mat[0][1]*static_cast<FP_T_>(Y_)+Mat[0][2];
        Mm2s_Y_[J_]=Mat[1][0]*static_cast<FP_T_>(X_*PPC_+J_)+Mat[1][1]*static_cast<FP_T_>(Y_)+Mat[1][2];
      }

      ap_uint<COLOR_CHANNELS_*DEPTH_*PPC_> Mm2s_;
      loopReadSrc: for(auto J_ {0};J_<PPC_;++J_){
#pragma HLS UNROLL
        if(Mm2s_X_[J_]>=FP_T_ {0}&&Mm2s_X_[J_]<static_cast<FP_T_>(Width)&&Mm2s_Y_[J_]>=FP_T_ {0}&&Mm2s_Y_[J_]<static_cast<FP_T_>(Height)){
          const auto Mm2s_Ydec_ {static_cast<ap_uint<Bit_Width<MAX_ROWS_>::Value>>(Mm2s_Y_[J_])};
          const auto Mm2s_Xdec_ {static_cast<ap_uint<Bit_Width<MAX_COLS_>::Value>>(Mm2s_X_[J_])};
          const auto Mm2s_Xshift_ {Mm2s_Xdec_(Bit_Width<MAX_ROWS_>::Value-1,Bit_Width<PPC_>::Value-1)};
          const auto S2mm_ {S2mm[(MAX_STRIDE_/PPC_)*Mm2s_Ydec_+Mm2s_Xshift_]};
#if 1==D_PPC_
          Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*0+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*0);
#else
          switch(Mm2s_Xdec_(Bit_Width<PPC_>::Value-2,0)){
            case 0:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*0+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*0);
              break;
            case 1:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*1+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*1);
              break;
            case 2:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*2+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*2);
              break;
            case 3:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*3+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*3);
              break;
            case 4:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*4+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*4);
              break;
            case 5:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*5+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*5);
              break;
            case 6:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*6+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*6);
              break;
            case 7:
              Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=S2mm_(COLOR_CHANNELS_*DEPTH_*7+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*7);
              break;
          }
#endif
        } else {
          Mm2s_(COLOR_CHANNELS_*DEPTH_*J_+COLOR_CHANNELS_*DEPTH_-1,COLOR_CHANNELS_*DEPTH_*J_)=ap_uint<COLOR_CHANNELS_*DEPTH_> {0};
        }
      }

      Mm2s[(MAX_STRIDE_/PPC_)*Y_+X_]=Mm2s_;
    }
  }
}

#endif
