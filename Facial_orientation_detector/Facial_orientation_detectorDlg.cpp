
// Facial_orientation_detectorDlg.cpp : 구현 파일
//
#include "stdafx.h"
#include "Facial_orientation_detector.h"
#include "Facial_orientation_detectorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define Button_GetState(hwndCtl)            ((int)(DWORD)SNDMSG((hwndCtl), BM_GETSTATE, 0L, 0L))
#define Button_Enable(hwndCtl, fEnable)         EnableWindow((hwndCtl), (fEnable))

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

														// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CFacial_orientation_detectorDlg 대화 상자

CFacial_orientation_detectorDlg::CFacial_orientation_detectorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_FACIAL_ORIENTATION_DETECTOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	playback_mode = false;
	renderer_data.playback_mode = false;
	Runing_thread = 0;
}

void CFacial_orientation_detectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFacial_orientation_detectorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CFacial_orientation_detectorDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CFacial_orientation_detectorDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_BROWSER, &CFacial_orientation_detectorDlg::OnBnClickedBrowser)
	ON_BN_CLICKED(IDC_RESET, &CFacial_orientation_detectorDlg::OnBnClickedReset)
END_MESSAGE_MAP()

// CFacial_orientation_detectorDlg 메시지 처리기

BOOL CFacial_orientation_detectorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

									// TODO: 여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CFacial_orientation_detectorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CFacial_orientation_detectorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CFacial_orientation_detectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CFacial_orientation_detectorDlg::GetPlaybackFile()
{
	OPENFILENAME filename;
	memset(&filename, 0, sizeof(filename));
	filename.lStructSize = sizeof(filename);
	filename.lpstrFilter = "*.avi\0";
	filename.lpstrFile = fileName;
	fileName[0] = 0;
	filename.nMaxFile = sizeof(fileName) / sizeof(CHAR);
	filename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (!GetOpenFileName(&filename))
		fileName[0] = 0;
	else {
		renderer_data.playback_mode = true; 
		playback_mode = true;
	}
}

void CFacial_orientation_detectorDlg::OnBnClickedStart()
{
	Runing_thread++;
	renderer_data.dialogwindow = AfxGetMainWnd()->m_hWnd;
	renderer_data.Openface_thread_activate = true;
	renderer_data.Reset_activate = false;
	if (playback_mode == true)
	{
		CString str = fileName;
		renderer_data.filename = ((LPCTSTR)str);
		renderer_data.playback_mode = true;
		playback_mode = false;
	}
	else
	{
		CString str = "";
		renderer_data.filename = ((LPCTSTR)str);
		renderer_data.playback_mode = false;
	}

	if (Runing_thread == 1) {

		m_pThread = CreateThread(NULL, NULL, OpenFace, &renderer_data, NULL, dwThreadID);
		if (m_pThread == NULL)
		{
			AfxMessageBox("Error");
		}

		Runing_thread = 0;
	}
}


void CFacial_orientation_detectorDlg::OnBnClickedStop()
{
	renderer_data.Openface_thread_activate = false;
	renderer_data.Reset_activate = false;
	renderer_data.playback_mode = false;
	playback_mode = false;
	Runing_thread = 0;
	fileName[0] = 0;
}


void CFacial_orientation_detectorDlg::OnBnClickedBrowser()
{
	GetPlaybackFile();
}


void CFacial_orientation_detectorDlg::OnBnClickedReset()
{
	renderer_data.Reset_activate = true;
}