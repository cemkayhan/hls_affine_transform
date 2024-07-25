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

    cv::Mat rot_mat = cv::Mat::zeros(2, 3, CV_64F); //2x3 matrix oluşturuyoruz
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

  if(imgBgr.empty()){
    std::cout << "Failed to read image \n";
    return -1;
  }

  // GOLDEN
  float goldenscale = 1.0;
  float goldenangle = 129.0;

  // Döndürme matrisi oluştur
  cv::Point2f goldencenter(imgBgr.cols / 2.0, imgBgr.rows / 2.0);
  cv::Mat M = goldenGetRotationMatrix2D(goldencenter, goldenangle, goldenscale);
  // Ek kaydırma (y ekseninde 10 piksel)
  //M.at<float>(1, 2) += 100;

  cv::Mat imgOut;
  imgOut=cv::Mat(imgBgr.size(),imgBgr.type());

  for (int y = 0; y < imgBgr.rows; y+=32) {
    for (int x = 0; x < imgBgr.cols; x+=32) {
      cv::Mat imgTmp(cv::Size(32,32),imgBgr.type());
      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          imgTmp.at<cv::Vec3b>(J,K)=imgBgr.at<cv::Vec3b>(J+y,K+x);
        }
      }

      float srcX[32][32];
      float srcY[32][32];
      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          srcX[J][K] = M.at<float>(0, 0) * (K+x) + M.at<float>(0, 1) * (J+y) + M.at<float>(0, 2);
          srcY[J][K] = M.at<float>(1, 0) * (K+x) + M.at<float>(1, 1) * (J+y) + M.at<float>(1, 2);
        }
      }

      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          if (srcX[J][K] >= 0 && srcX[J][K] < imgBgr.cols && srcY[J][K] >= 0 && srcY[J][K] < imgBgr.rows) {
              imgOut.at<cv::Vec3b>(J+y,K+x) = imgBgr.at<cv::Vec3b>(cv::Point(srcX[J][K], srcY[J][K]));
          } else {
              imgOut.at<cv::Vec3b>(J+y,K+x) = 0x0;
          }
        }
      }
    }
  }

  const std::string rotatedImageGoldenStr {"golden_"+std::to_string(goldenangle)+".jpg"};
  cv::imwrite(rotatedImageGoldenStr,imgOut);

  // HLS
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls2[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];

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

  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b Pix_=imgBgr.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0)=Pix_[0];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8)=Pix_[1];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16)=Pix_[2];
      }
      imgHls2[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)]=hlsPix_;
    }
  }
 
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width=imgBgr.cols;
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height=imgBgr.rows;
  fp_struct<float> M00_=fp_struct<float>(M.at<float>(0,0));
  fp_struct<float> M01_=fp_struct<float>(M.at<float>(0,1));
  fp_struct<float> M02_=fp_struct<float>(M.at<float>(0,2));
  fp_struct<float> M10_=fp_struct<float>(M.at<float>(1,0));
  fp_struct<float> M11_=fp_struct<float>(M.at<float>(1,1));
  fp_struct<float> M12_=fp_struct<float>(M.at<float>(1,2));

  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> dstBram_[D_MAX_ROWS_*(D_MAX_COLS_/D_MM_PPC_)];
  static ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> dstBram2_[D_MAX_ROWS_*(D_MAX_COLS_/D_MM_PPC_)];
  for(auto J_=0;J_<imgOut.rows;++J_){
    for(auto K_=0;K_<imgOut.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b Pix_=imgOut.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0)=Pix_[0];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8)=Pix_[1];
        hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16)=Pix_[2];
      }
      dstBram2_[J_*(D_MAX_COLS_/D_MM_PPC_)+K_]=hlsPix_;
    }
  }

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> > dstStream_("dstStream");
  D_TOP_(
    imgHls,
    imgHls2,
    dstBram_,
    dstBram2_,
    srcStream_,
    dstStream_,
    Width,
    Height,
    M00_.data(),M01_.data(),M02_.data(),
    M10_.data(),M11_.data(),M12_.data()
  );

  cv::Mat cvImgHlsOut_=cv::Mat::zeros(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      hlsPix_=imgHls[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)];
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b cvPix_;
        cvPix_[0]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+7,Z_*D_MM_CHANNELS_*D_DEPTH_+0);
        cvPix_[1]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+15,Z_*D_MM_CHANNELS_*D_DEPTH_+8);
        cvPix_[2]=hlsPix_(Z_*D_MM_CHANNELS_*D_DEPTH_+23,Z_*D_MM_CHANNELS_*D_DEPTH_+16);
        cvImgHlsOut_.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=cvPix_;
      }
    }
  }
  cv::imwrite("imgHlsOut.png",cvImgHlsOut_);

  cv::Mat cvImgDstBramOut_=cv::Mat::zeros(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      hlsPix_=dstBram_[(J_)*(D_MAX_COLS_/D_MM_PPC_)+K_];
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

  cv::Mat cvImgDstBram2Out_=cv::Mat::zeros(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      hlsPix_=dstBram2_[(J_)*(D_MAX_COLS_/D_MM_PPC_)+K_];
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
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_STRM_OUT_PPC_;++K_){
      ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> dstStreamPix_;
      dstStream_>>dstStreamPix_;
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
