#include "stdafx.h"
#include "resource.h"

// Predefined�� MABISCRVER_MAXIMIZE, MABISCRVER_TASKBAR, MABISCRVER_FULLSCREEN ���� 3���� �ִ�
// resource.h�� �����ؾ� ��

// ���α׷� �̸�
LPCTSTR lpszTitle = _T("Mabinogi Maximizer");
// ������ ���� ������Ʈ�� ���
LPCTSTR regMabinogi = _T("Software\\Nexon\\Mabinogi");

int ErrorMessageBox()
{
	LPCTSTR pM;
	DWORD err = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&pM, 0, NULL);
	MessageBox(NULL, pM, _T("MabiMaximizer"), MB_OK | MB_ICONERROR);

	LocalFree(&pM);
	return err;
}

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
	LONG result;

	// ���������� ��ġ�Ǿ��� �� ���� ��θ� ���� �� �ִ� ����
	result = SHRegGetPath(HKEY_CURRENT_USER, regMabinogi, _T(""), mabinogiPath, NULL);
	lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		return true;
	}

	// ���α׷����� ����ڰ� ������ ��ó�� ������ ������ ������
	result = SHRegGetPath(HKEY_CURRENT_USER, regMabinogi, _T("LauncherPath_"), mabinogiPath, NULL);
	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		return true;
	}

	// ��ġ���� �ʾ��� �� �����ϰ� ���� ��⸦ ã�� ���
	result = SHRegGetPath(HKEY_CURRENT_USER,
		_T("Software\\Ahnlab\\HShield\\eh*;o(.."),
		_T("GamePath"), mabinogiPath, NULL);

	if (result == ERROR_SUCCESS)
	{
		GetParentDirectory(mabinogiPath);
		lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
		if (FileExists(mabinogiPath))
		{
			return true;
		}
	}

	return false;
}

// �������� (����)��ο��� Mabinogi.exe �˻�
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

// Mabinogi.exe��θ� �پ��� ������� ã�Ƽ� �־���
LPCTSTR GetMabinogiPath()
{
	// ĳ��
	static TCHAR launcherPath[MAX_PATH] = {NULL, };
	if (FileExists(launcherPath))
	{
		return launcherPath;
	}

	static bool asked = false;
	if (asked)
	{
		return NULL;
	}
	asked = true;

	if (GetMabinogiPathOnRegistry  (launcherPath) ||
		GetMabinogiPathOnExecutable(launcherPath))
	{
		return launcherPath;
	}

	OPENFILENAME OFN;
	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.lpstrTitle = _T("������ ���������� �������ּ���.");
	OFN.lpstrFilter = _T("���� ����\0*.exe\0��� ����\0*.*\0");
	OFN.lpstrFile = launcherPath;
	OFN.nMaxFile = MAX_PATH;
	OFN.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&OFN))
	{
		// SHRegSetPath�� ���� ������ Ȯ���� �Ͼ�� �� ��.
		SHRegSetUSValue(regMabinogi, _T("LauncherPath_"), REG_SZ, launcherPath,
			lstrlen(launcherPath) * sizeof(launcherPath[0]), SHREGSET_FORCE_HKCU);
		return launcherPath;
	}

	return NULL;
}

BOOL SetPrivilege( 
	HANDLE hToken,  // token handle 
	LPCTSTR Privilege,  // Privilege to enable/disable 
	BOOL bEnablePrivilege  // TRUE to enable. FALSE to disable 
	) 
{ 
	TOKEN_PRIVILEGES tp = { 0 }; 
	// Initialize everything to zero 
	LUID luid; 
	DWORD cb=sizeof(TOKEN_PRIVILEGES); 
	if(!LookupPrivilegeValue( NULL, Privilege, &luid ))
		return FALSE; 
	tp.PrivilegeCount = 1; 
	tp.Privileges[0].Luid = luid; 
	if(bEnablePrivilege) { 
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
	} else { 
		tp.Privileges[0].Attributes = 0; 
	} 
	AdjustTokenPrivileges( hToken, FALSE, &tp, cb, NULL, NULL ); 
	if (GetLastError() != ERROR_SUCCESS) 
		return FALSE; 

	return TRUE;
}

