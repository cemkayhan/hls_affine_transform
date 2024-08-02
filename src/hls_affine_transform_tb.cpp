#include "hls_affine_transform.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <string>

// GOLDEN
cv::Mat goldenGetRotationMatrix2D(cv::Point2f center, float angle, float scale) {
    float alpha = scale * cos(angle * CV_PI / 180.0);
    float beta = scale * sin(angle * CV_PI / 180.0);

    cv::Mat rot_mat = cv::Mat(2, 3, CV_64F); //2x3 matrix oluşturuyoruz
    rot_mat.at<float>(0, 0) = alpha;
    rot_mat.at<float>(0, 1) = -beta;
    rot_mat.at<float>(0, 2) = (1 - alpha) * center.x + beta * center.y;
    rot_mat.at<float>(1, 0) = beta;
    rot_mat.at<float>(1, 1) = alpha;
    rot_mat.at<float>(1, 2) = (1 - alpha) * center.y - beta * center.x;

    return rot_mat;
}


int main()
{
  cv::Mat imgBgr = cv::imread("/tmp/image.png");
  cv::Mat imgBgr2 = cv::imread("/tmp/image.png");
  cv::Mat imgBgr3 = cv::imread("/tmp/image.png");

  if(imgBgr2.empty()){
    std::cout << "Failed to read image \n";
    return -1;
  }
  if(imgBgr.empty()){
    std::cout << "Failed to read image \n";
    return -1;
  }
  if(imgBgr3.empty()){
    std::cout << "Failed to read image \n";
    return -1;
  }

  // GOLDEN
  float goldenscale = 1.0;
  float goldenangle = D_GOLDENANGLE_;

  // Döndürme matrisi oluştur
  cv::Point2f goldencenter(imgBgr.cols / 2.0, imgBgr.rows / 2.0);
  cv::Mat M = goldenGetRotationMatrix2D(goldencenter, goldenangle, goldenscale);
  // Ek kaydırma (y ekseninde 10 piksel)
  //M.at<float>(1, 2) += 100;

  cv::Mat imgOut=cv::Mat::zeros(imgBgr2.size(),imgBgr2.type());
  //imgOut=cv::Mat(imgBgr.size(),imgBgr.type());

  cv::Mat imgOutExt_=cv::Mat::zeros(cv::Size(64+imgBgr2.cols+64,64+imgBgr2.rows+64),imgBgr3.type());
  for (int y = 0; y < imgBgr.rows; y++) {
    for (int x = 0; x < imgBgr.cols; x++) {
      imgOutExt_.at<cv::Vec3b>(64+y,64+x)=imgBgr3.at<cv::Vec3b>(y,x);
    }
  }
  cv::imwrite("imgOutExt.png",imgOutExt_);

#if 1
  std::ofstream ofs201 {"ref201.txt"};
  std::ofstream ofs2 {"topLeftRef.txt"};
  cv::Mat refImgSrc_=cv::Mat(imgBgr2.size(),CV_8UC3);
  cv::Mat refImgRoi_=cv::Mat::zeros(imgBgr2.size(),CV_8UC3);

  cv::Mat rotRefAll_=cv::Mat(cv::Size(imgBgr2.size()),CV_8UC3);

  for (int y = 0; y < imgBgr.rows; y+=32) {
    for (int x = 0; x < imgBgr.cols; x+=32) {
      float srcX[32][32];
      float srcY[32][32];
      std::ofstream ofs {"refXY+"+std::to_string(y)+"+"+std::to_string(x)+".txt"};

      auto topLeftXSet_ {false};
      auto topLeftYSet_ {false};
      int topLeftX_;
      int topLeftY_;

      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          srcX[J][K] = M.at<float>(0, 0) * (K+x) + M.at<float>(0, 1) * (J+y) + M.at<float>(0, 2);
          srcY[J][K] = M.at<float>(1, 0) * (K+x) + M.at<float>(1, 1) * (J+y) + M.at<float>(1, 2);
          const auto p_ {cv::Point(srcX[J][K],srcY[J][K])};
          if(!topLeftXSet_){
            topLeftXSet_=true;
            topLeftX_=p_.x;
          } else if(topLeftX_>p_.x){
            topLeftX_=p_.x;
          }
          if(!topLeftYSet_){
            topLeftYSet_=true;
            topLeftY_=p_.y;
          } else if(topLeftY_>p_.y){
            topLeftY_=p_.y;
          }
          ofs << "X: " << p_.x << ", Y: " << p_.y << '\n';
        }
      }
      ofs2<<"tX: "<<topLeftX_<<", tY: "<<topLeftY_<<'\n';

      int topLeftXAxi_;
      int topLeftYAxi_;
      if((topLeftY_+64)>=0&&topLeftY_<imgOut.rows&&(topLeftX_+64)>=0&&topLeftX_<imgOut.cols){
        topLeftXAxi_=topLeftX_;
        topLeftYAxi_=topLeftY_;
      } else {
        topLeftXAxi_=-64;
        topLeftYAxi_=-64;
      }
      cv::Mat top64x64_=cv::Mat::zeros(64,64,CV_8UC3);
      for(auto JJ_=topLeftYAxi_;JJ_<topLeftYAxi_+64;++JJ_){
        for(auto KK_=topLeftXAxi_;KK_<topLeftXAxi_+64;++KK_){
          top64x64_.at<cv::Vec3b>(JJ_-topLeftYAxi_,KK_-topLeftXAxi_)=imgOutExt_.at<cv::Vec3b>(JJ_+64,KK_+64);
        }
      }
      static auto cntr3_ {0};
      cv::imwrite("topRef64x64+"+std::to_string(cntr3_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",top64x64_);
      ++cntr3_;

      cv::Mat rotRef_=cv::Mat(32,32,CV_8UC3);
      for(auto yy_=0;yy_<32;++yy_){
        for(auto xx_=0;xx_<32;++xx_){
          const auto p_ {cv::Point(srcX[yy_][xx_],srcY[yy_][xx_])};
          rotRef_.at<cv::Vec3b>(yy_,xx_)=top64x64_.at<cv::Vec3b>(p_.y-topLeftY_,p_.x-topLeftX_);
          rotRefAll_.at<cv::Vec3b>(y+yy_,x+xx_)=top64x64_.at<cv::Vec3b>(p_.y-topLeftY_,p_.x-topLeftX_);
        }
      }
      static auto rotCntr_ {0};
      cv::imwrite("rotRef+"+std::to_string(rotCntr_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",rotRef_);
      cv::imwrite("rotRefAll+"+std::to_string(rotCntr_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",rotRefAll_);
      ++rotCntr_;

      cv::Mat ref32x32Img_=cv::Mat(32,32,CV_8UC3);
      cv::Mat ref32x32ImgSrc_=cv::Mat(32,32,CV_8UC3);
      static auto cntr_ {0};
      for(auto J_=0;J_<32;++J_){
        for(auto K_=0;K_<32;++K_){
          ref32x32ImgSrc_.at<cv::Vec3b>(J_,K_)=imgBgr2.at<cv::Vec3b>(J_+y,K_+x);
          refImgSrc_.at<cv::Vec3b>(J_+y,K_+x)=imgBgr2.at<cv::Vec3b>(J_+y,K_+x);
          if (srcX[J_][K_] >= 0 && srcX[J_][K_] < imgBgr2.cols && srcY[J_][K_] >= 0 && srcY[J_][K_] < imgBgr2.rows) {
            imgOut.at<cv::Vec3b>(J_+y,K_+x) = imgBgr2.at<cv::Vec3b>(cv::Point(srcX[J_][K_], srcY[J_][K_]));
            ref32x32Img_.at<cv::Vec3b>(J_,K_)=imgOut.at<cv::Vec3b>(J_+y,K_+x);
            refImgRoi_.at<cv::Vec3b>(J_+y,K_+x)=imgOut.at<cv::Vec3b>(J_+y,K_+x);
            if(201==cntr_){
              cv::Point p_=cv::Point(srcX[J_][K_], srcY[J_][K_]);
              ofs201<<"x: "<< p_.x << ", y: " << p_.y << ", val: " << imgOut.at<cv::Vec3b>(J_+y,K_+x) << '\n';
            }
          } else {
            imgOut.at<cv::Vec3b>(J_+y,K_+x) = 0x0;
            ref32x32Img_.at<cv::Vec3b>(J_,K_)=0x0;
            refImgRoi_.at<cv::Vec3b>(J_+y,K_+x)=0x0;
            if(201==cntr_){
              cv::Point p_=cv::Point(srcX[J_][K_], srcY[J_][K_]);
              ofs201<<"x: "<< p_.x << ", y: " << p_.y << ", val: " << 0 << '\n';
            }
          }
        }
      }
      cv::imwrite("ref32x32+"+std::to_string(cntr_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",ref32x32Img_);
      cv::imwrite("ref32x32Src+"+std::to_string(cntr_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",ref32x32ImgSrc_);
      cv::imwrite("refRoi+"+std::to_string(cntr_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",refImgRoi_);
      cv::imwrite("refSrc+"+std::to_string(cntr_)+"+"+std::to_string(y)+"+"+std::to_string(x)+".png",refImgSrc_);
      ++cntr_;
    }
  }
#endif

  const std::string rotatedImageGoldenStr {"golden_"+std::to_string(goldenangle)+".jpg"};
  cv::imwrite(rotatedImageGoldenStr,imgOut);

  // HLS
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls_[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls2_[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> > srcStream_("srcStream");
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_STRM_IN_PPC_;++K_){
      ap_axiu<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> srcStreamPix_;
      for(auto Z_=0;Z_<D_STRM_IN_PPC_;++Z_){
        cv::Vec3b cvPix_=imgBgr.at<cv::Vec3b>(J_,K_*D_STRM_IN_PPC_+Z_);
        ap_uint<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,1>::Value> pix_;
        pix_(7,0)=cvPix_[0];
        pix_(15,8)=cvPix_[1];
        pix_(23,16)=cvPix_[2];
        srcStreamPix_.data(Z_*D_STRM_IN_CHANNELS_*D_DEPTH_+D_STRM_IN_CHANNELS_*D_DEPTH_-1,Z_*D_STRM_IN_CHANNELS_*D_DEPTH_)=pix_;
      }
      srcStream_<<srcStreamPix_;
    }
  }

  cv::Mat imgBgrOut_=cv::Mat(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b Pix_=imgBgr.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        imgBgrOut_.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=imgBgr.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0)=Pix_[0];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8)=Pix_[1];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16)=Pix_[2];
      }
      imgHls2_[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)]=hlsPix_;
    }
  }
  cv::imwrite("imgBgrOut.png",imgBgrOut_);
 
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width_=imgBgr.cols;
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height_=imgBgr.rows;
  fp_struct<D_FP_T_> M00_=fp_struct<D_FP_T_>(static_cast<D_FP_T_>(M.at<float>(0,0)));
  fp_struct<D_FP_T_> M01_=fp_struct<D_FP_T_>(static_cast<D_FP_T_>(M.at<float>(0,1)));
  fp_struct<D_FP_T_> M02_=fp_struct<D_FP_T_>(static_cast<D_FP_T_>(M.at<float>(0,2)));
  fp_struct<D_FP_T_> M10_=fp_struct<D_FP_T_>(static_cast<D_FP_T_>(M.at<float>(1,0)));
  fp_struct<D_FP_T_> M11_=fp_struct<D_FP_T_>(static_cast<D_FP_T_>(M.at<float>(1,1)));
  fp_struct<D_FP_T_> M12_=fp_struct<D_FP_T_>(static_cast<D_FP_T_>(M.at<float>(1,2)));

  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> affWrAxi_[D_AFFWRAXI_DEPTH_];
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidRdAxi_[D_VIDRDAXI_DEPTH_];
  for(auto J_=0;J_<imgOut.rows;++J_){
    for(auto K_=0;K_<imgOut.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b Pix_=imgOut.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0)=Pix_[0];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8)=Pix_[1];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16)=Pix_[2];
      }
      vidRdAxi_[J_*(D_MAX_STRIDE_/D_MM_PPC_)+K_]=hlsPix_;
    }
  }

  ap_uint<Bit_Width<D_MAX_COLS_>::Value> padWidth_=width_;
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> padHeight_=height_;
  if(padHeight_%D_BLOCK_SIZE_){
    padHeight_+=D_BLOCK_SIZE_-(padHeight_%D_BLOCK_SIZE_);
  }
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> > dstStream_("dstStream");

  D_TOP_(
    imgHls_,
    imgHls2_,
    affWrAxi_,
    vidRdAxi_,
    srcStream_,
    dstStream_,
    width_,
    height_,
    padWidth_,
    padHeight_,
    M00_.data(),M01_.data(),M02_.data(),
    M10_.data(),M11_.data(),M12_.data()
  );

  cv::Mat cvImgHlsOut2Post_=cv::Mat(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      hlsPix_=imgHls2_[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)];
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b cvPix_;
        cvPix_[0]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0);
        cvPix_[1]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8);
        cvPix_[2]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16);
        cvImgHlsOut2Post_.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=cvPix_;
      }
    }
  }
  cv::imwrite("imgHls2.png",cvImgHlsOut2Post_);

  cv::Mat cvImgDstBramOut_=cv::Mat(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      hlsPix_=affWrAxi_[(J_)*(D_MAX_STRIDE_/D_MM_PPC_)+K_];
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b cvPix_;
        cvPix_[0]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0);
        cvPix_[1]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8);
        cvPix_[2]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16);
        cvImgDstBramOut_.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=cvPix_;
      }
    }
  }
  cv::imwrite("imgDstBramOut.png",cvImgDstBramOut_);

  cv::Mat cvImgDstBram2Out_=cv::Mat(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      hlsPix_=vidRdAxi_[J_*(D_MAX_STRIDE_/D_MM_PPC_)+K_];
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b cvPix_;
        cvPix_[0]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0);
        cvPix_[1]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8);
        cvPix_[2]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16);
        cvImgDstBram2Out_.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=cvPix_;
      }
    }
  }
  cv::imwrite("imgDstBram2Out.png",cvImgDstBram2Out_);

  cv::Mat imgBgrNewOut_;
  imgBgrNewOut_=cv::Mat(imgBgr.size(),imgBgr.type());
  std::ofstream ofs {"log.txt"};
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_STRM_OUT_PPC_;++K_){
      ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> dstStreamPix_;
      dstStream_>>dstStreamPix_;
      ofs<<"J_: "<<J_<<", K_: "<<K_<<", last: "<<dstStreamPix_.last<<", user: "<<dstStreamPix_.user<<'\n';
      for(auto Z_=0;Z_<D_STRM_OUT_PPC_;++Z_){
        ap_uint<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,1>::Value> pix_;
        pix_=dstStreamPix_.data(Z_*D_STRM_OUT_CHANNELS_*D_DEPTH_+D_STRM_OUT_CHANNELS_*D_DEPTH_-1,Z_*D_STRM_OUT_CHANNELS_*D_DEPTH_);
        cv::Vec3b cvPix_=imgBgr.at<cv::Vec3b>(J_,K_*D_STRM_IN_PPC_+Z_);
        cvPix_[0]=pix_(7,0);
        cvPix_[1]=pix_(15,8);
        cvPix_[2]=pix_(23,16);
        imgBgrNewOut_.at<cv::Vec3b>(J_,K_*D_STRM_OUT_PPC_+Z_)=cvPix_;
      }
    }
  }
  cv::imwrite("hls.png",imgBgrNewOut_);

  return 0;
}
