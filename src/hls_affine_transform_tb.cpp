#include "hls_affine_transform.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <string>

void yagizGolden(const cv::Mat& src, cv::Mat& dst, const cv::Mat& M, cv::Size dsize) {
    dst = cv::Mat::zeros(dsize, src.type());
    //std::cout << dst;
    for (int y = 0; y < dsize.height; ++y) {
        for (int x = 0; x < dsize.width; ++x) {    
            float srcX = M.at<float>(0, 0) * x + M.at<float>(0, 1) * y + M.at<float>(0, 2);
            float srcY = M.at<float>(1, 0) * x + M.at<float>(1, 1) * y + M.at<float>(1, 2);
            if (srcX >= 0 && srcX < src.cols && srcY >= 0 && srcY < src.rows) {
                dst.at<cv::Vec3b>(y, x) = src.at<cv::Vec3b>(cv::Point(srcX, srcY));
            } else {
                dst.at<cv::Vec3b>(y, x) = 0x80;
            }
        }
        
    }
    //std::cout << src;
    //std::cout << dst;
}

// GOLDEN
cv::Mat goldenGetRotationMatrix2D(cv::Point2f center, float angle, float scale) {
    float alpha = scale * hls::cosf(angle * CV_PI / 180.0);
    float beta = scale * hls::sinf(angle * CV_PI / 180.0);

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
  cv::Mat imgBgr2 = cv::imread("/tmp/image2.png");

  if(imgBgr2.empty()){
    std::cout << "Failed to read image \n";
    return -1;
  }
  auto colsNew=imgBgr2.cols;
  auto rowsNew=imgBgr2.rows;
  if(rowsNew%D_BLOCK_SIZE_){
    rowsNew+=D_BLOCK_SIZE_-(rowsNew%D_BLOCK_SIZE_);
  }
  cv::Mat imgBgrNew = cv::Mat::zeros(rowsNew,colsNew,CV_8UC3);
  for(auto I=0;I<imgBgr2.rows;++I){
    for(auto J=0;J<imgBgr2.cols;++J){
      imgBgrNew.at<cv::Vec3b>(I,J)=imgBgr2.at<cv::Vec3b>(I,J);
    }
  }

  // GOLDEN
  float goldenscale = 1.0;
  float goldenangle = D_GOLDENANGLE_;

  // Döndürme matrisi oluştur
  cv::Point2f goldencenter(imgBgrNew.cols / 2.0, imgBgrNew.rows / 2.0);
  cv::Mat M = goldenGetRotationMatrix2D(goldencenter, goldenangle, goldenscale);
  // Ek kaydırma (y ekseninde 10 piksel)
  //M.at<float>(1, 2) += 100;

  cv::Mat imgOut=cv::Mat::zeros(imgBgrNew.size(),imgBgrNew.type());

#if 1
  for (int y = 0; y < imgBgrNew.rows; y+=32) {
    for (int x = 0; x < imgBgrNew.cols; x+=32) {
      float srcX[32][32];
      float srcY[32][32];
      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          srcX[J][K] = M.at<float>(0, 0) * (K+x) + M.at<float>(0, 1) * (J+y) + M.at<float>(0, 2);
          srcY[J][K] = M.at<float>(1, 0) * (K+x) + M.at<float>(1, 1) * (J+y) + M.at<float>(1, 2);
         }
      }
      for(auto J_=0;J_<32;++J_){
        for(auto K_=0;K_<32;++K_){
          if (srcX[J_][K_] >= 0 && srcX[J_][K_] < imgBgrNew.cols && srcY[J_][K_] >= 0 && srcY[J_][K_] < imgBgrNew.rows) {
            imgOut.at<cv::Vec3b>(J_+y,K_+x) = imgBgrNew.at<cv::Vec3b>(cv::Point(srcX[J_][K_], srcY[J_][K_]));
          } else {
            imgOut.at<cv::Vec3b>(J_+y,K_+x) = 0x0;
          }
        }
      }
    }
  }
#endif

  const std::string rotatedImageGoldenStr {"golden_"+std::to_string(goldenangle)+".png"};
  cv::imwrite(rotatedImageGoldenStr,imgOut);

  // HLS
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls_[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls2_[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> > srcStream_("srcStream");
  for(auto J_=0;J_<imgBgrNew.rows;++J_){
    for(auto K_=0;K_<imgBgrNew.cols/D_STRM_IN_PPC_;++K_){
      ap_axiu<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> srcStreamPix_;
      for(auto Z_=0;Z_<D_STRM_IN_PPC_;++Z_){
        cv::Vec3b cvPix_=imgBgrNew.at<cv::Vec3b>(J_,K_*D_STRM_IN_PPC_+Z_);
        ap_uint<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,1>::Value> pix_;
        pix_(7,0)=cvPix_[0];
        pix_(15,8)=cvPix_[1];
        pix_(23,16)=cvPix_[2];
        srcStreamPix_.data(Z_*D_STRM_IN_CHANNELS_*D_DEPTH_+D_STRM_IN_CHANNELS_*D_DEPTH_-1,Z_*D_STRM_IN_CHANNELS_*D_DEPTH_)=pix_;
      }
      srcStream_<<srcStreamPix_;
    }
  }

  cv::Mat imgBgrOut_=cv::Mat(imgBgrNew.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgrNew.rows;++J_){
    for(auto K_=0;K_<imgBgrNew.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b Pix_=imgBgrNew.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        imgBgrOut_.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=imgBgrNew.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0)=Pix_[0];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8)=Pix_[1];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16)=Pix_[2];
      }
      imgHls2_[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)]=hlsPix_;
    }
  }
  cv::imwrite("imgBgrOut.png",imgBgrOut_);
 
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width_=imgBgrNew.cols;
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height_=imgBgrNew.rows;
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

  cv::Mat cvImgHlsOut2Post_=cv::Mat(imgBgrNew.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgrNew.rows;++J_){
    for(auto K_=0;K_<imgBgrNew.cols/D_MM_PPC_;++K_){
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

  cv::Mat cvImgDstBramOut_=cv::Mat(imgBgrNew.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgrNew.rows;++J_){
    for(auto K_=0;K_<imgBgrNew.cols/D_MM_PPC_;++K_){
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

  cv::Mat cvImgDstBram2Out_=cv::Mat(imgBgrNew.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgrNew.rows;++J_){
    for(auto K_=0;K_<imgBgrNew.cols/D_MM_PPC_;++K_){
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
  imgBgrNewOut_=cv::Mat(imgBgrNew.size(),imgBgrNew.type());
  for(auto J_=0;J_<imgBgrNew.rows;++J_){
    for(auto K_=0;K_<imgBgrNew.cols/D_STRM_OUT_PPC_;++K_){
      ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> dstStreamPix_;
      dstStream_>>dstStreamPix_;
      for(auto Z_=0;Z_<D_STRM_OUT_PPC_;++Z_){
        ap_uint<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,1>::Value> pix_;
        pix_=dstStreamPix_.data(Z_*D_STRM_OUT_CHANNELS_*D_DEPTH_+D_STRM_OUT_CHANNELS_*D_DEPTH_-1,Z_*D_STRM_OUT_CHANNELS_*D_DEPTH_);
        cv::Vec3b cvPix_=imgBgrNew.at<cv::Vec3b>(J_,K_*D_STRM_IN_PPC_+Z_);
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
