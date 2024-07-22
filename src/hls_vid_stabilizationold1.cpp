#include "hls_vid_stabilization.h"

#include <string>
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

void Func1(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Dst[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> >& dstStream,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> >& Img_Tmp,
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> >& srcStream,
  ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> imgBlock[32][32],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  double M[2][3],
  int SrcX[32][32],
  int SrcY[32][32],
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY,
  hls::stream<int>& bottomRightX,
  hls::stream<int>& bottomRightY
){
#pragma HLS INLINE off

  //for(auto J_=0;J_<Height;J_+=32){
  //  for(auto K_=0;K_<Width;K_+=32){
  //    for(auto JJ_=0;JJ_<32;++JJ_){
  //      for(auto KK_=0;KK_<32;++KK_){
  //        ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> Pix_; 
  //        Pix_.data=Src[(J_+JJ_)*Width+K_+KK_];
  //        Img_Tmp.write(Pix_);
  //      }
  //    }
  //  }
  //}

  hls::stream<int> localSrcXStream_("localSrcXStream_");
#pragma HLS STREAM variable=localSrcXStream_

  hls::stream<int> localSrcYStream_("localSrcYStream_");
#pragma HLS STREAM variable=localSrcYStream_

  for(auto J_=0;J_<Height;J_+=32){
    for(auto K_=0;K_<Width;K_+=32){
      auto topLeftXSet_=false;
      auto topLeftYSet_=false;
      auto bottomRightXSet_=false;
      auto bottomRightYSet_=false;
      int topLeftX_;
      int topLeftY_;
      int bottomRightX_;
      int bottomRightY_;
      std::ofstream ofsxy2 {"hlsxy2_"+std::to_string(J_)+"_"+std::to_string(K_)+".txt"};
      for(auto JJ_=0;JJ_<32;++JJ_){
        for(auto KK_=0;KK_<32;++KK_){
          const auto SrcX_{static_cast<int>(M[0][0]*(KK_+K_)+M[0][1]*(JJ_+J_)+M[0][2])};
          const auto SrcY_{static_cast<int>(M[1][0]*(KK_+K_)+M[1][1]*(JJ_+J_)+M[1][2])};
          static std::ofstream ofsxy {"hlsxy.txt"};
          ofsxy<<"X: "<<SrcX_<<", Y: "<<SrcY_<<'\n';
          ofsxy2<<"X: "<<SrcX_<<", Y: "<<SrcY_<<'\n';
          SrcX[JJ_][KK_]=SrcX_;
          SrcY[JJ_][KK_]=SrcY_;
          srcXStream.write(SrcX_);
          localSrcXStream_.write(SrcX_);
          srcYStream.write(SrcY_);
          localSrcYStream_.write(SrcY_);
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
      topLeftX.write(topLeftX_);
      topLeftY.write(topLeftY_);
      bottomRightX.write(bottomRightX_);
      bottomRightY.write(bottomRightY_);

      static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> tmp[64][64];
      //hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value>> tmpStream_;
      if(topLeftX_>=0&&topLeftY_>=0&&bottomRightX_<Width&&bottomRightY_<Height){
        for(auto JJ_=0;JJ_<bottomRightY_-topLeftY_+1;++JJ_){
          for(auto KK_=0;KK_<bottomRightX_-topLeftX_+1;++KK_){
            //ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> pix_; 
            //pix_.data=Src[J_*Width+K_];
            //dstStream.write(pix_);
            tmp[JJ_][KK_]=Src[(JJ_+topLeftY_)*Width+KK_+topLeftX_];
            //tmpStream_.write(Src[(J+topLeftY_)*Width+K_+topLeftX_]);
          }
        }
        for(auto JJ_=0;JJ_<32;++JJ_){
          for(auto KK_=0;KK_<32;++KK_){
            int localSrcX_;
            int localSrcY_;
            localSrcXStream_.read(localSrcX_);
            localSrcYStream_.read(localSrcY_);
            srcStream.write(tmp[localSrcY_-topLeftY_][localSrcX_-topLeftX_]);
          }
        }
      }else{
        for(auto J_=0;J_<32;++J_){
          for(auto K_=0;K_<32;++K_){
            int localSrcX_;
            int localSrcY_;
            localSrcXStream_.read(localSrcX_);
            localSrcYStream_.read(localSrcY_);
          }
        }
      }
    }
  }
}

void Func2(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> >& srcStream,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> >& Img_Tmp,
  ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> imgBlock[32][32],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> DstAxi[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> >& Dst,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> >& dstStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  int SrcX[32][32],
  int SrcY[32][32],
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY,
  hls::stream<int>& bottomRightX,
  hls::stream<int>& bottomRightY
){
#pragma HLS INLINE off

  cv::Mat dstImg_=cv::Mat::zeros(1024,1024,CV_8UC3);

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
      ofs<<"J_: "<<J_<<", K_: "<<K_<<", topX: "<<topLeftX_<<", topY: "<<topLeftY_<<", botX: "<<bottomRightX_<<", botY: "<<bottomRightY_<<'\n';
      cv::Mat rectImg_=cv::Mat::zeros(1024,1024,CV_8UC3);

      //static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> tmp[64][64];
      //for(auto J_=0;J_<64;++J_){
      //  for(auto K_=0;K_<64;++K_){
      //    tmp[J_][K_]=0;
      //  }
      //}

      //static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> imgBlock_[32][32];
      //for(auto J_=0;J_<32;++J_){
      //  for(auto K_=0;K_<32;++K_){
      //    ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> Pix_; 
      //    Img_Tmp.read(Pix_);
      //    imgBlock_[J_][K_]=Pix_.data;
      //    Dst.write(Pix_);
      //  }
      //}

      //static std::ofstream ofsxy3 {"hlsxy3.txt"};
      if(topLeftX_>=0&&topLeftY_>=0&&bottomRightX_<Width&&bottomRightY_<Height){
        for(auto JJ_=0;JJ_<32;++JJ_){
          for(auto KK_=0;KK_<32;++KK_){
            ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> srcStreamPix_;
            srcStream.read(srcStreamPix_);
            DstAxi[(J_+JJ_)*Width+K_+KK_]=srcStreamPix_;

            srcXStream.read(srcXStream_);
            srcYStream.read(srcYStream_);
          }
        }

        //cv::rectangle(rectImg_,cv::Point(topLeftX_,topLeftY_),cv::Point(bottomRightX_,bottomRightY_),cv::Scalar(255,0,0),1,cv::LINE_8);
        //for(auto J_=0;J_<bottomRightY_-topLeftY_;++J_){
        //  for(auto K_=0;K_<bottomRightX_-topLeftX_;++K_){
        //    ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> dstPix_; 
        //    //dstStream.read(dstPix_);
        //    //tmp[J_][K_]=dstPix_.data;
        //  }
        //}
        //for(auto JJ_=0;JJ_<32;++JJ_){
        //  for(auto KK_=0;KK_<32;++KK_){
        //    srcXStream.read(srcXStream_);
        //    srcYStream.read(srcYStream_);
        //    ofsxy3<<"X: "<<srcXStream_<<", Y: "<<srcYStream_<<'\n';
        //    ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> Pix_; 
        //    //Img_Tmp.read(Pix_);
        //    //Dst.write(Pix_);
        //    tmp[srcYStream_-topLeftY_][srcXStream_-topLeftX_]=Pix_.data;
        //    //tmp[JJ_][KK_]=Pix_.data;
        //    //tmp[srcYStream_-topLeftY_][srcXStream_-topLeftX_]=imgBlock_[srcYStream_-topLeftY_][srcXStream_-topLeftX_];

        //    cv::Vec3b pix_;
        //    ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> imgBlockPix_;
        //    //imgBlockPix_=imgBlock_[JJ_][KK_];
        //    //pix_[0]=imgBlockPix_(7,0);
        //    //pix_[1]=imgBlockPix_(15,8);
        //    //pix_[2]=imgBlockPix_(23,16);
        //    pix_[0]=Pix_.data(7,0);
        //    pix_[1]=Pix_.data(15,8);
        //    pix_[2]=Pix_.data(23,16);
        //    if(topLeftX_>=0&&topLeftY_>=0&&bottomRightX_<1024&&bottomRightY_<1024){
        //      rectImg_.at<cv::Vec3b>(cv::Point(srcXStream_,srcYStream_))=pix_;
        //    }
        //  }
        //}
        //for(auto JJ_=topLeftY_;JJ_<bottomRightY_;++JJ_){
        //  for(auto KK_=topLeftX_;KK_<bottomRightX_;++KK_){
        //    DstAxi[JJ_*Width+KK_]=tmp[JJ_-topLeftY_][KK_-topLeftX_];
        //  }
        //}
        //cv::Mat cvTmp_=cv::Mat::zeros(64,64,CV_8UC3);
        //for(auto J_=0;J_<64;++J_){
        //  for(auto K_=0;K_<64;++K_){
        //    ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> tmpPix_=tmp[J_][K_];
        //    cv::Vec3b pix_;
        //    pix_[0]=tmpPix_(7,0);
        //    pix_[1]=tmpPix_(15,8);
        //    pix_[2]=tmpPix_(23,16);
        //    cvTmp_.at<cv::Vec3b>(J_,K_)=pix_;
        //  }
        //}
        //cv::imwrite("tmpImg_"+std::to_string(J_)+"_"+std::to_string(K_)+".png",cvTmp_);
      } else {
        for(auto JJ_=0;JJ_<32;++JJ_){
          for(auto KK_=0;KK_<32;++KK_){
            srcXStream.read(srcXStream_);
            srcYStream.read(srcYStream_);
            //ofsxy3<<"X: "<<srcXStream_<<", Y: "<<srcYStream_<<'\n';
            //ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> Pix_; 
            //Img_Tmp.read(Pix_);
            //Dst.write(Pix_);
            //ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> Pix_; 
            //Img_Tmp.read(Pix_);
            //Dst.write(Pix_);
          }
        }
      }
      //cv::imwrite("rectImg_"+std::to_string(J_)+"_"+std::to_string(K_)+".png",rectImg_);
    }
  }
}

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Src[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> DstAxi[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_],
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> >& Dst,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  double M00, double M01, double M02,
  double M10, double M11, double M12
){
#pragma HLS INTERFACE m_axi port=Src bundle=srcaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_
#pragma HLS INTERFACE m_axi port=DstAxi bundle=dstaxi depth=(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_
#pragma HLS INTERFACE axis port=Dst

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

  static int SrcX_[32][32];
  static int SrcY_[32][32];
  static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value> imgBlock_[32][32];

#pragma HLS DATAFLOW

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> > Img_Tmp_;
#pragma HLS STREAM variable=Img_Tmp_

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_PPC_>::Value,1,1,1> > dstStream_;
#pragma HLS STREAM variable=dstStream_

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

  Func1(Src,DstAxi,dstStream_,Img_Tmp_,srcStream_,imgBlock_,Width_,Height_,M_,SrcX_,SrcY_,srcXStream_,srcYStream_,topLeftX_,topLeftY_,bottomRightX_,bottomRightY_);
  Func2(srcStream_,Img_Tmp_,imgBlock_,DstAxi,Dst,dstStream_,Width_,Height_,SrcX_,SrcY_,srcXStream_,srcYStream_,topLeftX_,topLeftY_,bottomRightX_,bottomRightY_);
}
