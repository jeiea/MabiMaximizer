#include "stdafx.h"
#include "resource.h"
using namespace std;

// Predefined�� MABISCRVER_MAXIMIZE, MABISCRVER_TITLEBAR, MABISCRVER_FULLSCREEN ���� 3���� �ִ�
// resource.h�� �����ؾ� ��

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

// ���α׷� �̸�
LPCTSTR lpszTitle = _T("Mabinogi Maximizer");
// ������ ���� ������Ʈ�� ���
LPCTSTR regMabinogi = _T("Software\\Nexon\\Mabinogi");


// Ensure value can be passed to free()
int asprintf(LPTSTR& str, LPCTSTR format, va_list ap)
{
	str = NULL;

	int count = _vstprintf_p(NULL, 0, format, ap);
	if (count >= 0)
	{
		TCHAR* buffer = (TCHAR*)malloc(sizeof(TCHAR) * (count + 1));
		if (buffer == NULL)
			return -1;

		count = _vstprintf_p(buffer, count + 1, format, ap);
		if (count < 0)
		{
			free(buffer);
			return count;
		}
		str = buffer;
	}

	return count;
}

// Ensure value can be passed to free()
int asprintf(LPTSTR& str, LPCTSTR format, ...)
{
	va_list ap;
	va_start(ap, format);
	int count = asprintf(str, format, ap);
	va_end(ap);

	return count;
}

#define ERRMSGBOX() ErrorMessageBox(_T(__FILE__) _T(":") _T(STRINGIZE(__LINE__)))
int ErrorMessageBox(LPCTSTR comment = _T(""))
{
	DWORD err = GetLastError();

	TCHAR title[64];
	wsprintf(title, _T("MabiMaximizer Error %u"), err);

	LPTSTR szEng, szKor, msg;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&szEng, 0, NULL);
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&szKor, 0, NULL);

	asprintf(msg, _T("%s, �����ڵ�: %u\n%s%s"), comment, err, szKor, szEng);
	LocalFree(&szEng);
	LocalFree(&szKor);

	MessageBox(NULL, msg, title, MB_OK | MB_ICONERROR);
	free(msg);
	return err;
}

// http://stackoverflow.com/a/6218957
BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES)) &&
		   !(dwAttrib &  FILE_ATTRIBUTE_DIRECTORY);
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

	if (GetMabinogiPathOnRegistry(launcherPath) ||
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

void ApplyWindowstyle(HWND hWnd, LONG style)
{
	SetLastError(0);
	if (!SetWindowLongPtr(hWnd, GWL_STYLE, style))
	{
		ERRMSGBOX();
	}
}

void ChangeWindowStyle(HWND hWnd)
{
	// WS_CAPTION = WS_BORDER | WS_DLGFRAME�ε�, �� �� �ϳ��� ������ Ÿ��Ʋ �ٰ� �������.
	// ���ÿ� �ִ�ȭ�� �� �۾�ǥ���ٵ� ���������� �Ǵµ�, �̰� �ذ��� ����� ����ġ �ʴ�.
	const LONG windowedStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS & ~WS_MAXIMIZEBOX;
	const LONG fullscreenStyle = WS_POPUP | WS_MINIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS;
	const LONG windowedFullscreenToggleStyle = WS_BORDER | WS_THICKFRAME;
	const LONG maximizeToggleStyle = WS_MAXIMIZEBOX;
	LONG style;

#ifdef MABISCRVER_FULLSCREEN
	const LONG& toggleStyle = windowedFullscreenToggleStyle;
#else
	const LONG& toggleStyle = maximizeToggleStyle;
#endif

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if (style & WS_POPUP)
	{
		MessageBox(hWnd, _T("â ���� �ٲٰ� �������ּ���."), lpszTitle, MB_OK);
		return;
	}
	style ^= toggleStyle;

	ApplyWindowstyle(hWnd, style);
	// �� �Լ� ȣ�� ������ �߿��ѵ�, ShowWindow�� ���߿� ȣ���Ű�� ���� ��üȭ������ �����Ѵ�.
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	ShowWindow(hWnd, SW_MAXIMIZE);
#ifdef MABISCRVER_TITLEBAR
	ApplyWindowstyle(hWnd, style ^ windowedFullscreenToggleStyle);
	//ShowWindow(hWnd, SW_MAXIMIZE);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#endif
}

DWORD SearchMabinogiFromSnapshot(HANDLE hSnap, PROCESSENTRY32& pe)
{
	do
	{
		if (StrStrI(pe.szExeFile, _T("Mabinogi.exe")) ||
			StrStrI(pe.szExeFile, _T("Client.exe")))
		{
			return pe.th32ProcessID;
		}
	}
	while (Process32Next(hSnap, &pe));
	return false;
}

DWORD IsMabinogiAlive()
{
	HANDLE hSnap;
	PROCESSENTRY32 pe;

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		ERRMSGBOX();
		return false;
	}

	DWORD isAlive = false;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnap, &pe))
	{
		isAlive = SearchMabinogiFromSnapshot(hSnap, pe);
	}
	else
	{
		ERRMSGBOX();
	}

	CloseHandle(hSnap);
	return isAlive;
}

