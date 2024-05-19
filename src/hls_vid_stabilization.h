#ifndef HLS_VID_STABILIZATION_H_INCLUDE_GUARD_
#define HLS_VID_STABILIZATION_H_INCLUDE_GUARD_

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "axi_vid_bus_width.h"
#include "bit_width.h"
#include "hls_math.h"
#include "sinfunction.h"
#include "cosfunction.h"

#if 1==D_ENABLE_C_SIMULATION_
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#if 1==D_ENABLE_C_SIMULATION_DEBUG_
#include <fstream>
#endif
#endif

void D_TOP_
(
  ap_uint<2*D_DEPTH_*D_PPC_> S2mm[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<2*D_DEPTH_*D_PPC_> Mm2s[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Shift_Y,
  D_FP_T_ Angle,
  D_FP_T_ Scale
);

template<typename FP_T_,int MAX_COLS_,int MAX_ROWS_>
inline static void calcGeoMatrix(
  FP_T_ Angle,FP_T_ Scale,
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

  const auto Mypi_ {FP_T_ {3.141592653589793238462f}};
  //const auto Alpha_ {FP_T_ {Scale*cosFunction(FP_T_ {Angle*Mypi_/FP_T_ {180}})}};
  const auto Alpha_ {FP_T_ {Scale*cosFunction(FP_T_ {Angle*FP_T_ {CV_PI}/FP_T_ {180}})}};
  //const auto Beta_ {FP_T_ {Scale*sinFunction(FP_T_ {Angle*Mypi_/FP_T_ {180}})}};
  const auto Beta_ {FP_T_ {Scale*sinFunction(FP_T_ {Angle*FP_T_ {CV_PI}/FP_T_ {180}})}};

  Mat[0][0]=Alpha_;
  Mat[0][1]=-Beta_;
  Mat[0][2]=(ap_uint<1> {1}-Alpha_)*(static_cast<FP_T_>(Width)/2)+Beta_*(static_cast<FP_T_>(Height)/2);
  Mat[1][0]=Beta_;
  Mat[1][1]=Alpha_;
  Mat[1][2]=(ap_uint<1> {1}-Alpha_)*(static_cast<FP_T_>(Height)/2)-Beta_*(static_cast<FP_T_>(Width)/2);
  Mat[1][2]+=Shift_Y;
}

template<typename FP_T_,int MAX_STRIDE_,int MAX_COLS_,int MAX_ROWS_,int DEPTH_,int PPC_>
inline static void rotateFrame
(
  ap_uint<2*DEPTH_*PPC_> S2mm[(MAX_STRIDE_/PPC_)*MAX_ROWS_],
  ap_uint<2*DEPTH_*PPC_> Mm2s[(MAX_STRIDE_/PPC_)*MAX_ROWS_],
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  FP_T_ Mat[2][3]
){
#pragma HLS INLINE

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
  static std::ofstream ofs {"rotateFrameHls.txt"}; 
#endif

  loopRows: for(auto Y_= ap_uint<Bit_Width<MAX_ROWS_>::Value> {0};Y_<Height;++Y_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_ max=MAX_ROWS_

    loopCols: for(auto X_=ap_uint<Bit_Width<MAX_COLS_/PPC_>::Value> {0};X_<Width/PPC_;++X_){
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
        if(Mm2s_X_[J]>=static_cast<FP_T_>(0)&&Mm2s_X_[J]<static_cast<FP_T_>(Width)&&Mm2s_Y_[J]>=static_cast<FP_T_>(0)&&Mm2s_Y_[J]<static_cast<FP_T_>(Height)){
          const auto Mm2s_Ydec_ {static_cast<ap_uint<Bit_Width<MAX_ROWS_>::Value>>(Mm2s_Y_[J])};
          const auto Mm2s_Xdec_ {static_cast<ap_uint<Bit_Width<MAX_COLS_>::Value>>(Mm2s_X_[J])};
          const auto Mm2s_Xshift_ {Mm2s_Xdec_(Bit_Width<MAX_ROWS_>::Value-1,Bit_Width<PPC_>::Value-1)};
          const auto S2mm_ {S2mm[(MAX_STRIDE_/PPC_)*Mm2s_Ydec_+Mm2s_Xshift_]};
#if 1==D_PPC_
          Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*0+2*DEPTH_-1,2*DEPTH_*0);
#else
          switch(Mm2s_Xdec_(Bit_Width<PPC_>::Value-2,0)){
            case 0:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*0+2*DEPTH_-1,2*DEPTH_*0);
              break;
            case 1:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*1+2*DEPTH_-1,2*DEPTH_*1);
              break;
            case 2:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*2+2*DEPTH_-1,2*DEPTH_*2);
              break;
            case 3:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*3+2*DEPTH_-1,2*DEPTH_*3);
              break;
            case 4:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*4+2*DEPTH_-1,2*DEPTH_*4);
              break;
            case 5:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*5+2*DEPTH_-1,2*DEPTH_*5);
              break;
            case 6:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*6+2*DEPTH_-1,2*DEPTH_*6);
              break;
            case 7:
              Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=S2mm_(2*DEPTH_*7+2*DEPTH_-1,2*DEPTH_*7);
              break;
          }
#endif
        } else {
          Mm2s_(2*DEPTH_*J+2*DEPTH_-1,2*DEPTH_*J)=ap_uint<2*DEPTH_> {0x80};
        }
      }

      Mm2s[(MAX_STRIDE_/PPC_)*Y_+X_]=Mm2s_;
    }
  }
}

#endif
