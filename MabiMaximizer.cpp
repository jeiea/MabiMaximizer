#include "stdafx.h"

// 정의하면 전체화면 모드, 정의를 해제하면 최대화만 활성화시킨다.
//#define FULLSCREEN

// 프로그램 이름
LPCTSTR lpszTitle = _T("Mabinogi Maximizer");
// 마비노기 세팅 레지스트리 경로
LPCTSTR regMabinogi = _T("Software\\Nexon\\Mabinogi");

// http://stackoverflow.com/a/6218957
BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool GetParentDirectory(LPTSTR szPath)
{
	TCHAR* lastSeparater = StrRChr(szPath, szPath + lstrlen(szPath) - 1, _T('\\'));
	if (lastSeparater)
	{
		*lastSeparater = NULL;
		return true;
	}
	return false;
}

bool GetMabinogiPathOnRegistry(TCHAR mabinogiPath[MAX_PATH])
{
	// 정상적으로 설치되었을 때 쉽게 경로를 구할 수 있는 정석
	LONG result;
	result = SHRegGetPath(HKEY_CURRENT_USER, regMabinogi, _T(""), mabinogiPath, NULL);

	lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		return true;
	}

	// 설치하지 않았을 때 실행하고 남는 찌꺼기를 찾는 방법
	result = SHRegGetPath(HKEY_CURRENT_USER,
		_T("Software\\Ahnlab\\HShield\\eh*;o(.."),
		_T("GamePath"), mabinogiPath, NULL);

	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		GetParentDirectory(mabinogiPath);
		lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
		return true;
	}

	return false;
}

// 실행파일 (상위)경로에서 Mabinogi.exe 검색
bool GetMabinogiPathOnExecutable(TCHAR mabinogiPath[MAX_PATH])
{
	TCHAR executablePath[MAX_PATH];

	GetModuleFileName(NULL, executablePath, MAX_PATH);
	while (GetParentDirectory(executablePath))
	{
		lstrcpy(mabinogiPath, executablePath);
		lstrcat(mabinogiPath, _T("\\Mabinogi\\Mabinogi.exe"));
		if (FileExists(mabinogiPath))
		{
			return true;
		}

		lstrcpy(mabinogiPath, executablePath);
		lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
		if (FileExists(mabinogiPath))
		{
			return true;
		}
	}
	return false;
}

// 마비노기 런처경로를 주든 폴더경로를 주든 폴더경로로 바꿔서 레지삽입
void SetMabinogiPathOnRegistry(TCHAR mabinogiPath[MAX_PATH])
{
	TCHAR mabinogiRoot[MAX_PATH];
	lstrcpy(mabinogiRoot, mabinogiPath);

	if (FileExists(mabinogiRoot))
	{
		GetParentDirectory(mabinogiRoot);
	}

	// SHRegSetPath를 쓰면 변수로 확장이 일어나서 안 됨.
	SHRegSetUSValue(regMabinogi, _T(""), REG_SZ, mabinogiRoot,
		lstrlen(mabinogiRoot) * sizeof(mabinogiRoot[0]), SHREGSET_FORCE_HKCU);
}

// Mabinogi.exe경로를 다양한 방법으로 찾아서 넣어줌
bool GetMabinogiPath(TCHAR mabinogiPath[MAX_PATH])
{
	if (GetMabinogiPathOnRegistry  (mabinogiPath) ||
		GetMabinogiPathOnExecutable(mabinogiPath))
	{
		return true;
	}

	OPENFILENAME OFN;
	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.lpstrTitle = _T("마비노기 실행파일을 선택해주세요.");
	OFN.lpstrFilter = TEXT("마비노기 실행파일(Mabinogi.exe)\0Mabinogi.exe\0모든 파일\0*.*\0");
	OFN.lpstrFile = mabinogiPath;
	OFN.nMaxFile = MAX_PATH;
	OFN.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&OFN))
	{
		SetMabinogiPathOnRegistry(mabinogiPath);
		return true;
	}

	return false;
}

