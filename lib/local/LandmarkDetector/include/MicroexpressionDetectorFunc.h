#pragma once
//  Header for all external CLNF/CLM-Z/CLM methods of interest to the user
#ifndef __LANDMARK_DETECTOR_UTILS_h_
#define __LANDMARK_DETECTOR_UTILS_h_

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

	void DrawDistance(cv::Mat img, const cv::Mat_<double>& shape2D, const cv::Mat_<int>& visibilities);
	void DrawDistance(cv::Mat img, const cv::Mat_<double>& shape2D);
	void DrawDistance(cv::Mat img, const CLNF& clnf_model);

}
#endif
