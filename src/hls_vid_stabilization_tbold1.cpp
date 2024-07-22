#include "hls_vid_stabilization.h"
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

//#include "geometric_transform.h"
#include <string>

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


cv::Mat getRotationMatrix2D(cv::Point2f center, float angle, float scale, float& alpha, float& beta) {
    alpha = scale * cos(angle * float {CV_PI} / 180.0f);
    beta = scale * sin(angle * float {CV_PI} / 180.0f);

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
  double goldenscale = 1.0;
  double goldenangle = -72.0;

  // Döndürme matrisi oluştur
  cv::Point2f goldencenter(imgBgr.cols / 2.0, imgBgr.rows / 2.0);
  cv::Mat M = goldenGetRotationMatrix2D(goldencenter, goldenangle, goldenscale);
  // Ek kaydırma (y ekseninde 10 piksel)
  M.at<double>(1, 2) += 100;

  cv::Mat imgOut;
  imgOut=cv::Mat(imgBgr.size(),imgBgr.type());

  cv::Mat dstImg_=cv::Mat::zeros(imgBgr.size(),CV_8UC3);
  cv::Mat dstImg2_=cv::Mat::zeros(imgBgr.size(),CV_8UC3);
  std::ofstream reflog {"reflog.txt"};
  cv::Mat tmpImgAll_=cv::Mat::zeros(imgBgr.size(),CV_8UC3);
  for (int y = 0; y < imgBgr.rows; y+=32) {
    for (int x = 0; x < imgBgr.cols; x+=32) {
      cv::Mat imgTmp(cv::Size(32,32),imgBgr.type());
      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          imgTmp.at<cv::Vec3b>(J,K)=imgBgr.at<cv::Vec3b>(J+y,K+x);
        }
      }

      double srcX[32][32];
      double srcY[32][32];
      std::ofstream ofsxy2 {"refxy2_"+std::to_string(y)+"_"+std::to_string(x)+".txt"};
      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          srcX[J][K] = M.at<double>(0, 0) * (K+x) + M.at<double>(0, 1) * (J+y) + M.at<double>(0, 2);
          srcY[J][K] = M.at<double>(1, 0) * (K+x) + M.at<double>(1, 1) * (J+y) + M.at<double>(1, 2);
          cv::Point pnt_=cv::Point(srcX[J][K], srcY[J][K]);
          static std::ofstream ofsxy {"refxy.txt"};
          ofsxy<<"X: "<<pnt_.x<<", Y: "<<pnt_.y<<'\n';
          ofsxy2<<"X: "<<pnt_.x<<", Y: "<<pnt_.y<<'\n';
        }
      }

      auto topLeftXSet_=false;
      auto topLeftYSet_=false;
      auto bottomRightXSet_=false;
      auto bottomRightYSet_=false;
      int topLeftX_;
      int topLeftY_;
      int bottomRightX_;
      int bottomRightY_;

      for(auto J=0;J<32;++J){
        for(auto K=0;K<32;++K){
          cv::Point pnt_=cv::Point(srcX[J][K], srcY[J][K]);
          if(!topLeftXSet_){
            topLeftXSet_=true;
            topLeftX_=pnt_.x;
          } else if(topLeftX_>pnt_.x){
            topLeftX_=pnt_.x;
          }
          if(!topLeftYSet_){
            topLeftYSet_=true;
            topLeftY_=pnt_.y;
          } else if(topLeftY_>pnt_.y){
            topLeftY_=pnt_.y;
          }
          if(!bottomRightXSet_){
            bottomRightXSet_=true;
            bottomRightX_=pnt_.x;
          } else if(bottomRightX_<pnt_.x){
            bottomRightX_=pnt_.x;
          }
          if(!bottomRightYSet_){
            bottomRightYSet_=true;
            bottomRightY_=pnt_.y;
          } else if(bottomRightY_<pnt_.y){
            bottomRightY_=pnt_.y;
          }
          if (srcX[J][K] >= 0 && srcX[J][K] < imgBgr.cols && srcY[J][K] >= 0 && srcY[J][K] < imgBgr.rows) {
              imgOut.at<cv::Vec3b>(J+y,K+x) = imgBgr.at<cv::Vec3b>(cv::Point(srcX[J][K], srcY[J][K]));
          } else {
              imgOut.at<cv::Vec3b>(J+y,K+x) = 0x0;
          }
        }
      }
      reflog<<"J_: "<<y<<", K_: "<<x<<", topX: "<<topLeftX_<<", topY: "<<topLeftY_<<", botX: "<<bottomRightX_<<", botY: "<<bottomRightY_<<'\n';

      cv::Mat rectImg_=cv::Mat::zeros(1024,1024,CV_8UC3);
      cv::Mat rectImg2_=cv::Mat::zeros(32,32,CV_8UC3);
      cv::Mat tmpImg_=cv::Mat::zeros(64,64,CV_8UC3);
      if(topLeftX_>=0&&topLeftY_>=0&&bottomRightX_<imgBgr.cols&&bottomRightY_<imgBgr.rows){
        cv::rectangle(rectImg_,cv::Point(topLeftX_,topLeftY_),cv::Point(bottomRightX_,bottomRightY_),cv::Scalar(255,0,0),1,cv::LINE_8);

        ///cv::Mat imgBlock_=cv::Mat::zeros(64,64,CV_8UC3);
        ///for(auto JJ_=topLeftY_;JJ_<bottomRightY_;++JJ_){
        ///  for(auto KK_=topLeftX_;KK_<bottomRightX_;++KK_){
        ///    imgBlock_.at<cv::Vec3b>(JJ_,KK_)=
        ///  }
        ///}

        for(auto JJ_=0;JJ_<32;++JJ_){
          for(auto KK_=0;KK_<32;++KK_){
            cv::Point pnt_=cv::Point(srcX[JJ_][KK_], srcY[JJ_][KK_]);
            cv::Point pnt2_=cv::Point(srcX[JJ_][KK_]-topLeftX_, srcY[JJ_][KK_]-topLeftY_);
            rectImg_.at<cv::Vec3b>(pnt_)=imgBgr.at<cv::Vec3b>(pnt_);
            //rectImg2_.at<cv::Vec3b>(pnt2_)=imgBgr.at<cv::Vec3b>(pnt_);
            dstImg_.at<cv::Vec3b>(JJ_+y,KK_+x)=rectImg_.at<cv::Vec3b>(pnt_);
            //tmpImg_.at<cv::Vec3b>(JJ_,KK_)=imgBgr.at<cv::Vec3b>(y+JJ_,x+KK_);
            tmpImg_.at<cv::Vec3b>(JJ_,KK_)=imgBgr.at<cv::Vec3b>(pnt_);
            tmpImgAll_.at<cv::Vec3b>(JJ_+y,KK_+x)=tmpImg_.at<cv::Vec3b>(JJ_,KK_);
          }
        }
        cv::imwrite("tmpImgRef_"+std::to_string(y)+"_"+std::to_string(x)+".png",tmpImg_);
        cv::Mat tmpImg2_=cv::Mat::zeros(64,64,CV_8UC3);
        //for(auto JJ_=topLeftY_;JJ_<bottomRightY_;++JJ_){
        //  for(auto KK_=topLeftX_;KK_<bottomRightX_;++KK_){
        //    tmpImg2_.at<cv::Vec3b>(JJ_-topLeftY_,KK_-topLeftX_)=imgBgr.at<cv::Vec3b>(JJ_,KK_);
        //  }
        //}
        for(auto JJ_=0;JJ_<32;++JJ_){
          for(auto KK_=0;KK_<32;++KK_){
            //tmpImg2_.at<cv::Vec3b>(JJ_,KK_)=imgBgr.at<cv::Vec3b>(y+JJ_,x+KK_);
            tmpImg2_.at<cv::Vec3b>(srcY[JJ_][KK_]-topLeftY_,srcX[JJ_][KK_]-topLeftX_)=imgBgr.at<cv::Vec3b>(y+JJ_,x+KK_);
            //dstImg2_.at<cv::Vec3b>(y+JJ_,x+KK_)=tmpImg2_.at<cv::Vec3b>(JJ_,KK_);
            //cv::Point pnt_=cv::Point(srcX[JJ_][KK_], srcY[JJ_][KK_]);
            //dstImg2_.at<cv::Vec3b>(pnt_)=tmpImg2_.at<cv::Vec3b>(JJ_,KK_);
          }
        }
        for(auto JJ_=topLeftY_;JJ_<bottomRightY_;++JJ_){
          for(auto KK_=topLeftX_;KK_<bottomRightX_;++KK_){
            dstImg2_.at<cv::Vec3b>(JJ_,KK_)=tmpImg2_.at<cv::Vec3b>(JJ_-topLeftY_,KK_-topLeftX_);
          }
        }
        cv::imwrite("tmpImgRef2_"+std::to_string(y)+"_"+std::to_string(x)+".png",tmpImg2_);
      }
        //for(auto J_=0;J_<32;++J_){
        //  for(auto K_=0;K_<32;++K_){
        //    cv::Point pnt_=cv::Point(srcX[J_][K_],srcY[J_][K_]);
        //    dstImg_.at<cv::Vec3b>(J_+y,K_+x)=imgBgr.at<cv::Vec3b>(pnt_);
        //  }
        //}
      cv::imwrite("rectImgRef_"+std::to_string(y)+"_"+std::to_string(x)+".png",rectImg_);
      cv::imwrite("rectImgRef2_"+std::to_string(y)+"_"+std::to_string(x)+".png",rectImg2_);
    }
  }
  cv::imwrite("dstImgRef.png",dstImg_);
  cv::imwrite("dstImgRef2.png",dstImg2_);
  cv::imwrite("tmpImgAll.png",tmpImgAll_);

  const std::string rotatedImageGoldenStr {"golden_"+std::to_string(goldenangle)+".jpg"};
  cv::imwrite(rotatedImageGoldenStr,imgOut);

  // HLS
  static ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> imgHls[(D_MAX_STRIDE_/D_PPC_)*(2*D_MAX_ROWS_)];
  static ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> imgHlsDst[(D_MAX_STRIDE_/D_PPC_)*D_MAX_ROWS_];
  for(auto J_=0;J_<2*imgBgr.rows;++J_){
    for(auto K_=0;K_<2*imgBgr.cols;++K_){
      imgHls[J_*imgBgr.cols+K_]=0;
    }
  }
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols;++K_){
      cv::Vec3b Pix_=imgBgr.at<cv::Vec3b>(J_,K_);
      ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> hlsPix_;
      hlsPix_(7,0)=Pix_[0];
      hlsPix_(15,8)=Pix_[1];
      hlsPix_(23,16)=Pix_[2];
      imgHls[(J_+imgBgr.rows/2)*(D_MAX_STRIDE_/D_PPC_)+(K_+imgBgr.cols/2)]=hlsPix_;
      imgHlsDst[J_*imgBgr.cols+K_]=0;
    }
  }
 
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width=imgBgr.cols;
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height=imgBgr.rows;
  double M00_=M.at<double>(0,0);
  double M01_=M.at<double>(0,1);
  double M02_=M.at<double>(0,2);
  double M10_=M.at<double>(1,0);
  double M11_=M.at<double>(1,1);
  double M12_=M.at<double>(1,2);
