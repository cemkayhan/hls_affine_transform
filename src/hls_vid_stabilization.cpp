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

  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      auto topLeftXSet_{false};
      auto topLeftYSet_{false};
      int topLeftX_;
      int topLeftY_;
      for(auto JJ_=0;JJ_<32;++JJ_){
        for(auto KK_=0;KK_<32;++KK_){
          const auto SrcX_{static_cast<int>(M[0][0]*(KK_+K_)+M[0][1]*(JJ_+J_)+M[0][2])};
          const auto SrcY_{static_cast<int>(M[1][0]*(KK_+K_)+M[1][1]*(JJ_+J_)+M[1][2])};
          //srcXStream.write(SrcX_+Width/2);
          //srcXStream.write(SrcX_+Width/D_PPC_/2);
          srcXStream.write(SrcX_);
          srcYStream.write(SrcY_+Height/2);
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
      //topLeftX.write(topLeftX_+Width/2);
      //topLeftX.write(topLeftX_+Width/D_PPC_/2);
      topLeftX.write(topLeftX_);
      topLeftY.write(topLeftY_+Height/2);
    }
  }
}

void Func2(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(D_MAX_STRIDE_/D_PPC_)*(2*D_MAX_ROWS_)],
  //hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> >& srcStream,
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> >& srcStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY
){
#pragma HLS INLINE off

  std::ofstream ofs {"log.txt"};
  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      int topLeftX_;
      int topLeftY_;
      topLeftX.read(topLeftX_);
      topLeftY.read(topLeftY_);
      ofs<<"J_: "<<J_<<", K_: "<<K_<<", topX: "<<topLeftX_<<", topY: "<<topLeftY_<<'\n';

      //static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> tmp[64][64];
      static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> tmp[64][64];

      for(auto JJ_=0;JJ_<64;++JJ_){
        std::ofstream myofs {"hls_"+std::to_string(J_)+"_"+std::to_string(K_)+"_"+std::to_string(JJ_)+".txt"};
        for(auto KK_=0;KK_<(64/D_PPC_);++KK_){
          //const auto pix_ {Src[(JJ_+topLeftY_)*(D_MAX_STRIDE_/D_PPC_)+(KK_+topLeftX_)]};
          //const auto pix_ {Src[(JJ_+topLeftY_)*(D_MAX_STRIDE_/D_PPC_)+(KK_+topLeftX_+Width/2)]};
          //const auto pix_ {Src[(JJ_+topLeftY_)*(D_MAX_STRIDE_/D_PPC_)+(KK_+((topLeftX_)/D_PPC_+(Width/2)/D_PPC_))]};
          const auto pix_ {Src[(JJ_+topLeftY_)*(D_MAX_STRIDE_/D_PPC_)+(KK_+((topLeftX_+Width/2)/D_PPC_))]};
          //const auto pix_ {Src[(JJ_+topLeftY_)*(D_MAX_STRIDE_/D_PPC_)+(KK_+(topLeftX_+Width/2)/D_PPC_)]};
 
          for(auto I_=0;I_<D_PPC_;++I_){
#pragma HLS UNROLL
            tmp[JJ_][KK_*D_PPC_+I_]=pix_(I_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_-1,I_*D_COLOR_CHANNELS_*D_DEPTH_);
            ap_uint<32> tmp2=tmp[JJ_][KK_*D_PPC_+I_];
            myofs<<"topLeftX_: "<<topLeftX_<<", JJ_: "<<JJ_<<", KK_: "<<KK_*D_PPC_+I_<<", pix[0]: "<<tmp2(7,0)<<", pix[1]: "<<tmp2(15,8)<<", pix[2]: "<<tmp2(23,16)<<'\n';
          }
        }
      }

      auto tmpTmp_=0;
      cv::Mat tmpImg_=cv::Mat::zeros(64,64,CV_8UC3);
      for(auto JJ_=0;JJ_<64;++JJ_){
        for(auto KK_=tmpTmp_;KK_<64;++KK_){
          ap_uint<32> tmp2=tmp[JJ_][KK_];
          cv::Vec3b pix_;
          pix_[0]=tmp2(7,0);
          pix_[1]=tmp2(15,8);
          pix_[2]=tmp2(23,16);
          tmpImg_.at<cv::Vec3b>(JJ_,KK_-tmpTmp_)=pix_;
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

      std::ofstream ofs2 {"topLeftX_"+std::to_string(J_)+"_"+std::to_string(K_)+".txt"};
      ofs2<<"topLeftX: "<<topLeftX_<<", topLeftX/D_PPC_: "<<topLeftX_/D_PPC_<<'\n';
      for(auto JJ_=0;JJ_<32;++JJ_){
        for(auto KK_=0;KK_<32;++KK_){
          int srcXStream_;
          int srcYStream_;
          srcXStream.read(srcXStream_);
          srcYStream.read(srcYStream_);
          //srcStream.write(tmp[srcYStream_-topLeftY_][(srcXStream_+Width/2)-(topLeftX_+Width/2)]);
          //srcStream.write(tmp[srcYStream_-topLeftY_][(srcXStream_+Width/2)-(topLeftX_+Width/2)]);
          ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> tmpx_;
          //tmpx_=tmp[srcYStream_-topLeftY_][(srcXStream_+Width/2)-(topLeftX_+Width/2)];
          //tmpx_=tmp[srcYStream_-topLeftY_][(srcXStream_+Width/2)-(topLeftX_+Width/2)];
          tmpx_=tmp[srcYStream_-topLeftY_][(srcXStream_+tmpTmp2_+Width/2)-(topLeftX_+Width/2)];
          srcStream.write(tmpx_);
          cv::Vec3b tmpPix_;
          tmpPix_[0]=44;
          tmpPix_[1]=144;
          tmpPix_[2]=244;
          tmpImg_.at<cv::Vec3b>(cv::Point((srcXStream_+tmpTmp2_+Width/2)-(topLeftX_+Width/2),srcYStream_-topLeftY_))=tmpPix_;
          ofs2<<"srcYStream_-topLeftY_: "<<srcYStream_-topLeftY_<<", (srcXStream_+Width/2)-(topLeftX_+Width/2): "<<(srcXStream_+Width/2)-(topLeftX_+Width/2)
              <<", pix[0]: "<<tmpx_(7,0)<<", pix[1]: "<<tmpx_(15,8)<<", pix[2]: "<<tmpx_(23,16)<<'\n';
        }
      }
      cv::imwrite("tmpImgNow"+std::to_string(J_)+"_"+std::to_string(K_)+".png",tmpImg_);
    }
  }
}

void Func3(
  //hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> >& srcStream,
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> >& srcStream,
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> DstAxi[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height
){
  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      for(auto JJ_=0;JJ_<32;++JJ_){
        for(auto KK_=0;KK_<(32/D_PPC_);++KK_){
          //ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> srcStreamPix_;
          //DstAxi[(J_+JJ_)*(D_MAX_STRIDE_/D_PPC_)+K_+KK_]=srcStreamPix_;

          ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> dstAxiPix_;
          for(auto I_=0;I_<D_PPC_;++I_){
#pragma HLS UNROLL
            ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> srcStreamPix_;
            srcStream.read(srcStreamPix_);
            dstAxiPix_(I_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_,I_*D_COLOR_CHANNELS_*D_DEPTH_)=srcStreamPix_;
          }
          //DstAxi[(J_+JJ_)*(D_MAX_STRIDE_/D_PPC_)+K_+KK_]=dstAxiPix_;
          DstAxi[(J_+JJ_)*(D_MAX_STRIDE_/D_PPC_)+K_/D_PPC_+KK_]=dstAxiPix_;
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

  //hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> > srcStream_;
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