void ChangeWindowStyle(HWND hWnd)
{
	// WS_CAPTION == WS_BORDER | WS_DLGFRAME�ε�, �� �� �ϳ��� ������ Ÿ��Ʋ �ٰ� �������.
	// ���ÿ� �ִ�ȭ�� �� �۾�ǥ���ٵ� ���������� �Ǵµ�, �̰� �ذ��� ����� ����ġ �ʴ�.
	const LONG windowedStyle = WS_CAPTION | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME | WS_OVERLAPPED | WS_MINIMIZEBOX;
	const LONG fullscreenStyle = WS_POPUP | WS_MINIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS;
	const LONG windowedFullscreenToggleStyle = WS_MAXIMIZEBOX | WS_BORDER | WS_THICKFRAME;
	const LONG maximizeToggleStyle = WS_MAXIMIZEBOX;
	LONG style;

#ifdef MABISCRVER_MAXIMIZE
	const LONG& toggleStyle = maximizeToggleStyle;
#else
	const LONG& toggleStyle = windowedFullscreenToggleStyle;
#endif

	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | 
		TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if (style & WS_POPUP)
	{
		MessageBox(hWnd, _T("â ���� �ٲٰ� �������ּ���."), lpszTitle, MB_OK);
		return;
	}
	style ^= toggleStyle;
	if (!SetWindowLongPtr(hWnd, GWL_STYLE, style))
	{
		ErrorMessageBox();
	}

	// �� �Լ� ȣ�� ������ �߿��ѵ�, ShowWindow�� ���߿� ȣ���Ű�� ���� ��üȭ������ �����Ѵ�.
#ifdef MABISCRVER_TASKBAR
	ShowWindow(hWnd, SW_MAXIMIZE);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#else
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	ShowWindow(hWnd, SW_MAXIMIZE);
#endif
}

bool SearchMabinogiFromSnapshot(HANDLE hSnap, PROCESSENTRY32& pe)
{
	do
	{
		if (StrStrI(pe.szExeFile, _T("Mabinogi.exe")) || 
			StrStrI(pe.szExeFile, _T("Client.exe")))
		{
			return true;
		}
	}
	while (Process32Next(hSnap, &pe));
	return false;
}

bool IsMabinogiAlive()
{
	HANDLE hSnap;
	PROCESSENTRY32 pe;

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		ErrorMessageBox();
		return false;
	}

	bool isAlive;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnap, &pe))
	{
		isAlive = SearchMabinogiFromSnapshot(hSnap, pe);
	}

	CloseHandle(hSnap);
	return isAlive;
}

void CALLBACK MonitoringTimer(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	KillTimer(hWnd, idEvent);

	HWND hMabiWin = FindWindow(_T("Mabinogi"), _T("������"));
	if (IsWindowVisible(hMabiWin))
	{
		ChangeWindowStyle(hMabiWin);
		PostQuitMessage(0);
	}

	if (!IsMabinogiAlive())
	{
		PostQuitMessage(0);
	}

	SetTimer(hWnd, idEvent, 500, MonitoringTimer);
}

void LaunchMabinogi()
{
	LPCTSTR mabinogiPath = GetMabinogiPath();
	if (mabinogiPath == NULL)
	{
		return;
	}

	// CreateProcess�� ���� DLL�� �ϳ� �� ������ ���ѻ���� �׻� �ʿ���
	TCHAR mabinogiDir[MAX_PATH];
	lstrcpy(mabinogiDir, mabinogiPath);
	GetParentDirectory(mabinogiDir);
	ShellExecute(NULL, _T("runas"), mabinogiPath, NULL, mabinogiDir, SW_SHOW);
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

	// �������� ���ϸ� Ÿ�̸ӿ��� IsMabinogiAlive�� �˾Ƽ� ������ �� ����.
	if (!IsMabinogiAlive())
	{
		LaunchMabinogi();
	}

	// Sleep() ���ѹݺ��� ������, �´� WaitForInput���� ���ְ� �� �Ǿ���� Ŀ����
	// ��׶��� �۾� ������ ��� �����ִ� ���� �߰�. ��� Ÿ�̸ӷ�ƾ.
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
