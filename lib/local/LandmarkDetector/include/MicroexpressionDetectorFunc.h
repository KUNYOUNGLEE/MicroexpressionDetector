#pragma once

// OpenCV includes
#include <opencv2/core/core.hpp>

#include "LandmarkDetectorModel.h"

using namespace std;

namespace LandmarkDetector
{
	//===========================================================================
	// Visualisation functions
	//===========================================================================
	void ProjectDistance(cv::Mat_<double>& dest, const cv::Mat_<double>& mesh, double fx, double fy, double cx, double cy);
	vector<cv::Point2d> CalculateLandmarksDistance(const cv::Mat_<double>& shape2D, cv::Mat_<int>& visibilities);
	vector<cv::Point2d> CalculateLandmarksDistance(CLNF& clnf_model);
	void DrawLandmarksDistance(cv::Mat img, vector<cv::Point> landmarks);

	void DrawDistance(cv::Mat img, const cv::Mat_<double>& shape2D, const cv::Mat_<int>& visibilities, double *dist);
	void DrawDistance(cv::Mat img, const cv::Mat_<double>& shape2D);
	void DrawDistance(cv::Mat img, const CLNF& clnf_model, double *dist);

}