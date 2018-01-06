// Dialogwindow.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "Dialogwindow.h"
#include "afxdialogex.h"


// CDialogwindow 대화 상자입니다.

IMPLEMENT_DYNAMIC(CDialogwindow, CDialogEx)

CDialogwindow::CDialogwindow(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MAINFRAME, pParent)
{

}

CDialogwindow::~CDialogwindow()
{
}

void CDialogwindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogwindow, CDialogEx)
END_MESSAGE_MAP()


// CDialogwindow 메시지 처리기입니다.
