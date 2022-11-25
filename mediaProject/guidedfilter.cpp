#include "guidedfilter.hpp"
#include <opencv2/imgproc.hpp>
#include <vector>

using namespace std;

Mat GuidedFilter(Mat& I, Mat& p, Size kSize, float eps) {
	vector<Mat> channel, result;
	Mat temp, q;
	if (p.channels() > 1) {
		split(p, channel);
		for (auto i = 0; i < channel.size(); i++) {
			temp = guidedFilter(I, channel[i], kSize, eps);
			result.push_back(temp);
		}
		merge(result, q);
	}
	else
		q = guidedFilter(I, p, kSize, eps);

	return q;
}

Mat guidedFilter(Mat& I, Mat& p, Size kSize, float eps) {

	if (I.channels() == 3)
		return guidedFilter_color(I, p, kSize, eps * eps);
	else if (I.channels() == 1)
		return guidedFilter_gray(I, p, kSize, eps * eps);
}

Mat guidedFilter_gray(Mat& I, Mat& p, Size kSize, float eps) {

	Mat mean_I, mean_p;
	boxFilter(I, mean_I, -1, kSize);
	boxFilter(p, mean_p, -1, kSize); // mean of I & p

	Mat cov_Ip, var_I, tmp1, tmp2;
	tmp1 = I - mean_I;
	tmp2 = p - mean_p;
	multiply(tmp1, tmp2, cov_Ip);
	boxFilter(cov_Ip, cov_Ip, -1, kSize); // covariance of I & p

	multiply(tmp1, tmp1, var_I);
	boxFilter(var_I, var_I, -1, kSize); // variance of I

	Mat a, b;
	divide(cov_Ip, var_I + eps, a);

	multiply(a, mean_I, tmp1);
	b = mean_p - tmp1;

	Mat A, B;
	boxFilter(a, A, -1, kSize);
	boxFilter(b, B, -1, kSize);

	Mat q;
	multiply(A, I, tmp1);
	q = tmp1 + B;
	return q;
}

