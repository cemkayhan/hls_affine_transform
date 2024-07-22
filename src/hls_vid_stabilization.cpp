#include "hls_vid_stabilization.h"

#include <string>
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

void Func1(
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  double M[2][3],
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY
){
#pragma HLS INLINE off

  for(auto J_=0;J_<Height;J_+=D_BLOCK_SIZE_){
    for(auto K_=0;K_<Width;K_+=D_BLOCK_SIZE_){
      auto topLeftXSet_{false};
      auto topLeftYSet_{false};
      int topLeftX_;
      int topLeftY_;
      for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        for(auto KK_=0;KK_<D_BLOCK_SIZE_;++KK_){
          const auto SrcX_{static_cast<int>(M[0][0]*(KK_+K_)+M[0][1]*(JJ_+J_)+M[0][2])};
          const auto SrcY_{static_cast<int>(M[1][0]*(KK_+K_)+M[1][1]*(JJ_+J_)+M[1][2])};
          srcXStream.write(SrcX_);
          srcYStream.write(SrcY_);
          if(!topLeftXSet_){
            topLeftXSet_=true;
            topLeftX_=SrcX_;
          } else if(topLeftX_>SrcX_){
            topLeftX_=SrcX_;
          }
          if(!topLeftYSet_){
            topLeftYSet_=true;
            topLeftY_=SrcY_;
          } else if(topLeftY_>SrcY_){
            topLeftY_=SrcY_;
          }
        }
      }
      topLeftX.write(topLeftX_);
      topLeftY.write(topLeftY_);
    }
  }
}

void Func2(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(D_MAX_STRIDE_/D_PPC_)*(2*D_MAX_ROWS_)],
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> >& srcStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY
){
#pragma HLS INLINE off

  for(auto J_=0;J_<Height;J_+=D_BLOCK_SIZE_){
    for(auto K_=0;K_<Width;K_+=D_BLOCK_SIZE_){
      int topLeftX_;
      int topLeftY_;
      topLeftX>>topLeftX_;
      topLeftY>>topLeftY_;

      static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> tmp[(2*D_BLOCK_SIZE_)][(2*D_BLOCK_SIZE_)];

      for(auto JJ_=0;JJ_<(2*D_BLOCK_SIZE_);++JJ_){
        for(auto KK_=0;KK_<((2*D_BLOCK_SIZE_)/D_PPC_);++KK_){
          const auto pix_ {Src[(JJ_+topLeftY_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_PPC_)+(KK_+((topLeftX_+D_COLS_MARGIN_)/D_PPC_))]};
 
          for(auto I_=0;I_<D_PPC_;++I_){
#pragma HLS UNROLL
            tmp[JJ_][KK_*D_PPC_+I_]=pix_(I_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_-1,I_*D_COLOR_CHANNELS_*D_DEPTH_);
          }
        }
      }

#if 4==D_PPC_
      const ap_int<32> topLeftX2_ {topLeftX_};
      auto tmpTmp2_=topLeftX2_(1,0);
#elif 2==D_PPC_
      auto tmpTmp2_=topLeftX2_(0,0);
#else
      auto tmpTmp2_=0;
#endif

      for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        for(auto KK_=0;KK_<D_BLOCK_SIZE_;++KK_){
          int srcXStream_;
          int srcYStream_;
          srcXStream>>srcXStream_;
          srcYStream>>srcYStream_;
          srcStream<<tmp[(srcYStream_+D_ROWS_MARGIN_)-(topLeftY_+D_ROWS_MARGIN_)][(srcXStream_+tmpTmp2_+D_COLS_MARGIN_)-(topLeftX_+D_COLS_MARGIN_)];
        }
      }
    }
  }
}

void Func3(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> >& srcStream,
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> dstAxi[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height
){
  for(auto J_=0;J_<height;J_+=D_BLOCK_SIZE_){
    for(auto K_=0;K_<width;K_+=D_BLOCK_SIZE_){
      for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        for(auto KK_=0;KK_<(D_BLOCK_SIZE_/D_PPC_);++KK_){
          ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> dstAxiPix_;
          for(auto I_=0;I_<D_PPC_;++I_){
#pragma HLS UNROLL
            ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> srcStreamPix_;
            srcStream>>srcStreamPix_;
            dstAxiPix_(I_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_,I_*D_COLOR_CHANNELS_*D_DEPTH_)=srcStreamPix_;
          }
          dstAxi[(J_+JJ_)*(D_MAX_STRIDE_/D_PPC_)+K_/D_PPC_+KK_]=dstAxiPix_;
        }
      }
    }
  }
}

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(D_MAX_STRIDE_/D_PPC_)*(2*D_MAX_ROWS_)],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> DstAxi[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  double M00, double M01, double M02,
  double M10, double M11, double M12
){
#pragma HLS INTERFACE m_axi port=Src bundle=srcaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_
#pragma HLS INTERFACE m_axi port=DstAxi bundle=dstaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_

#pragma HLS INTERFACE s_axilite bundle=ctrl port=return
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x10 port=Width
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x18 port=Height
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x20 port=Src

#pragma HLS STABLE variable=Width
#pragma HLS STABLE variable=Height

  const auto Width_ {Width};
  const auto Height_ {Height};

  double M_[2][3];
  M_[0][0]=M00;
  M_[0][1]=M01;
  M_[0][2]=M02;
  M_[1][0]=M10;
  M_[1][1]=M11;
  M_[1][2]=M12;

#pragma HLS DATAFLOW

  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> > srcStream_;
#pragma HLS STREAM variable=srcStream_

  hls::stream<int> topLeftX_("topLeftX");
#pragma HLS STREAM variable=topLeftX_

  hls::stream<int> topLeftY_("topLeftY");
#pragma HLS STREAM variable=topLeftY_

  hls::stream<int> srcXStream_("srcXStream_");
#pragma HLS STREAM variable=srcXStream_

  hls::stream<int> srcYStream_("srcYStream_");
#pragma HLS STREAM variable=srcYStream_

  Func1(Width_,Height_,M_,srcXStream_,srcYStream_,topLeftX_,topLeftY_);
  Func2(Src,srcStream_,Width_,Height_,srcXStream_,srcYStream_,topLeftX_,topLeftY_);
  Func3(srcStream_,DstAxi,Width_,Height_);
}
