
// Facial_orientation_detectorDlg.h : 헤더 파일
//

#pragma once

struct Renderingdata
{
	HWND dialogwindow;
	string filename;
	bool Openface_thread_activate;//디스플레이하는 스레드를 종료 시키기위한 전역변수
	bool Reset_activate;//얼굴추적을 Reset하기위한 전역변수
	bool playback_mode;
};

// CFacial_orientation_detectorDlg 대화 상자
class CFacial_orientation_detectorDlg : public CDialogEx
{
	// 생성입니다.
public:
	CFacial_orientation_detectorDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	DWORD *dwThreadID;
	HANDLE m_pThread;
	Renderingdata renderer_data;
	TCHAR fileName[200];
	bool playback_mode;
	//TCHAR fileName[200];
	//bool Openface_thread_activate;//디스플레이하는 스레드를 종료 시키기위한 전역변수
	//bool Reset_activate;//얼굴추적을 Reset하기위한 전역변수
	//bool playback_mode;

	int Runing_thread;
	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FACIAL_ORIENTATION_DETECTOR_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

														// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	void GetPlaybackFile();

	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedBrowser();
	afx_msg void OnBnClickedReset();

public:
	static DWORD WINAPI OpenFace(LPVOID arg)
	{
		CFacial_orientation_detectorApp *This;
		Renderingdata *pdata = (Renderingdata *)arg;
		string current_file = pdata->filename;
		vector<string> arguments = This->get_arguments(__argc, __argv);

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
		outroot = NPath;
		outroot = outroot + "/recording/";
		outfile = This->currentDateTime() + ".avi";
		///////////////////////////////////////////recording/////////////////////////////////////////////////////
		// Do some grabbing
		cv::VideoCapture video_capture;
		if (current_file.size() > 0)
		{
			video_capture = cv::VideoCapture(current_file);
			pdata->playback_mode = false;
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
			AfxMessageBox("Failed to open video source");
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


		if (This->IsModuleSelected(pdata->dialogwindow, IDC_RECORD))
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
				This->NonOverlapingDetections(clnf_models, face_detections);

				vector<tbb::atomic<bool> > face_detections_used(face_detections.size());

				// Go through every model and update the tracking
				tbb::parallel_for(0, (int)clnf_models.size(), [&](int model) {
					//for(unsigned int model = 0; model < clnf_models.size(); ++model)
					//{

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

						// Work out the pose of the head from the tracked model
						cv::Vec6d pose_estimate = LandmarkDetector::GetCorrectedPoseWorld(clnf_models[model], fx, fy, cx, cy);

						// Draw it in reddish if uncertain, blueish if certain
						LandmarkDetector::DrawBox(disp_image, pose_estimate, cv::Scalar((1 - detection_certainty)*255.0, 0, detection_certainty * 255), thickness, fx, fy, cx, cy);
						////////////////////////////////////////////////recording/////////////////////////////////////////////////

						if (This->IsModuleSelected(pdata->dialogwindow, IDC_RECORD))
						{
							double curr_time = (cv::getTickCount() - init_time) / freq;
							curr_time *= 1000;

							video_writer << captured_image;

							outlog << fixed << setprecision(2);
							outlog << setw(4) << frameProc + 1 << setw(11) << pose_estimate[0] << setw(9) << pose_estimate[1] << setw(9)
								<< pose_estimate[2] << setw(9) << pose_estimate[3] << setw(9) << pose_estimate[4] << setw(8) << pose_estimate[5];
							outlog << endl;

							frameProc++;
						}
						///////////////////////////////////////////////recording/////////////////////////////////////////////////
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

				//char active_m_C[255];
				//sprintf(active_m_C, "%d", num_active_models);
				//string active_models_st("Active models:");
				//active_models_st += active_m_C;
				//cv::putText(disp_image, active_models_st, cv::Point(10, 60), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

				if (!det_parameters[0].quiet_mode)
				{
					if (!disp_image.empty())
					{
						// 다이얼로그 picture control에 영상 띄워주는 함수
						This->DisplayImg(pdata->dialogwindow, disp_image);
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
				if (pdata->Reset_activate == true)
				{
					for (size_t i = 0; i < clnf_models.size(); ++i)
					{
						clnf_models[i].Reset();
						active_models[i] = false;
					}
					pdata->Reset_activate = false;
				}
				//// quit the application
				if (pdata->Openface_thread_activate == false)
				{
					return 0;
				}

				// Update the frame count
				frame_count++;
			}

			frame_count = 0;

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
		}

		//This->Button_Enable(GetDlgItem(dialogWindow, IDC_RECORD), true);
		//This->Openface_thread_activate = false;
		//This->Runing_thread = 0;
		return 0;
	}
};