// 3 channel(color) guidance image
Mat guidedFilter_color(Mat& I, Mat& p, Size kSize, float eps) {

	vector<Mat> bgr;
	split(I, bgr); // split I into 3 channel(r,g,b)

	Mat mean_p, mean_b, mean_g, mean_r;

	boxFilter(p, mean_p, -1, kSize);
	boxFilter(bgr[0], mean_b, -1, kSize);
	boxFilter(bgr[1], mean_g, -1, kSize);
	boxFilter(bgr[2], mean_r, -1, kSize); // mean of b,g,r(I)

	Mat cov_bp, cov_gp, cov_rp;
	Mat tmp1, tmp2;

	tmp1 = bgr[0] - mean_b;
	tmp2 = p - mean_p;
	multiply(tmp1, tmp2, cov_bp);
	boxFilter(cov_bp, cov_bp, -1, kSize); // covariance of b(I) & p

	tmp1 = bgr[1] - mean_g;
	tmp2 = p - mean_p;
	multiply(tmp1, tmp2, cov_gp);
	boxFilter(cov_gp, cov_gp, -1, kSize); // covariance of g(I) & p

	tmp1 = bgr[2] - mean_r;
	tmp2 = p - mean_p;
	multiply(tmp1, tmp2, cov_rp);
	boxFilter(cov_rp, cov_rp, -1, kSize); // covariance of r(I) & p

	Mat cov_bb, cov_bg, cov_br, cov_gg, cov_gr, cov_rr;

	tmp1 = bgr[0] - mean_b;
	multiply(tmp1, tmp1, cov_bb);
	boxFilter(cov_bb, cov_bb, -1, kSize); // covariance of b & b

	tmp1 = bgr[1] - mean_g;
	multiply(tmp1, tmp1, cov_gg);
	boxFilter(cov_gg, cov_gg, -1, kSize); // covariance of g & g

	tmp1 = bgr[2] - mean_r;
	multiply(tmp1, tmp1, cov_rr);
	boxFilter(cov_rr, cov_rr, -1, kSize); // covariance of r & r

	tmp1 = bgr[0] - mean_b;
	tmp2 = bgr[1] - mean_g;
	multiply(tmp1, tmp2, cov_bg);
	boxFilter(cov_bg, cov_bg, -1, kSize); // covariance of b & g

	tmp1 = bgr[0] - mean_b;
	tmp2 = bgr[2] - mean_r;
	multiply(tmp1, tmp2, cov_br);
	boxFilter(cov_br, cov_br, -1, kSize); // covariance of b & r

	tmp1 = bgr[1] - mean_g;
	tmp2 = bgr[2] - mean_r;
	multiply(tmp1, tmp2, cov_gr);
	boxFilter(cov_gr, cov_gr, -1, kSize); // covariance of g & r

	Mat a_b(p.rows, p.cols, CV_32F);
	Mat a_g(p.rows, p.cols, CV_32F);
	Mat a_r(p.rows, p.cols, CV_32F);
	Mat b(p.rows, p.cols, CV_32F);

	for (auto i = 0; i < p.rows; i++) for (auto j = 0; j < p.cols; j++) {

		Mat sigma(3, 3, CV_32F);
		sigma.at<float>(0, 0) = cov_bb.at<float>(i, j);
		sigma.at<float>(1, 0) = cov_bg.at<float>(i, j);
		sigma.at<float>(2, 0) = cov_br.at<float>(i, j);

		sigma.at<float>(0, 1) = cov_bg.at<float>(i, j);
		sigma.at<float>(1, 1) = cov_gg.at<float>(i, j);
		sigma.at<float>(2, 1) = cov_gr.at<float>(i, j);

		sigma.at<float>(0, 2) = cov_br.at<float>(i, j); // | bb bg br | 
		sigma.at<float>(1, 2) = cov_gr.at<float>(i, j); // | bg gg gr |
		sigma.at<float>(2, 2) = cov_rr.at<float>(i, j); // | br gr rr |

		sigma = sigma + eps * Mat::eye(3, 3, CV_32F); // sigma_k + eps * Identity matrix -> 3 x 3 (A)

		Mat cov(3, 1, CV_32F);
		cov.at<float>(0, 0) = cov_bp.at<float>(i, j);
		cov.at<float>(1, 0) = cov_gp.at<float>(i, j);
		cov.at<float>(2, 0) = cov_rp.at<float>(i, j); // covariance b,g,r(I) & p -> 3 x 1 (b)

		Mat a(3, 1, CV_32F); // a_k (x)

		solve(sigma, cov, a); // solve lienar system (Ax = b)

		a_b.at<float>(i, j) = a.at<float>(0, 0);
		a_g.at<float>(i, j) = a.at<float>(1, 0);
		a_r.at<float>(i, j) = a.at<float>(2, 0);

		b.at<float>(i, j) = mean_p.at<float>(i, j) - a.at<float>(0, 0) * mean_b.at<float>(i, j) - a.at<float>(1, 0) * mean_g.at<float>(i, j) - a.at<float>(2, 0) * mean_r.at<float>(i, j);
	}

	Mat mean_ab, mean_ag, mean_ar, B;
	boxFilter(a_b, mean_ab, -1, kSize);
	boxFilter(a_g, mean_ag, -1, kSize);
	boxFilter(a_r, mean_ar, -1, kSize);
	boxFilter(b, B, -1, kSize); // mean of coeffient a_b, a_g, a_r, b

	Mat A_b, A_g, A_r;
	multiply(mean_ab, bgr[0], A_b);
	multiply(mean_ag, bgr[1], A_g);
	multiply(mean_ar, bgr[2], A_r);

	Mat q;
	q = A_b + A_g + A_r + B;
	return q;
}

