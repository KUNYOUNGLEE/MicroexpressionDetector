///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016, Carnegie Mellon University and University of Cambridge,
// all rights reserved.
//
// THIS SOFTWARE IS PROVIDED 밃S IS?FOR ACADEMIC USE ONLY AND ANY EXPRESS
// OR IMPLIED WARRANTIES WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY.
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Notwithstanding the license granted herein, Licensee acknowledges that certain components
// of the Software may be covered by so-called 뱋pen source?software licenses (밢pen Source
// Components?, which means any software licenses approved as open source licenses by the
// Open Source Initiative or any substantially similar licenses, including without limitation any
// license that, as a condition of distribution of the software licensed under such license,
// requires that the distributor make the software available in source code format. Licensor shall
// provide a list of Open Source Components for a particular version of the Software upon
// Licensee뭩 request. Licensee will comply with the applicable terms of such licenses and to
// the extent required by the licenses covering Open Source Components, the terms of such
// licenses will apply in lieu of the terms of this Agreement. To the extent the terms of the
// licenses applicable to Open Source Components prohibit any of the restrictions in this
// License Agreement with respect to such Open Source Component, such restrictions will not
// apply to such Open Source Component. To the extent the terms of the licenses applicable to
// Open Source Components require Licensor to make an offer to provide source code or
// related information in connection with the Software, such offer is hereby made. Any request
// for source code or related information should be directed to cl-face-tracker-distribution@lists.cam.ac.uk
// Licensee acknowledges receipt of notices for the Open Source Components for the initial
// delivery of the Software.

//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltru쉆itis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltru쉆itis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltru쉆itis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltru쉆itis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

// FaceTrackingVidMulti.cpp : Defines the entry point for the multiple face tracking console application.
#include "LandmarkCoreIncludes.h"
#include "Resource.h"
#include <map>

#include <fstream>
#include <sstream>

#include <iostream>

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

#include <FaceAnalyser.h>

#define Button_GetState(hwndCtl)            ((int)(DWORD)SNDMSG((hwndCtl), BM_GETSTATE, 0L, 0L))
#define Button_Enable(hwndCtl, fEnable)         EnableWindow((hwndCtl), (fEnable))

INT_PTR CALLBACK    MessageLoopThread(HWND, UINT, WPARAM, LPARAM);//프로그램 메인프레임으로 사용하는 다이얼로그 메세지루프
DWORD WINAPI OpenFace(LPVOID arg);//OpenFace 소스를 사용해 얼굴 방향을 계산하는 스레드 함수

//녹화 파일을 저장하기 위한 폴더명, 폴더경로 설정
string current_file;
TCHAR fileName[200] = { 0 };
HWND dialogWindow;
volatile bool Openface_thread_activate = false;//디스플레이하는 스레드를 종료 시키기위한 전역변수
volatile bool Reset_activate = false;//얼굴추적을 Reset하기위한 전역변수
volatile bool playback_mode = false;

int Runing_thread = 0;

static void printErrorAndAbort(const std::string & error)
{
	abort();
}

#define FATAL_STREAM( stream ) \
printErrorAndAbort( std::string( "Fatal error: " ) + stream )

using namespace std;

static void DisplayImg(cv::Mat disp_image) // 다이얼로그 picture control에 영상 출력
{
	IplImage* m_pImage = NULL;
	CvvImage m_cImage;

	if (!disp_image.empty())
	{
		m_pImage = &IplImage(disp_image);//cv::Mat -> IplImage
		m_cImage.CopyOf(m_pImage);//IplImage -> cvvImage (Opencv의 cv::Mat은 다양한 플랫폼을 지원함으로 picture control DC에 출력하기 위해선 변환이 필요함)

		HWND panel = GetDlgItem(dialogWindow, IDC_PANEL);
		RECT rc;
		GetClientRect(panel, &rc);//pitcture control 사각형을 받음
		HDC dc = GetDC(panel);
		m_cImage.DrawToHDC(dc, &rc);//출력
		ReleaseDC(dialogWindow, dc);//dc해제
	}
}

