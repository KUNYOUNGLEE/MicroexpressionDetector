
// Facial_orientation_detector.cpp : 응용 프로그램에 대한 클래스 동작을 정의합니다.
//
#include "stdafx.h"
#include "Facial_orientation_detector.h"
#include "Facial_orientation_detectorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FATAL_STREAM( stream ) \
printErrorAndAbort( std::string( "Fatal error: " ) + stream )

// CFacial_orientation_detectorApp

BEGIN_MESSAGE_MAP(CFacial_orientation_detectorApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CFacial_orientation_detectorApp 생성

CFacial_orientation_detectorApp::CFacial_orientation_detectorApp()
{
	// 다시 시작 관리자 지원
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}


// 유일한 CFacial_orientation_detectorApp 개체입니다.

CFacial_orientation_detectorApp theApp;


// CFacial_orientation_detectorApp 초기화

BOOL CFacial_orientation_detectorApp::InitInstance()
{
	// 응용 프로그램 매니페스트가 ComCtl32.dll 버전 6 이상을 사용하여 비주얼 스타일을
	// 사용하도록 지정하는 경우, Windows XP 상에서 반드시 InitCommonControlsEx()가 필요합니다.
	// InitCommonControlsEx()를 사용하지 않으면 창을 만들 수 없습니다.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 응용 프로그램에서 사용할 모든 공용 컨트롤 클래스를 포함하도록
	// 이 항목을 설정하십시오.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 대화 상자에 셸 트리 뷰 또는
	// 셸 목록 뷰 컨트롤이 포함되어 있는 경우 셸 관리자를 만듭니다.
	CShellManager *pShellManager = new CShellManager;

	// MFC 컨트롤의 테마를 사용하기 위해 "Windows 원형" 비주얼 관리자 활성화
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// 표준 초기화
	// 이들 기능을 사용하지 않고 최종 실행 파일의 크기를 줄이려면
	// 아래에서 필요 없는 특정 초기화
	// 루틴을 제거해야 합니다.
	// 해당 설정이 저장된 레지스트리 키를 변경하십시오.
	// TODO: 이 문자열을 회사 또는 조직의 이름과 같은
	// 적절한 내용으로 수정해야 합니다.
	SetRegistryKey(_T("로컬 응용 프로그램 마법사에서 생성된 응용 프로그램"));
	CFacial_orientation_detectorDlg dlg;

	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "경고: 대화 상자를 만들지 못했으므로 응용 프로그램이 예기치 않게 종료됩니다.\n");
		TRACE(traceAppMsg, 0, "경고: 대화 상자에서 MFC 컨트롤을 사용하는 경우 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS를 수행할 수 없습니다.\n");
	}

	//// 위에서 만든 셸 관리자를 삭제합니다.
	//if (pShellManager != NULL)
	//{
	//	delete pShellManager;
	//}

#ifndef _AFXDLL
	ControlBarCleanUp();
#endif

	// 대화 상자가 닫혔으므로 응용 프로그램의 메시지 펌프를 시작하지 않고  응용 프로그램을 끝낼 수 있도록 FALSE를
	// 반환합니다.
	return FALSE;
}

void CFacial_orientation_detectorApp::printErrorAndAbort(const std::string & error)
{
	abort();
}

void CFacial_orientation_detectorApp::DisplayImg(HWND dialogwindow, cv::Mat disp_image)
{
	IplImage* m_pImage = NULL;
	CvvImage m_cImage;

	if (!disp_image.empty())
	{
		m_pImage = &IplImage(disp_image);//cv::Mat -> IplImage
		m_cImage.CopyOf(m_pImage);//IplImage -> cvvImage (Opencv의 cv::Mat은 다양한 플랫폼을 지원함으로 picture control DC에 출력하기 위해선 변환이 필요함)

		HWND panel = GetDlgItem(dialogwindow, IDC_PANEL);
		RECT rc;
		GetClientRect(panel, &rc);//pitcture control 사각형을 받음
		HDC dc = GetDC(panel);
		m_cImage.DrawToHDC(dc, &rc);//출력
		ReleaseDC(dialogwindow, dc);//dc해제
	}
}

const std::string CFacial_orientation_detectorApp::currentDateTime()
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	localtime_s(&tstruct, &now);
	// Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M", &tstruct);

	return buf;
}

vector<string> CFacial_orientation_detectorApp::get_arguments(int argc, char ** argv)
{
	vector<string> arguments;

	for (int i = 0; i < argc; ++i)
	{
		arguments.push_back(string(argv[i]));
	}
	return arguments;
}

void CFacial_orientation_detectorApp::NonOverlapingDetections(const vector<LandmarkDetector::CLNF>& clnf_models, vector<cv::Rect_<double>>& face_detections)
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

bool CFacial_orientation_detectorApp::IsModuleSelected(HWND hwndDlg, const int moduleID)
{
	return (Button_GetState(GetDlgItem(hwndDlg, moduleID)) & BST_CHECKED) != 0;
}