void ChangeWindowStyle(HWND hWnd)
{
	// WS_CAPTION == WS_BORDER | WS_DLGFRAME인데, 둘 중 하나라도 없으면 타이틀 바가 사라진다.
	// 동시에 최대화할 때 작업표시줄도 가려버리게 되는데, 이걸 해결할 방법이 마땅치 않다.
	const LONG windowedStyle = WS_CAPTION | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME | WS_OVERLAPPED | WS_MINIMIZEBOX;
	const LONG fullscreenStyle = WS_POPUP | WS_MINIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS;
	const LONG windowedFullscreenToggleStyle = WS_MAXIMIZEBOX | WS_BORDER | WS_THICKFRAME;
	const LONG maximizeToggleStyle = WS_MAXIMIZEBOX;
	LONG style;

#ifdef FULLSCREEN
	const LONG& toggleStyle = windowedFullscreenToggleStyle;
#else
	const LONG& toggleStyle = maximizeToggleStyle;
#endif

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if (style & WS_POPUP)
	{
		MessageBox(hWnd, _T("창 모드로 바꾸고 실행해주세요."), lpszTitle, MB_OK);
		return;
	}
	style ^= toggleStyle;
	SetWindowLongPtr(hWnd, GWL_STYLE, style);
	SetClassLongPtr(hWnd, GCL_STYLE, style);

	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	ShowWindow(hWnd, SW_MAXIMIZE);
}

bool IsMabinogiProcess(DWORD processID)
{
	TCHAR szProcessName[MAX_PATH] = _T("");
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID); 

	if (hProcess)
	{
		GetProcessImageFileName(hProcess, szProcessName, MAX_PATH);
		CloseHandle(hProcess);

		if (StrStrI(szProcessName, _T("\\Mabinogi.exe\0")) ||
			StrStrI(szProcessName, _T("\\Client.exe\0")))
		{
			return true;
		}
	}
	return false;
}

bool IsMabinogiAlive()
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		MessageBox(NULL, _T("프로세스에 접근할 수 없습니다."), _T("에러"), MB_OK);
		return false;
	}
	cProcesses = cbNeeded / sizeof(DWORD);

	for (unsigned i = 0; i < cProcesses; i++)
		if (IsMabinogiProcess(aProcesses[i]))
			return true;

	return false;
}

void CALLBACK MonitoringTimer(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	HWND hMabiWin = FindWindow(_T("Mabinogi"), _T("마비노기"));
	if (IsWindowVisible(hMabiWin))
	{
		KillTimer(hWnd, idEvent);
		ChangeWindowStyle(hMabiWin);
		PostQuitMessage(0);
	}

	if (!IsMabinogiAlive())
	{
		PostQuitMessage(0);
	}
}

void LaunchMabinogi()
{
	TCHAR mabinogiPath[MAX_PATH];
	STARTUPINFO si = {sizeof(STARTUPINFO), };
	PROCESS_INFORMATION pi;

	if (!GetMabinogiPath(mabinogiPath))
	{
		return;
	}

	// 의존성을 줄이기 위해 ShellExecute대신 CreateProcess를 씀.
	CreateProcess(mabinogiPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
	HANDLE hMutex = CreateMutex(NULL, FALSE, _T("Mabinogi Maximizer Only Instance Mutex"));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(hMutex);
		return 0;
	}

	// 실행하지 못하면 타이머에서 IsMabinogiAlive가 알아서 종료해 줄 것임.
	if (!IsMabinogiAlive())
	{
		LaunchMabinogi();
	}

	// Sleep() 무한반복도 있지만, 걔는 WaitForInput으로 간주가 안 되어서인지 커서가
	// 백그라운드 작업 중으로 잠깐 남아있는 현상 발견. 고로 타이머루틴.
	LONG timerID;
	timerID = SetTimer(NULL, 0, 500, MonitoringTimer);

	MSG Message;
	while (GetMessage(&Message, NULL, 0, 0))
	{
		DispatchMessage(&Message);
	}
	KillTimer(NULL, timerID);

	CloseHandle(hMutex);
	return (int)Message.wParam;
}