__declspec(dllexport)
LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HSHELL_WINDOWCREATED)
	{
		TCHAR mes[MAX_PATH];

		GetWindowText((HWND)wParam, mes, MAX_PATH);
		if (StrCmp(mes, _T("�������ּ���!")) != 0)
		{
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		HWND& hMsgBox = reinterpret_cast<HWND&>(wParam);
		HWND hStaticWarning = FindWindowEx((HWND)wParam, NULL, _T("Static"), NULL);
		if (GetWindowText(hStaticWarning, mes, _countof(mes)))
		{
			if (StrStrI(mes, _T("������ �ý��ۿ� ��ġ�Ǿ��ִ� �׷��� ī���")) &&
				StrStrI(mes, _T("��� �����Ͻðڽ��ϱ�?")))
			{
				HWND hButtonOK = FindWindowEx(hMsgBox, NULL, _T("Button"), _T("��(&Y)"));
				SendDlgItemMessage(hMsgBox, GetDlgCtrlID(hButtonOK), BM_CLICK, NULL, NULL);
				return 0;
			}
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool IsIgnorableWarningMsgBox(HWND hStaticWarning)
{
	TCHAR mes[MAX_PATH];
	if (GetWindowText(hStaticWarning, mes, sizeof(mes) / sizeof(mes[0])))
	{
		if (StrStrI(mes, _T("������ �ý��ۿ� ��ġ�Ǿ��ִ� �׷��� ī���")) &&
			StrStrI(mes, _T("��� �����Ͻðڽ��ϱ�?")))
		{
			return true;
		}
	}
	else
	{
		ERRMSGBOX();
	}
	return false;
}

void PassIgnorableWarningMsgBox()
{
	HWND hMsgBox = NULL;
	while (hMsgBox = FindWindowEx(NULL, hMsgBox, NULL, _T("�������ּ���!")))
	{
		HWND hStaticWarning = FindWindowEx(hMsgBox, NULL, _T("Static"), NULL);
		if (hStaticWarning &&
			IsIgnorableWarningMsgBox(hStaticWarning))
		{
			HWND hButtonOK = FindWindowEx(hMsgBox, NULL, _T("Button"), _T("��(&Y)"));
			SendDlgItemMessage(hMsgBox, GetDlgCtrlID(hButtonOK), BM_CLICK, NULL, NULL);
		}
	}
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

	PassIgnorableWarningMsgBox();

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

	// ShellExecute�� runas�� ������ ������ ���� �� ������ shell32.dll�������� �߰��ϰ� ��
	// �̰� �ƿ� UAC Execution Level: requireAdministrator�ϱ� ��� ����
	STARTUPINFO si = {sizeof(STARTUPINFO),};
	PROCESS_INFORMATION pi;
	TCHAR mabinogiDir[MAX_PATH];
	lstrcpy(mabinogiDir, mabinogiPath);
	GetParentDirectory(mabinogiDir);
  printf("Mabinogi Launch path: ");
	_putws(mabinogiDir);
	if (CreateProcess(mabinogiPath, NULL, NULL, NULL, FALSE, 0, NULL, mabinogiDir, &si, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	else
	{
		ERRMSGBOX();
	}
}

int APIENTRY _tWinMain(_In_     HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_     LPTSTR    lpCmdLine,
					   _In_     int       nCmdShow)
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
    puts("Launch Mabinogi!");
		LaunchMabinogi();
	}

	// Sleep() ���ѹݺ��� ������, �´� WaitForInput���� ���ְ� �� �Ǿ���� Ŀ����
	// ��׶��� �۾� ������ ��� �����ִ� ���� �߰�. ��� Ÿ�̸ӷ�ƾ.
	HANDLE thisMod = GetModuleHandle(NULL);
	if (thisMod == NULL)
		thisMod = LoadLibrary(__targv[0]);
	if (thisMod == NULL)
		ERRMSGBOX();

	DWORD hMabinogiMod = IsMabinogiAlive();
  puts("Alive test.");
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		ERRMSGBOX();
		return false;
	}

	//vector<HHOOK> hhk;
	//DWORD isAlive = false;
	//THREADENTRY32 threadEntry = {0, };
	//threadEntry.dwSize = sizeof(threadEntry);
	//if (Thread32First(hSnap, &threadEntry))
	//{
	//  do
	//  {
	//      if (threadEntry.th32OwnerProcessID == hMabinogiMod)
	//      {
	//          HINSTANCE mod = GetModuleHandle(NULL);
	//          hhk.push_back(SetWindowsHookEx(
	//                            WH_SHELL, ShellProc, mod,
	//                            threadEntry.th32ThreadID));
	//          if (hhk.back() == NULL)
	//              ERRMSGBOX();
	//      }
	//  }
	//  while (Thread32Next(hSnap, &threadEntry));
	//}

	LONG timerID;
	timerID = SetTimer(NULL, 0, 500, MonitoringTimer);

	MSG Message;
	while (GetMessage(&Message, NULL, 0, 0))
	{
		DispatchMessage(&Message);
	}
	KillTimer(NULL, timerID);

	//for (auto hook : hhk)
	//  UnhookWindowsHookEx(hook);

	CloseHandle(hMutex);
	return (int)Message.wParam;
}

int _tmain()
{
  return _tWinMain(0, 0, 0, 0);
}
