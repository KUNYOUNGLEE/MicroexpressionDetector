#include "stdafx.h"

#include <MicroexpressionDetectorFunc.h>

// OpenCV includes
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>

// Boost includes
#include <filesystem.hpp>
#include <filesystem/fstream.hpp>

using namespace boost::filesystem;

using namespace std;

namespace LandmarkDetector
{

	// For subpixel accuracy drawing
	const int draw_shiftbits_micro = 4;
	const int draw_multiplier_micro = 1 << 4;

	//===========================================================================
	// Visualisation functions
	//===========================================================================
	void ProjectDistance(cv::Mat_<double>& dest, const cv::Mat_<double>& mesh, double fx, double fy, double cx, double cy)
	{
		dest = cv::Mat_<double>(mesh.rows, 2, 0.0);

		int num_points = mesh.rows;

		double X, Y, Z;


		cv::Mat_<double>::const_iterator mData = mesh.begin();
		cv::Mat_<double>::iterator projected = dest.begin();

		for (int i = 0; i < num_points; i++)
		{
			// Get the points
			X = *(mData++);
			Y = *(mData++);
			Z = *(mData++);

			double x;
			double y;

			// if depth is 0 the projection is different
			if (Z != 0)
			{
				x = ((X * fx / Z) + cx);
				y = ((Y * fy / Z) + cy);
			}
			else
			{
				x = X;
				y = Y;
			}

			// Project and store in dest matrix
			(*projected++) = x;
			(*projected++) = y;
		}

	}

	// Computing landmarks (to be drawn later possibly)
	vector<cv::Point2d> CalculateLandmarksDistance(const cv::Mat_<double>& shape2D, cv::Mat_<int>& visibilities)
	{
		int n = shape2D.rows / 2;
		vector<cv::Point2d> landmarks;

		for (int i = 0; i < n; ++i)
		{
			if (visibilities.at<int>(i))
			{
				cv::Point2d featurePoint(shape2D.at<double>(i), shape2D.at<double>(i + n));

				landmarks.push_back(featurePoint);
			}
		}
		return landmarks;
	}

	// Computing landmarks (to be drawn later possibly)
	vector<cv::Point2d> CalculateLandmarksDistance(cv::Mat img, const cv::Mat_<double>& shape2D)
	{
		int n;
		vector<cv::Point2d> landmarks;

		if (shape2D.cols == 2)
		{
			n = shape2D.rows;
		}
		else if (shape2D.cols == 1)
		{
			n = shape2D.rows / 2;
		}

		for (int i = 0; i < n; ++i)
		{
			cv::Point2d featurePoint;
			if (shape2D.cols == 1)
			{
				featurePoint = cv::Point2d(shape2D.at<double>(i), shape2D.at<double>(i + n));
			}
			else
			{
				featurePoint = cv::Point2d(shape2D.at<double>(i, 0), shape2D.at<double>(i, 1));
			}
			landmarks.push_back(featurePoint);
		}
		return landmarks;
	}

	// Computing landmarks (to be drawn later possibly)
	vector<cv::Point2d> CalculateLandmarksDistance(CLNF& clnf_model)
	{

		int idx = clnf_model.patch_experts.GetViewIdx(clnf_model.params_global, 0);

		// Because we only draw visible points, need to find which points patch experts consider visible at a certain orientation
		return CalculateLandmarksDistance(clnf_model.detected_landmarks, clnf_model.patch_experts.visibilities[0][idx]);

	}

