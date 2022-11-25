#ifndef guidedfilter_hpp
#define guidedfilter_hpp

#include <opencv2/core.hpp>

using namespace cv;

extern Mat GuidedFilter(Mat& I, Mat& p, Size kSize, float eps);

extern Mat guidedFilter(Mat& I, Mat& p, Size kSize, float eps);

extern Mat guidedFilter_gray(Mat& I, Mat& p, Size kSize, float eps);

extern Mat guidedFilter_color(Mat& I, Mat& p, Size kSize, float eps);

#endif /*guidedfilter.hpp*/
