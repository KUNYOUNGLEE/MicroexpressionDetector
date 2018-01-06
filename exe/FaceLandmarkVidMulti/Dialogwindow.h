#pragma once


// CDialogwindow 대화 상자입니다.

class CDialogwindow : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogwindow)

public:
	CDialogwindow(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CDialogwindow();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAINFRAME };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
};
