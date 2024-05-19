#ifndef GEOMETRIC_TRANSFORM_H
#define GEOMETRIC_TRANSFORM_H

#include <opencv2/opencv.hpp>


class Geometric_Transform {
public:
    Geometric_Transform();
    ~Geometric_Transform();
    cv::Mat getRotationMatrix2D(cv::Point2f center, float angle, float scale);
    void warpAffine(const cv::Mat& src, cv::Mat& dst, const cv::Mat& M, cv::Size dsize);


private:

};

#endif // GEOMETRIC_TRANSFORM_H
