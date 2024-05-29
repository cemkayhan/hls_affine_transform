#include "hls_vid_stabilization.h"
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "geometric_transform.h"
#include <string>

Geometric_Transform::Geometric_Transform(){
    
}

Geometric_Transform::~Geometric_Transform() {

}

// GOLDEN
cv::Mat goldenGetRotationMatrix2D(cv::Point2f center, double angle, double scale) {
    double alpha = scale * cos(angle * CV_PI / 180.0);
    double beta = scale * sin(angle * CV_PI / 180.0);

    cv::Mat rot_mat = cv::Mat::zeros(2, 3, CV_64F); //2x3 matrix oluşturuyoruz
    rot_mat.at<double>(0, 0) = alpha;
    rot_mat.at<double>(0, 1) = -beta;
    rot_mat.at<double>(0, 2) = (1 - alpha) * center.x + beta * center.y;
    rot_mat.at<double>(1, 0) = beta;
    rot_mat.at<double>(1, 1) = alpha;
    rot_mat.at<double>(1, 2) = (1 - alpha) * center.y - beta * center.x;

    return rot_mat;
}


void goldenWarpAffine(const cv::Mat& src, cv::Mat& dst, const cv::Mat& M, cv::Size dsize) {
    dst = cv::Mat::zeros(dsize, src.type());
    //std::cout << dst;
    for (int y = 0; y < dsize.height; ++y) {
        for (int x = 0; x < dsize.width; ++x) {    
            double srcX = M.at<double>(0, 0) * x + M.at<double>(0, 1) * y + M.at<double>(0, 2);
            double srcY = M.at<double>(1, 0) * x + M.at<double>(1, 1) * y + M.at<double>(1, 2);
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


cv::Mat Geometric_Transform::getRotationMatrix2D(cv::Point2f center, float angle, float scale) {
    float alpha = scale * cos(angle * float {CV_PI} / 180.0f);
    float beta = scale * sin(angle * float {CV_PI} / 180.0f);

    cv::Mat rot_mat = cv::Mat::zeros(2, 3, CV_64F); //2x3 matrix oluşturuyoruz
    rot_mat.at<float>(0, 0) = alpha;
    rot_mat.at<float>(0, 1) = -beta;
    rot_mat.at<float>(0, 2) = (1 - alpha) * center.x + beta * center.y;
    rot_mat.at<float>(1, 0) = beta;
    rot_mat.at<float>(1, 1) = alpha;
    rot_mat.at<float>(1, 2) = (1 - alpha) * center.y - beta * center.x;

    return rot_mat;
}


template<int PPC_>
void warpAffine(cv::Mat& src, cv::Mat& dst, const cv::Mat& M, cv::Size dsize
  ) {

    for (int y = 0; y < dsize.height; ++y) {
        for (int x = 0; x < dsize.width/PPC_; ++x) {    

            float srcX[PPC_];
            float srcY[PPC_];
            cv::Vec3b srcXY[PPC_];

            for(auto J=0;J<PPC_;++J){
              srcX[J] = M.at<float>(0, 0) * (x*PPC_+J) + M.at<float>(0, 1) * y + M.at<float>(0, 2);
              srcY[J] = M.at<float>(1, 0) * (x*PPC_+J) + M.at<float>(1, 1) * y + M.at<float>(1, 2);

            }

            for(auto J=0;J<PPC_;++J){
              if (srcX[J] >= 0 && srcX[J] < src.cols && srcY[J] >= 0 && srcY[J] < src.rows) {


                srcXY[J] = src.at<cv::Vec3b>(cv::Point(srcX[J], srcY[J]));


              } else {
                srcXY[J] = 0x80;
              }
            }

            for(auto J=0;J<PPC_;++J){
                  dst.at<cv::Vec3b>(y, x*PPC_+J) = srcXY[J];
            }
        }
        
    }
}

int testFunction(
  ap_uint<Type_Width<D_FP_T_>::Value> Angle,
  ap_uint<Type_Width<D_FP_T_>::Value> Scale,
  float angle, float scale,
  int index,double goldenangle
) {
    cv::Mat imageBgr = cv::imread("/tmp/image.png");

    if(imageBgr.empty()){
      std::cout << "Failed to read image \n";
      return 1;
    }

    //cv::Mat imageUyvy(imageBgr.size(), CV_8UC2);
    //for (int y = 0; y < imageUyvy.rows; y++) {
    //    for (int x = 0; x < imageUyvy.cols; x += 2) {
    //        imageUyvy.at<cv::Vec2b>(y, x)[1] = imageBgr.at<cv::Vec3b>(y, x)[1];  // U
    //        imageUyvy.at<cv::Vec2b>(y, x)[0] = imageBgr.at<cv::Vec3b>(y, x)[0];  // Y
    //        imageUyvy.at<cv::Vec2b>(y, x + 1)[1] = imageBgr.at<cv::Vec3b>(y, x + 1)[1];  // V
    //        imageUyvy.at<cv::Vec2b>(y, x + 1)[0] = imageBgr.at<cv::Vec3b>(y, x)[0];  // Y
    //    }
    //}

    Geometric_Transform geo;
    cv::Point2f center(imageBgr.cols / 2.0, imageBgr.rows / 2.0);
    //float scale = 1.0;

    // Döndürme matrisi oluştur
    cv::Mat rotMat = geo.getRotationMatrix2D(center, angle, scale);
    // Ek kaydırma (y ekseninde 10 piksel)
    rotMat.at<float>(1, 2) += 100;

    static ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> S2mm[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_];
    for(auto Y=0;Y<imageBgr.rows;++Y){
      for(auto X=0;X<imageBgr.cols/D_PPC_;++X){
        ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> S2mm_;
        for(auto J=0;J<D_PPC_;++J){
          cv::Vec3b Pix=imageBgr.at<cv::Vec3b>(Y,X*D_PPC_+J);
          S2mm_(D_COLOR_CHANNELS_*D_DEPTH_*J+D_COLOR_CHANNELS_*D_DEPTH_-1,D_COLOR_CHANNELS_*D_DEPTH_*J)=ap_uint<D_COLOR_CHANNELS_*D_DEPTH_> {(ap_uint<8> {0}, ap_uint<8> {Pix[2]},ap_uint<8> {Pix[1]},ap_uint<8> {Pix[0]})};
        }
        S2mm[(D_MAX_STRIDE_/D_PPC_)*Y+X]=S2mm_;
      }
    }

#if 1
    for(auto Y=0;Y<imageBgr.rows;++Y){
      for(auto X=0;X<imageBgr.cols/D_PPC_;++X){
        ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> S2mm_=S2mm[(D_MAX_STRIDE_/D_PPC_)*Y+X];
        for(auto J=0;J<D_PPC_;++J){
          ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> S2mm_Pix_;
          cv::Vec3b Pix_;
      
          S2mm_Pix_=S2mm_(D_COLOR_CHANNELS_*D_DEPTH_*J+D_COLOR_CHANNELS_*D_DEPTH_-1,D_COLOR_CHANNELS_*D_DEPTH_*J);
          Pix_[0]=S2mm_Pix_(7,0);
          Pix_[1]=S2mm_Pix_(15,8);
          Pix_[2]=S2mm_Pix_(23,16);
          imageBgr.at<cv::Vec3b>(Y,X*D_PPC_+J)=Pix_;
        }
      }
    }
#endif

    static ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Mm2s[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_];
    D_TOP_(
      S2mm,
      Mm2s,
      imageBgr.cols,
      imageBgr.rows,
      100,
      Angle,
      Scale
    );

    cv::Mat imageBgrRotated(imageBgr.size(), CV_8UC3);
    for(auto Y=0;Y<imageBgrRotated.rows;++Y){
      for(auto X=0;X<imageBgrRotated.cols/D_PPC_;++X){
        ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> Mm2s_=Mm2s[(D_MAX_STRIDE_/D_PPC_)*Y+X];
        for(auto J=0;J<D_PPC_;++J){
          cv::Vec3b Pix_;
          ap_uint<D_COLOR_CHANNELS_*D_DEPTH_> Mm2s_Pix_;

          Mm2s_Pix_=Mm2s_(D_COLOR_CHANNELS_*D_DEPTH_*J+D_COLOR_CHANNELS_*D_DEPTH_-1,D_COLOR_CHANNELS_*D_DEPTH_*J);
          Pix_[0]=Mm2s_Pix_(7,0);
          Pix_[1]=Mm2s_Pix_(15,8);
          Pix_[2]=Mm2s_Pix_(23,16);
          imageBgrRotated.at<cv::Vec3b>(Y,D_PPC_*X+J)=Pix_;
        }
      }
    }

    //cv::Mat imageBgrRotatedx;
    //cv::cvtColor(imageUyvyRotated, imageBgrRotatedx, cv::COLOR_YUV2BGR_UYVY);
    const std::string imageBgrRotatedStr {std::to_string(index)+"hls_"+std::to_string(angle)+".jpg"};
    cv::imwrite(imageBgrRotatedStr, imageBgrRotated);

    cv::Mat rotatedImageBgr;
    rotatedImageBgr=cv::Mat(imageBgr.size(),imageBgr.type());
    warpAffine<D_PPC_>(imageBgr, rotatedImageBgr, rotMat, imageBgr.size());

    //cv::Mat rotatedImageBgr(imageBgr.size(),CV_8UC3);
    //cv::cvtColor(rotatedImageBgr, rotatedImageBgr, cv::COLOR_YUV2BGR_UYVY);
    const std::string rotatedImageBgrStr {std::to_string(index)+"ref_"+std::to_string(angle)+".jpg"};
    cv::imwrite(rotatedImageBgrStr,rotatedImageBgr);


    // GOLDEN
    double goldenscale = 1.0;

    // Döndürme matrisi oluştur
    cv::Point2f goldencenter(imageBgr.cols / 2.0, imageBgr.rows / 2.0);
    cv::Mat goldenRotMat = goldenGetRotationMatrix2D(goldencenter, goldenangle, goldenscale);
    // Ek kaydırma (y ekseninde 10 piksel)
    goldenRotMat.at<double>(1, 2) += 100;

    cv::Mat rotatedImageBgrGolden;
    rotatedImageBgrGolden=cv::Mat(imageBgr.size(),imageBgr.type());
    goldenWarpAffine(imageBgr, rotatedImageBgrGolden, goldenRotMat, imageBgr.size());

    //cv::Mat rotatedImageBgrGolden(imageBgr.size(),CV_8UC3);
    //cv::cvtColor(rotatedImageGolden,rotatedImageBgrGolden, cv::COLOR_YUV2BGR_UYVY);
    const std::string rotatedImageGoldenStr {std::to_string(index)+"golden_"+std::to_string(goldenangle)+".jpg"};
    cv::imwrite(rotatedImageGoldenStr, rotatedImageBgrGolden);

    // GOLDEN


   return 0;
}

int main()
{
  double goldenangle = 1.0;
  int result;
  int index=0;

  for(float angle=float(goldenangle); angle<360.0f;angle+=1.0f){
    float scale=1.0f;
    fp_struct<D_FP_T_> Angle=fp_struct<D_FP_T_>(D_FP_T_(angle));
    fp_struct<D_FP_T_> Scale=fp_struct<D_FP_T_>(D_FP_T_(scale));
    result=testFunction(Angle.data(),Scale.data(),angle,scale,index,goldenangle);
    ++index;
    goldenangle+=1.0;
  }

  return result;
}