void GetPlaybackFile(void)
{
	OPENFILENAME filename;
	memset(&filename, 0, sizeof(filename));
	filename.lStructSize = sizeof(filename);
	filename.lpstrFilter = L"*.avi\0";
	filename.lpstrFile = fileName;
	fileName[0] = 0;
	filename.nMaxFile = sizeof(fileName) / sizeof(CHAR);
	filename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (!GetOpenFileName(&filename))
		fileName[0] = 0;
	playback_mode = true;
}

const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	localtime_s(&tstruct, &now);
	// Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M", &tstruct);

	return buf;
}

std::string TCHARToString(const TCHAR* ptsz)
{
	int len = wcslen((wchar_t*)ptsz);
	char* psz = new char[2 * len + 1];
	wcstombs(psz, (wchar_t*)ptsz, 2 * len + 1);
	std::string s = psz;
	delete[] psz;
	return s;
}

vector<string> get_arguments(int argc, char **argv)
{
	vector<string> arguments;

	for (int i = 0; i < argc; ++i)
	{
		arguments.push_back(string(argv[i]));
	}
	return arguments;
}

void NonOverlapingDetections(const vector<LandmarkDetector::CLNF>& clnf_models, vector<cv::Rect_<double> >& face_detections)
{
	// Go over the model and eliminate detections that are not informative (there already is a tracker there)
	for (size_t model = 0; model < clnf_models.size(); ++model)
	{

		// See if the detections intersect
		cv::Rect_<double> model_rect = clnf_models[model].GetBoundingBox();

		for (int detection = face_detections.size() - 1; detection >= 0; --detection)
		{
			double intersection_area = (model_rect & face_detections[detection]).area();
			double union_area = model_rect.area() + face_detections[detection].area() - 2 * intersection_area;

			// If the model is already tracking what we're detecting ignore the detection, this is determined by amount of overlap
			if (intersection_area / union_area > 0.5)
			{
				face_detections.erase(face_detections.begin() + detection);
			}
		}
	}
}

bool IsModuleSelected(HWND hwndDlg, const int moduleID)//다이얼로그 체크박스 체크여부 확인하는 함수
{
	return (Button_GetState(GetDlgItem(hwndDlg, moduleID)) & BST_CHECKED) != 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{

	dialogWindow = CreateDialogW(hInstance, MAKEINTRESOURCE(IDD_MAINFRAME), 0, MessageLoopThread);
	if (!dialogWindow)
	{
		return 1;
	}
	ShowWindow(dialogWindow, SW_SHOW);

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);											// 두 메시지를 하나로 변형할때 사용한다.
		DispatchMessage(&msg);											// 메시지를 처리하는 함수에 메시지를 보낸다.
	}

	Openface_thread_activate = false;

	Sleep(10);

	return msg.wParam;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK MessageLoopThread(HWND dialogWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	int screenwidth = GetSystemMetrics(SM_CXSCREEN);
	int screenheight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		GetClientRect(dialogWindow, &rc);
		SetWindowPos(dialogWindow, NULL, (screenwidth/2) - (int)((rc.right-rc.left)/2)
			, (screenheight/2) - (int)((rc.bottom - rc.top) / 2), 0, 0, SWP_NOSIZE);//다이얼로그 화면 정가운데에 출력

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_START:
			Openface_thread_activate = true;
			Reset_activate = false;
			Runing_thread++;
			Button_Enable(GetDlgItem(dialogWindow, IDC_RECORD), false);
			if (Runing_thread == 1) {
				CreateThread(NULL, NULL, OpenFace, NULL, NULL, NULL);
			}

			return TRUE;
		case ID_STOP:
			Openface_thread_activate = false;//디스플레이하는 스레드를 종료 시키기위한 전역변수
			Reset_activate = false;
			Runing_thread = 0;
			Button_Enable(GetDlgItem(dialogWindow, IDC_RECORD), true);

			return TRUE;
		case ID_PLAYBACK:
			if(Openface_thread_activate == false && Reset_activate == false)
				GetPlaybackFile();

			return TRUE;
		case ID_RESET:
			if (Openface_thread_activate == true && Reset_activate == false)
				Reset_activate = true;

			return TRUE;
		}

		break;
	case WM_DESTROY:
		Openface_thread_activate = false;//디스플레이하는 스레드를 종료 시키기위한 전역변수
		PostQuitMessage(0);

		Sleep(10);

		return (INT_PTR)FALSE;

		break;
	default:
		return DefWindowProc(dialogWindow, message, wParam, lParam);
		break;
	}
	return (INT_PTR)FALSE;
}