#if 1
  D_TOP_(
    imgHls,
    imgHlsDst,
    Width,
    Height,
    M00_,M01_,M02_,
    M10_,M11_,M12_
  );
  cv::Mat dstHlsImgOrig=cv::Mat(imgBgr.rows*2,imgBgr.cols*2,CV_8UC3);
  for(auto J_=0;J_<2*imgBgr.rows;++J_){
    for(auto K_=0;K_<2*imgBgr.cols;++K_){
      ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> imgHlsDstPix_;
      imgHlsDstPix_=imgHls[J_*(D_MAX_STRIDE_/D_PPC_)+K_];
      cv::Vec3b pix_;
      pix_[0]=imgHlsDstPix_(7,0);
      pix_[1]=imgHlsDstPix_(15,8);
      pix_[2]=imgHlsDstPix_(23,16);
      dstHlsImgOrig.at<cv::Vec3b>(J_,K_)=pix_;
    }
  }
  cv::imwrite("dstImgHlsOrig.png",dstHlsImgOrig);

  cv::Mat dstHlsImg=cv::Mat(imgBgr.size(),CV_8UC3);
  for(auto J_=0;J_<imgBgr.rows;++J_){
    for(auto K_=0;K_<imgBgr.cols;++K_){
      ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_PPC_> imgHlsDstPix_;
      imgHlsDstPix_=imgHlsDst[J_*(D_MAX_STRIDE_/D_PPC_)+K_];
      cv::Vec3b pix_;
      pix_[0]=imgHlsDstPix_(7,0);
      pix_[1]=imgHlsDstPix_(15,8);
      pix_[2]=imgHlsDstPix_(23,16);
      dstHlsImg.at<cv::Vec3b>(J_,K_)=pix_;
    }
  }
  cv::imwrite("dstImgHls.png",dstHlsImg);
#endif

  return 0;
}
