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
  hls::stream<int>& topLeftY,
  hls::stream<int>& bottomRightX,
  hls::stream<int>& bottomRightY
){
#pragma HLS INLINE off

  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      auto topLeftXSet_{false};
      auto topLeftYSet_{false};
      auto bottomRightXSet_{false};
      auto bottomRightYSet_{false};
      int topLeftX_;
      int topLeftY_;
      int bottomRightX_;
      int bottomRightY_;
      for(auto JJ_=0;JJ_<32;++JJ_){
        for(auto KK_=0;KK_<32;++KK_){
          const auto SrcX_{static_cast<int>(M[0][0]*(KK_+K_)+M[0][1]*(JJ_+J_)+M[0][2])};
          const auto SrcY_{static_cast<int>(M[1][0]*(KK_+K_)+M[1][1]*(JJ_+J_)+M[1][2])};
          srcXStream.write(SrcX_);
          //srcXStream.write(SrcX_+Width/2);
          srcYStream.write(SrcY_);
          //srcYStream.write(SrcY_+Height/2);
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
          if(!bottomRightXSet_){
            bottomRightXSet_=true;
            bottomRightX_=SrcX_;
          } else if(bottomRightX_<SrcX_){
            bottomRightX_=SrcX_;
          }
          if(!bottomRightYSet_){
            bottomRightYSet_=true;
            bottomRightY_=SrcY_;
          } else if(bottomRightY_<SrcY_){
            bottomRightY_=SrcY_;
          }
        }
      }
      //topLeftX.write(topLeftX_);
      topLeftX.write(topLeftX_+Width/2);
      //topLeftY.write(topLeftY_);
      topLeftY.write(topLeftY_+Height/2);
      bottomRightX.write(bottomRightX_);
      //bottomRightX.write(bottomRightX_+Width/2);
      bottomRightY.write(bottomRightY_);
      //bottomRightY.write(bottomRightY_+Height/2);
    }
  }
}

void Func2(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(2*(D_MAX_STRIDE_/D_PPC_))*(2*D_MAX_ROWS_)],
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> >& srcStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY,
  hls::stream<int>& bottomRightX,
  hls::stream<int>& bottomRightY
){
#pragma HLS INLINE off

  std::ofstream ofs {"log.txt"};
  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      int topLeftX_;
      int topLeftY_;
      int bottomRightX_;
      int bottomRightY_;
      int srcXStream_;
      int srcYStream_;
      topLeftX.read(topLeftX_);
      topLeftY.read(topLeftY_);
      bottomRightX.read(bottomRightX_);
      bottomRightY.read(bottomRightY_);
      ofs<<"J_: "<<J_<<", K_: "<<K_<<", topX: "<<topLeftX_<<", topY: "<<topLeftY_<<", botX: "<<bottomRightX_<<", botY: "<<bottomRightY_<<", diffX: "<<bottomRightX_-topLeftX_<<", diffY: "<<bottomRightY_-topLeftY_<<'\n';

      static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> tmp[64][64];
      //if(topLeftX_>=0&&topLeftY_>=0&&bottomRightX_<Width&&bottomRightY_<Height){
        //for(auto JJ_=0;JJ_<bottomRightY_-topLeftY_+1;++JJ_){
        //  for(auto KK_=0;KK_<bottomRightX_-topLeftX_+1;++KK_){
        //    tmp[JJ_][KK_]=Src[(JJ_+Height/2+topLeftY_)*Width+(KK_+Width/2+topLeftX_)];
        //  }
        //}

        for(auto JJ_=0;JJ_<64;++JJ_){
          for(auto KK_=0;KK_<64;++KK_){
            //tmp[JJ_][KK_]=Src[(JJ_+Height/2+topLeftY_)*Width+(KK_+Width/2+topLeftX_)];
            //tmp[JJ_][KK_]=Src[(JJ_+Height/2+topLeftY_)*(2*Width)+(KK_+Width/2+topLeftX_)];
            //tmp[JJ_][KK_]=Src[(JJ_+Height/2+topLeftY_)*(2*Width)+(KK_+topLeftX_)];
            tmp[JJ_][KK_]=Src[(JJ_+topLeftY_)*(2*Width)+(KK_+topLeftX_)];
            //tmp[JJ_][KK_]=Src[(JJ_+topLeftY_)*(2*Width)+(KK_+(topLeftX_-Width/2))];
            //tmp[JJ_][KK_]=Src[(JJ_+topLeftY_)*Width+(KK_+topLeftX_)];
          }
        }

        for(auto JJ_=0;JJ_<32;++JJ_){
          for(auto KK_=0;KK_<32;++KK_){
            srcXStream.read(srcXStream_);
            srcYStream.read(srcYStream_);

            //srcStream.write(tmp[srcYStream_-topLeftY_][srcXStream_-topLeftX_]);
            //srcStream.write(tmp[srcYStream_-topLeftY_][srcXStream_-(topLeftX_-Width/2)]);
            srcStream.write(tmp[srcYStream_-(topLeftY_-Height/2)][srcXStream_-(topLeftX_-Width/2)]);
            //srcStream.write(tmp[srcYStream_-(topLeftY_-Height/2)][srcXStream_-(topLeftX_-Width/2)]);
          }
        }
      //} else {
      //  for(auto JJ_=0;JJ_<32;++JJ_){
      //    for(auto KK_=0;KK_<32;++KK_){
      //      srcXStream.read(srcXStream_);
      //      srcYStream.read(srcYStream_);

      //      srcStream.write(0);
      //    }
      //  }
      //}
    }
  }
}

void Func3(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> >& srcStream,
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> DstAxi[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height
){
  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      for(auto JJ_=0;JJ_<32;++JJ_){
        for(auto KK_=0;KK_<32;++KK_){
          ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> srcStreamPix_;
          srcStream.read(srcStreamPix_);
          DstAxi[(J_+JJ_)*Width+K_+KK_]=srcStreamPix_;
        }
      }
    }
  }
}

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(2*(D_MAX_STRIDE_/D_PPC_))*(2*D_MAX_ROWS_)],
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

  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> > srcStream_;
#pragma HLS STREAM variable=srcStream_

  hls::stream<int> topLeftX_("topLeftX");
#pragma HLS STREAM variable=topLeftX_

  hls::stream<int> topLeftY_("topLeftY");
#pragma HLS STREAM variable=topLeftY_

  hls::stream<int> bottomRightX_("bottomRightX_");
#pragma HLS STREAM variable=bottomRightX_

  hls::stream<int> bottomRightY_("bottomRightY_");
#pragma HLS STREAM variable=bottomRightY_

  hls::stream<int> srcXStream_("srcXStream_");
#pragma HLS STREAM variable=srcXStream_

  hls::stream<int> srcYStream_("srcYStream_");
#pragma HLS STREAM variable=srcYStream_

  Func1(Width_,Height_,M_,srcXStream_,srcYStream_,topLeftX_,topLeftY_,bottomRightX_,bottomRightY_);
  Func2(Src,srcStream_,Width_,Height_,srcXStream_,srcYStream_,topLeftX_,topLeftY_,bottomRightX_,bottomRightY_);
  Func3(srcStream_,DstAxi,Width_,Height_);
}