	// Drawing landmarks on a face image
	void DrawDistance(cv::Mat img, const cv::Mat_<double>& shape2D, const cv::Mat_<int>& visibilities, double *distance)
	{
		int n = shape2D.rows / 2;
		cv::Point featurePoint48, featurePoint54, normPoint1, normPoint2;

		// Drawing feature points
		if (n >= 66)
		{
			// A rough heuristic for drawn point size
			int thickness = (int)std::ceil(3.0* ((double)img.cols) / 640.0);
			int thickness_2 = (int)std::ceil(1.0* ((double)img.cols) / 640.0);

			if (visibilities.at<int>(48))
			{
				cv::Point featurePoint(cvRound(shape2D.at<double>(48) * (double)draw_multiplier_micro), cvRound(shape2D.at<double>(48 + n) * (double)draw_multiplier_micro));
				featurePoint48 = featurePoint;

				cv::circle(img, featurePoint, 1 * draw_multiplier_micro, cv::Scalar(0, 0, 255), thickness_2, CV_AA, draw_shiftbits_micro);
			}

			if (visibilities.at<int>(54))
			{
				cv::Point featurePoint(cvRound(shape2D.at<double>(54) * (double)draw_multiplier_micro), cvRound(shape2D.at<double>(54 + n) * (double)draw_multiplier_micro));
				featurePoint54 = featurePoint;

				cv::circle(img, featurePoint, 1 * draw_multiplier_micro, cv::Scalar(0, 0, 255), thickness_2, CV_AA, draw_shiftbits_micro);
			}

			if (visibilities.at<int>(39))
			{
				cv::Point featurePoint(cvRound(shape2D.at<double>(39) * (double)draw_multiplier_micro), cvRound(shape2D.at<double>(39 + n) * (double)draw_multiplier_micro));
				normPoint1 = featurePoint;
				cv::circle(img, featurePoint, 1 * draw_multiplier_micro, cv::Scalar(0, 255, 0), thickness_2, CV_AA, draw_shiftbits_micro);
			}

			if (visibilities.at<int>(42))
			{
				cv::Point featurePoint(cvRound(shape2D.at<double>(42) * (double)draw_multiplier_micro), cvRound(shape2D.at<double>(42 + n) * (double)draw_multiplier_micro));
				normPoint2 = featurePoint;
				cv::circle(img, featurePoint, 1 * draw_multiplier_micro, cv::Scalar(0, 255, 0), thickness_2, CV_AA, draw_shiftbits_micro);
			}
			double norm_factor = cv::norm(normPoint1 - normPoint2);
			double res = cv::norm(featurePoint48 - featurePoint54)/ norm_factor;//Euclidian distance
			
			char dist[255];
			sprintf(dist, "%0.2lf", res);
			string distr("N_dist:");
			distr += dist;
			cv::putText(img, distr, cv::Point(100, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);
			*distance = res;

			sprintf(dist, "%0.1lf", norm_factor);
			distr = "Norm_factor:";
			distr += dist;
			cv::putText(img, distr, cv::Point(220, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);
		}
	}

	// Drawing landmarks on a face image
	void DrawDistance(cv::Mat img, const cv::Mat_<double>& shape2D)
	{

		int n;

		if (shape2D.cols == 2)
		{
			n = shape2D.rows;
		}
		else if (shape2D.cols == 1)
		{
			n = shape2D.rows / 2;
		}

		for (int i = 0; i < n; ++i)
		{
			cv::Point featurePoint;
			if (shape2D.cols == 1)
			{
				featurePoint = cv::Point(cvRound(shape2D.at<double>(i) * (double)draw_multiplier_micro), cvRound(shape2D.at<double>(i + n) * (double)draw_multiplier_micro));
			}
			else
			{
				featurePoint = cv::Point(cvRound(shape2D.at<double>(i, 0) * (double)draw_multiplier_micro), cvRound(shape2D.at<double>(i, 1) * (double)draw_multiplier_micro));
			}
			// A rough heuristic for drawn point size
			int thickness = (int)std::ceil(5.0* ((double)img.cols) / 640.0);
			int thickness_2 = (int)std::ceil(1.5* ((double)img.cols) / 640.0);

			cv::circle(img, featurePoint, 1 * draw_multiplier_micro, cv::Scalar(0, 0, 255), thickness, CV_AA, draw_shiftbits_micro);
			cv::circle(img, featurePoint, 1 * draw_multiplier_micro, cv::Scalar(255, 0, 0), thickness_2, CV_AA, draw_shiftbits_micro);

		}

	}

	// Drawing detected landmarks on a face image
	void DrawDistance(cv::Mat img, const CLNF& clnf_model, double *dist)
	{

		int idx = clnf_model.patch_experts.GetViewIdx(clnf_model.params_global, 0);

		// Because we only draw visible points, need to find which points patch experts consider visible at a certain orientation
		DrawDistance(img, clnf_model.detected_landmarks, clnf_model.patch_experts.visibilities[0][idx], dist);

		// If the model has hierarchical updates draw those too
		for (size_t i = 0; i < clnf_model.hierarchical_models.size(); ++i)
		{
			if (clnf_model.hierarchical_models[i].pdm.NumberOfPoints() != clnf_model.hierarchical_mapping[i].size())
			{
				DrawDistance(img, clnf_model.hierarchical_models[i], dist);
			}
		}
	}

	void DrawLandmarksDistance(cv::Mat img, vector<cv::Point> landmarks)
	{
		for (cv::Point p : landmarks)
		{

			// A rough heuristic for drawn point size
			int thickness = (int)std::ceil(5.0* ((double)img.cols) / 640.0);
			int thickness_2 = (int)std::ceil(1.5* ((double)img.cols) / 640.0);

			cv::circle(img, p, 1, cv::Scalar(0, 0, 255), thickness, CV_AA);
			cv::circle(img, p, 1, cv::Scalar(255, 0, 0), thickness_2, CV_AA);
		}

	}

}
