
// Facial_orientation_detector.h : PROJECT_NAME 응용 프로그램에 대한 주 헤더 파일입니다.
//

#pragma once

#include "LandmarkCoreIncludes.h"
#include "Resource.h"

#include <fstream>
#include <sstream>

#include <iostream>

#include <windows.h>
// OpenCV includes
#include <opencv2/videoio/videoio.hpp>  // Video write
#include <opencv2/videoio/videoio_c.h>  // Video write
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "CvvImage.h"

#include <stdio.h>
#include <time.h>

#include <filesystem.hpp>
#include <filesystem/fstream.hpp>

#define Button_GetState(hwndCtl)            ((int)(DWORD)SNDMSG((hwndCtl), BM_GETSTATE, 0L, 0L))
#define Button_Enable(hwndCtl, fEnable)         EnableWindow((hwndCtl), (fEnable))

// CFacial_orientation_detectorApp:
// 이 클래스의 구현에 대해서는 Facial_orientation_detector.cpp을 참조하십시오.
//
class CFacial_orientation_detectorApp : public CWinApp
{
public:
	CFacial_orientation_detectorApp();

// 재정의입니다.
public:
	virtual BOOL InitInstance();

// 구현입니다.
public:

	void printErrorAndAbort(const std::string & error);
	void DisplayImg(HWND dialogwindow, cv::Mat disp_image);
	const std::string currentDateTime();
	vector<string> get_arguments(int argc, char **argv);
	void NonOverlapingDetections(const vector<LandmarkDetector::CLNF>& clnf_models, vector<cv::Rect_<double> >& face_detections);
	bool IsModuleSelected(HWND hwndDlg, const int moduleID);//다이얼로그 체크박스 체크여부 확인하는 함수

	//녹화 파일을 저장하기 위한 폴더명, 폴더경로 설정


	DECLARE_MESSAGE_MAP()
};

extern CFacial_orientation_detectorApp theApp;