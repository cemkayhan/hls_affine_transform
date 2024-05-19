#include "hls_vid_stabilization.h"
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "geometric_transform.h"

Geometric_Transform::Geometric_Transform(){
    
}

Geometric_Transform::~Geometric_Transform() {

}

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

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
  static std::ofstream ofs {"rotateFrameReference.txt"}; 
#endif

    for (int y = 0; y < dsize.height; ++y) {
        for (int x = 0; x < dsize.width/PPC_; ++x) {    

            float srcX[PPC_];
            float srcY[PPC_];
            cv::Vec2b srcXY[PPC_];

            for(auto J=0;J<PPC_;++J){
              srcX[J] = M.at<float>(0, 0) * (x*PPC_+J) + M.at<float>(0, 1) * y + M.at<float>(0, 2);
              srcY[J] = M.at<float>(1, 0) * (x*PPC_+J) + M.at<float>(1, 1) * y + M.at<float>(1, 2);

            }

            for(auto J=0;J<PPC_;++J){
              if (srcX[J] >= 0 && srcX[J] < src.cols && srcY[J] >= 0 && srcY[J] < src.rows) {


                srcXY[J] = src.at<cv::Vec2b>(cv::Point(srcX[J], srcY[J]));


#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
              cv::Point Pnt=cv::Point(srcX[J], srcY[J]);
              ofs << "X[" << J << "]: " << std::fixed << std::setprecision(12) << srcX[J] << ", "
                  << "Y[" << J << "]: " << std::fixed << std::setprecision(12) << srcY[J] << '\n';
              ofs << "X[" << J << "]: " << Pnt.x << ", "
                  << "Y[" << J << "]: " << Pnt.y << '\n';
              ofs << "X[" << J << "]: " << +srcXY[J][1] << ", "
                  << "Y[" << J << "]: " << +srcXY[J][0] << '\n';
#endif



              } else {
                srcXY[J] = 0x80;
              }
            }

            for(auto J=0;J<PPC_;++J){
                  dst.at<cv::Vec2b>(y, x*PPC_+J) = srcXY[J];
            }
        }
        
    }
}