DWORD WINAPI OpenFace(LPVOID arg)
{
	vector<string> arguments = get_arguments(__argc, __argv);

	// Some initial parameters that can be overriden from command line	
	vector<string> files, depth_directories, tracked_videos_output, dummy_out;

	// By default try webcam 0
	int device = 0;

	// cx and cy aren't necessarilly in the image center, so need to be able to override it (start with unit vals and init them if none specified)
	float fx = 600, fy = 600, cx = 0, cy = 0;

	LandmarkDetector::FaceModelParameters det_params(arguments);
	det_params.use_face_template = true;
	// This is so that the model would not try re-initialising itself
	det_params.reinit_video_every = -1;

	det_params.curr_face_detector = LandmarkDetector::FaceModelParameters::HOG_SVM_DETECTOR;

	vector<LandmarkDetector::FaceModelParameters> det_parameters;
	det_parameters.push_back(det_params);

	// Get the input output file parameters
	bool u;
	string output_codec;
	LandmarkDetector::get_video_input_output_params(files, depth_directories, dummy_out, tracked_videos_output, u, output_codec, arguments);
	// Get camera parameters
	LandmarkDetector::get_camera_params(device, fx, fy, cx, cy, arguments);

	// The modules that are being used for tracking
	vector<LandmarkDetector::CLNF> clnf_models;
	vector<bool> active_models;

	int num_faces_max = 1;

	LandmarkDetector::CLNF clnf_model(det_parameters[0].model_location);
	clnf_model.face_detector_HAAR.load(det_parameters[0].face_detector_location);
	clnf_model.face_detector_location = det_parameters[0].face_detector_location;

	clnf_models.reserve(num_faces_max);

	clnf_models.push_back(clnf_model);
	active_models.push_back(false);
	
	for (int i = 1; i < num_faces_max; ++i)
	{
		clnf_models.push_back(clnf_model);
		active_models.push_back(false);
		det_parameters.push_back(det_params);
	}

	// If multiple video files are tracked, use this to indicate if we are done
	bool done = false;
	int f_n = -1;

	// If cx (optical axis centre) is undefined will use the image size/2 as an estimate
	bool cx_undefined = false;
	if (cx == 0 || cy == 0)
	{
		cx_undefined = true;
	}
	///////////////////////////////////////////recording/////////////////////////////////////////////////////
	// Some initial parameters that can be overriden from command line	
	string outroot, outfile;
	TCHAR NPath[200];
	GetCurrentDirectory(200, NPath);

	current_file = TCHARToString(fileName);
	// By default write to same directory
	outroot = TCHARToString(NPath);
	outroot = outroot + "/recording/";
	outfile = currentDateTime() + ".avi";
	///////////////////////////////////////////recording/////////////////////////////////////////////////////
	// Do some grabbing
	cv::VideoCapture video_capture;
	// (current_file.size() > 0)
	if(playback_mode)
	{
		video_capture = cv::VideoCapture(current_file);
		playback_mode = false;
	}
	else
	{
		video_capture = cv::VideoCapture(device);
		// Read a first frame often empty in camera
		cv::Mat captured_image;
		video_capture >> captured_image;
	}

	if (!video_capture.isOpened())
	{
		FATAL_STREAM("Failed to open video source");
		return 0;
	}

	cv::Mat captured_image;
	video_capture >> captured_image;

	boost::filesystem::path dir(outroot);
	boost::filesystem::create_directory(dir);

	string out_file = outroot + outfile;

	// saving the videos
	cv::VideoWriter video_writer;
	ofstream outlog;

	if (IsModuleSelected(dialogWindow, IDC_RECORD))
	{
		video_writer.open(out_file, CV_FOURCC('D', 'I', 'V', 'X'), 30, captured_image.size(), true);
		outlog.open((outroot + outfile + ".log").c_str(), ios_base::out);
		outlog << "frame  X(trans)  Y(trans)  Z(trans)     X(rot)   Y(rot)    Z(rot) " << endl;
	}

	double freq = cv::getTickFrequency();
	double init_time = (double)cv::getTickCount();
	int frameProc = 0;

	while (!done) // this is not a for loop as we might also be reading from a webcam
	{
		// We might specify multiple video files as arguments
		if (files.size() > 0)
		{
			f_n++;
			current_file = files[f_n];
		}

		bool use_depth = !depth_directories.empty();
		// If optical centers are not defined just use center of image
		if (cx_undefined)
		{
			cx = captured_image.cols / 2.0f;
			cy = captured_image.rows / 2.0f;
		}

		int frame_count = 0;
		// For measuring the timings
		int64 t1, t0 = cv::getTickCount();
		double fps = 10;

		while (!captured_image.empty())
		{

			// Reading the images
			cv::Mat_<float> depth_image;
			cv::Mat_<uchar> grayscale_image;

			cv::Mat disp_image = captured_image.clone();

			if (captured_image.channels() == 3)
			{
				cv::cvtColor(captured_image, grayscale_image, CV_BGR2GRAY);
			}
			else
			{
				grayscale_image = captured_image.clone();
			}

			// Get depth image
			if (use_depth)
			{
				char* dst = new char[100];
				std::stringstream sstream;

				sstream << depth_directories[f_n] << "\\depth%05d.png";
				sprintf(dst, sstream.str().c_str(), frame_count + 1);
				// Reading in 16-bit png image representing depth
				cv::Mat_<short> depth_image_16_bit = cv::imread(string(dst), -1);

				// Convert to a floating point depth image
				if (!depth_image_16_bit.empty())
				{
					depth_image_16_bit.convertTo(depth_image, CV_32F);
				}
			}

			vector<cv::Rect_<double> > face_detections;

			bool all_models_active = true;
			for (unsigned int model = 0; model < clnf_models.size(); ++model)
			{
				if (!active_models[model])
				{
					all_models_active = false;
				}
			}

			// Get the detections (every 8th frame and when there are free models available for tracking)
			if (frame_count % 8 == 0 && !all_models_active)
			{
				if (det_parameters[0].curr_face_detector == LandmarkDetector::FaceModelParameters::HOG_SVM_DETECTOR)
				{
					vector<double> confidences;
					LandmarkDetector::DetectFacesHOG(face_detections, grayscale_image, clnf_models[0].face_detector_HOG, confidences);
				}
				else
				{
					LandmarkDetector::DetectFaces(face_detections, grayscale_image, clnf_models[0].face_detector_HAAR);
				}

			}
			// Keep only non overlapping detections (also convert to a concurrent vector)
			NonOverlapingDetections(clnf_models, face_detections);

			vector<tbb::atomic<bool> > face_detections_used(face_detections.size());

			// Go through every model and update the tracking
			tbb::parallel_for(0, (int)clnf_models.size(), [&](int model) {

				bool detection_success = false;

				// If the current model has failed more than 4 times in a row, remove it
				if (clnf_models[model].failures_in_a_row > 4)
				{
					active_models[model] = false;
					clnf_models[model].Reset();

				}

				// If the model is inactive reactivate it with new detections
				if (!active_models[model])
				{

					for (size_t detection_ind = 0; detection_ind < face_detections.size(); ++detection_ind)
					{
						// if it was not taken by another tracker take it (if it is false swap it to true and enter detection, this makes it parallel safe)
						if (face_detections_used[detection_ind].compare_and_swap(true, false) == false)
						{
							// Reinitialise the model
							clnf_models[model].Reset();

							// This ensures that a wider window is used for the initial landmark localisation
							clnf_models[model].detection_success = false;
							detection_success = LandmarkDetector::DetectLandmarksInVideo(grayscale_image, depth_image, face_detections[detection_ind], clnf_models[model], det_parameters[model]);
							
							// This activates the model
							active_models[model] = true;

							// break out of the loop as the tracker has been reinitialised
							break;
						}

					}
				}
				else
				{
					// The actual facial landmark detection / tracking
					detection_success = LandmarkDetector::DetectLandmarksInVideo(grayscale_image, depth_image, clnf_models[model], det_parameters[model]);
				}
			});

			// Go through every model and visualise the results
			for (size_t model = 0; model < clnf_models.size(); ++model)
			{
				// Visualising the results
				// Drawing the facial landmarks on the face and the bounding box around it if tracking is successful and initialised
				double detection_certainty = clnf_models[model].detection_certainty;
				double visualisation_boundary = -0.1;

				// Only draw if the reliability is reasonable, the value is slightly ad-hoc
				if (detection_certainty < visualisation_boundary)
				{
					LandmarkDetector::Draw(disp_image, clnf_models[model]);

					if (detection_certainty > 1)
						detection_certainty = 1;
					if (detection_certainty < -1)
						detection_certainty = -1;

					detection_certainty = (detection_certainty + 1) / (visualisation_boundary + 1);

					// A rough heuristic for box around the face width
					int thickness = (int)std::ceil(2.0* ((double)captured_image.cols) / 640.0);
				}
			}

			
			// Work out the framerate
			if (frame_count % 10 == 0)
			{
				t1 = cv::getTickCount();
				fps = 10.0 / (double(t1 - t0) / cv::getTickFrequency());
				t0 = t1;
			}

			// Write out the framerate on the image before displaying it
			char fpsC[255];
			sprintf(fpsC, "%d", (int)fps);
			string fpsSt("FPS:");
			fpsSt += fpsC;
			cv::putText(disp_image, fpsSt, cv::Point(10, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

			int num_active_models = 0;

			for (size_t active_model = 0; active_model < active_models.size(); active_model++)
			{
				if (active_models[active_model])
				{
					num_active_models++;
				}
			}

			char active_m_C[255];
			sprintf(active_m_C, "%d", num_active_models);
			string active_models_st("Active models:");
			active_models_st += active_m_C;
			cv::putText(disp_image, active_models_st, cv::Point(10, 60), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

			if (!det_parameters[0].quiet_mode)
			{
				if (!disp_image.empty())
				{
					// 다이얼로그 picture control에 영상 띄워주는 함수
					DisplayImg(disp_image);
				}
				if (!depth_image.empty())
				{
					// Division needed for visualisation purposes
					imshow("depth", depth_image / 2000.0);
				}
			}

			video_capture >> captured_image;

			// detect key presses
			char character_press = cv::waitKey(1);

			// restart the trackers
			if (Reset_activate == true)
			{
				for (size_t i = 0; i < clnf_models.size(); ++i)
				{
					clnf_models[i].Reset();
					active_models[i] = false;
				}
				Reset_activate = false;
			}
			// quit the application
			else if (Openface_thread_activate == false)
			{
				return 0;
			}

			// Update the frame count
			frame_count++;
		}

		// Reset the model, for the next video
		for (size_t model = 0; model < clnf_models.size(); ++model)
		{
			clnf_models[model].Reset();
			active_models[model] = false;
		}

		// break out of the loop if done with all the files
		if (f_n == files.size() - 1)
		{
			done = true;
		}

		// Reset the variable, for the next video
		frame_count = 0;
	}

	// Reset the dialog button and utill_variable
	Button_Enable(GetDlgItem(dialogWindow, IDC_RECORD), true);
	Openface_thread_activate = false;
	Runing_thread = 0;
	return 0;
}

