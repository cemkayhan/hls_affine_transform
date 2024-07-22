#include "hls_vid_stabilization.h"
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

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
  float goldenangle = -29.0;

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
  static ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHls[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)];
  static ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHlsDst[(D_MAX_STRIDE_/D_MM_PPC_)*D_MAX_ROWS_];
  for(auto J_=0;J_<2*imgBgr.rows;++J_){
    for(auto K_=0;K_<2*imgBgr.cols;++K_){
      imgHls[J_*imgBgr.cols+K_]=0;
    }
  }
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> hlsPix_;
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b Pix_=imgBgr.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_);
        hlsPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+7,Z_*D_COLOR_CHANNELS_*D_DEPTH_+0)=Pix_[0];
        hlsPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+15,Z_*D_COLOR_CHANNELS_*D_DEPTH_+8)=Pix_[1];
        hlsPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+23,Z_*D_COLOR_CHANNELS_*D_DEPTH_+16)=Pix_[2];
      }
      imgHls[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)]=hlsPix_;
      imgHlsDst[J_*imgBgr.cols+K_]=0;
    }
  }
 
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width=imgBgr.cols;
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height=imgBgr.rows;
  float M00_=M.at<float>(0,0);
  float M01_=M.at<float>(0,1);
  float M02_=M.at<float>(0,2);
  float M10_=M.at<float>(1,0);
  float M11_=M.at<float>(1,1);
  float M12_=M.at<float>(1,2);
#if 1
  D_TOP_(
    imgHls,
    imgHlsDst,
    Width,
    Height,
    M00_,M01_,M02_,
    M10_,M11_,M12_
  );
  cv::Mat dstHlsImgOrig=cv::Mat(imgBgr.rows,imgBgr.cols,CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<(imgBgr.cols/D_MM_PPC_);++K_){
      ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHlsDstPix_;
      imgHlsDstPix_=imgHls[(J_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(K_+D_COLS_MARGIN_/D_MM_PPC_)];
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b pix_;
        pix_[0]=imgHlsDstPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+7,Z_*D_COLOR_CHANNELS_*D_DEPTH_+0);
        pix_[1]=imgHlsDstPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+15,Z_*D_COLOR_CHANNELS_*D_DEPTH_+8);
        pix_[2]=imgHlsDstPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+23,Z_*D_COLOR_CHANNELS_*D_DEPTH_+16);
        dstHlsImgOrig.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=pix_;
      }
    }
  }
  cv::imwrite("dstImgHlsOrig.png",dstHlsImgOrig);

  cv::Mat dstHlsImg=cv::Mat(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols/D_MM_PPC_;++K_){
      ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> imgHlsDstPix_;
      imgHlsDstPix_=imgHlsDst[J_*(D_MAX_STRIDE_/D_MM_PPC_)+K_];
      for(auto Z_=0;Z_<D_MM_PPC_;++Z_){
        cv::Vec3b pix_;
        pix_[0]=imgHlsDstPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+7,Z_*D_COLOR_CHANNELS_*D_DEPTH_+0);
        pix_[1]=imgHlsDstPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+15,Z_*D_COLOR_CHANNELS_*D_DEPTH_+8);
        pix_[2]=imgHlsDstPix_(Z_*D_COLOR_CHANNELS_*D_DEPTH_+23,Z_*D_COLOR_CHANNELS_*D_DEPTH_+16);
        dstHlsImg.at<cv::Vec3b>(J_,K_*D_MM_PPC_+Z_)=pix_;
      }
    }
  }
  cv::imwrite("dstImgHls.png",dstHlsImg);
#endif

  return 0;
}