int main()
{
    cv::Mat imageBgr = cv::imread("/tmp/image.png");

    if(imageBgr.empty()){
      std::cout << "Failed to read image \n";
      return 1;
    }

    cv::Mat imageUyvy(imageBgr.size(), CV_8UC2);
    for (int y = 0; y < imageUyvy.rows; y++) {
        for (int x = 0; x < imageUyvy.cols; x += 2) {
            imageUyvy.at<cv::Vec2b>(y, x)[1] = imageBgr.at<cv::Vec3b>(y, x)[1];  // U
            imageUyvy.at<cv::Vec2b>(y, x)[0] = imageBgr.at<cv::Vec3b>(y, x)[0];  // Y
            imageUyvy.at<cv::Vec2b>(y, x + 1)[1] = imageBgr.at<cv::Vec3b>(y, x + 1)[1];  // V
            imageUyvy.at<cv::Vec2b>(y, x + 1)[0] = imageBgr.at<cv::Vec3b>(y, x)[0];  // Y
        }
    }

#if 1
#endif

    Geometric_Transform geo;
    cv::Point2f center(imageUyvy.cols / 2.0, imageUyvy.rows / 2.0);
    float angle = 45.0;
    float scale = 1.0;

    // Döndürme matrisi oluştur
    cv::Mat rotMat = geo.getRotationMatrix2D(center, angle, scale);

    // Ek kaydırma (y ekseninde 10 piksel)
    rotMat.at<float>(1, 2) += 100;

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
  std::cout << "RefMat00: " << std::fixed << std::setprecision(12) << rotMat.at<float>(0,0) << '\n';
  std::cout << "RefMat01: " << std::fixed << std::setprecision(12) << rotMat.at<float>(0,1) << '\n';
  std::cout << "RefMat02: " << std::fixed << std::setprecision(12) << rotMat.at<float>(0,2) << '\n';
  std::cout << "RefMat10: " << std::fixed << std::setprecision(12) << rotMat.at<float>(1,0) << '\n';
  std::cout << "RefMat11: " << std::fixed << std::setprecision(12) << rotMat.at<float>(1,1) << '\n';
  std::cout << "RefMat12: " << std::fixed << std::setprecision(12) << rotMat.at<float>(1,2) << '\n';
    //float alphax = scale * cos(angle * float {CV_PI} / 180.0f);
    //float betax = scale * sin(angle * float {CV_PI} / 180.0f);
  //std::cout << "RefCenterX: " << std::fixed << std::setprecision(12) << alphax << '\n';
  //std::cout << "RefCenterY: " << std::fixed << std::setprecision(12) << angle * float {CV_PI} / 180.0f << '\n';
#endif




#if 1
    static ap_uint<2*D_DEPTH_*D_PPC_> S2mm[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_];
    for(auto Y=0;Y<imageUyvy.rows;++Y){
      for(auto X=0;X<imageUyvy.cols/8;++X){
        ap_uint<2*D_DEPTH_*D_PPC_> S2mm_;
#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
        static std::ofstream ofs1 {"frameReferenceUyvy.txt"}; 
#endif
        for(auto J=0;J<D_PPC_;++J){
          cv::Vec2b Pix=imageUyvy.at<cv::Vec2b>(Y,X*8+J);
          S2mm_(2*D_DEPTH_*J+2*D_DEPTH_-1,2*D_DEPTH_*J)=ap_uint<2*D_DEPTH_> {(ap_uint<8> {Pix[1]},ap_uint<8> {Pix[0]})};
          ofs1 << "X[" << J << "]: " << X*8+J << ", "
               << "Y[" << J << "]: " << Y << '\n';
          ofs1 << "X[" << J << "]: " << ap_uint<8> {Pix[1]} << ", "
               << "Y[" << J << "]: " << ap_uint<8> {Pix[0]} << '\n';
        }
#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
        ofs1 << S2mm_.to_string(16) << '\n';
#endif
        S2mm[(D_MAX_STRIDE_/D_PPC_)*Y+X]=S2mm_;
      }
    }

#if 1
    for(auto Y=0;Y<imageUyvy.rows;++Y){
      for(auto X=0;X<imageUyvy.cols/8;++X){
        ap_uint<2*D_DEPTH_*D_PPC_> S2mm_=S2mm[(D_MAX_STRIDE_/D_PPC_)*Y+X];
        for(auto J=0;J<8;++J){
          cv::Vec2b Pix;
          ap_uint<16> Pix_=S2mm_(2*D_DEPTH_*J+2*D_DEPTH_-1,2*D_DEPTH_*J);
          Pix[0]=Pix_(7,0);
          Pix[1]=Pix_(15,8);
#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_DEBUG_
          static std::ofstream ofs1 {"frameReferenceS2mm.txt"}; 
          ofs1 << "X[" << J << "]: " << X*8+J << ", "
               << "Y[" << J << "]: " << Y << '\n';
          ofs1 << "X[" << J << "]: " << ap_uint<8> {Pix[1]} << ", "
               << "Y[" << J << "]: " << ap_uint<8> {Pix[0]} << '\n';
#endif
          imageUyvy.at<cv::Vec2b>(Y,8*X+J)=Pix;
        }
      }
    }

#endif

    static ap_uint<2*D_DEPTH_*D_PPC_> Mm2s[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_];
    D_TOP_(
      S2mm,
      Mm2s,
      imageUyvy.cols,
      imageUyvy.rows,
      100,
      angle,
      scale
    );

    cv::Mat imageUyvyRotated(imageUyvy.size(), CV_8UC2);
    for(auto Y=0;Y<imageUyvyRotated.rows;++Y){
      for(auto X=0;X<imageUyvyRotated.cols/D_PPC_;++X){
        ap_uint<2*D_DEPTH_*D_PPC_> Mm2s_=Mm2s[(D_MAX_STRIDE_/D_PPC_)*Y+X];
        for(auto J=0;J<D_PPC_;++J){
          cv::Vec2b Pix;
          ap_uint<16> Pix_=Mm2s_(2*D_DEPTH_*J+2*D_DEPTH_-1,2*D_DEPTH_*J);
          Pix[0]=Pix_(7,0);
          Pix[1]=Pix_(15,8);
          imageUyvyRotated.at<cv::Vec2b>(Y,D_PPC_*X+J)=Pix;
        }
      }
    }

    cv::Mat imageBgrRotatedx;
    cv::cvtColor(imageUyvyRotated, imageBgrRotatedx, cv::COLOR_YUV2BGR_UYVY);
    cv::imwrite("imageBgrRotatedx.jpg", imageBgrRotatedx);
#endif

    // Görüntüyü döndür ve kaydır
    cv::Mat rotatedImage;
    rotatedImage=cv::Mat(imageUyvy.size(),imageUyvy.type());
    warpAffine<8>(imageUyvy, rotatedImage, rotMat, imageUyvy.size());

    // Sonucu kaydet
    cv::Mat rotatedImageBgr(imageBgr.size(),CV_8UC3);
    cv::cvtColor(rotatedImage, rotatedImageBgr, cv::COLOR_YUV2BGR_UYVY);
    cv::imwrite("rotatedImageBgr.jpg", rotatedImageBgr);

   //cv::Mat channels[2];
    //cv::split(imageUyvy, channels);

#if 0
 for (int y = 0; y < imageUyvy.rows; y++) {
        for (int x = 0; x < imageUyvy.cols; x += 2) {
            // Extract UYVY values
            uchar Y0 = channels[0].at<uchar>(y, x);
            uchar Y1 = channels[0].at<uchar>(y, x + 1);
            uchar U = channels[1].at<uchar>(y, x);
            uchar V = channels[1].at<uchar>(y, x + 1);

            // Convert to BGR
            imageBgr.at<cv::Vec3b>(y, x)[0] = Y0 + 1.4075 * (V - 128);  // B
            imageBgr.at<cv::Vec3b>(y, x)[1] = Y0 - 0.3455 * (U - 128) - (0.7169 * (V - 128));  // G
            imageBgr.at<cv::Vec3b>(y, x)[2] = Y0 + 1.779 * (U - 128);  // R

            imageBgr.at<cv::Vec3b>(y, x + 1)[0] = Y1 + 1.4075 * (V - 128);  // B
            imageBgr.at<cv::Vec3b>(y, x + 1)[1] = Y1 - 0.3455 * (U - 128) - (0.7169 * (V - 128));  // G
            imageBgr.at<cv::Vec3b>(y, x + 1)[2] = Y1 + 1.779 * (U - 128);  // R
        }
    }
#endif

    cv::Mat imageBgrx;
    cv::cvtColor(imageUyvy, imageBgrx, cv::COLOR_YUV2BGR_UYVY);
    cv::imwrite("imageBgrx.jpg", imageBgrx);
    cv::imwrite("imageBgr.jpg", imageBgr);

}
