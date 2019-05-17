#include "stdafx.h"
#include "resource.h"
using namespace std;

// Predefined로 MABISCRVER_MAXIMIZE, MABISCRVER_TITLEBAR, MABISCRVER_FULLSCREEN 버전 3개가 있다
// resource.h에 정의해야 함

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

// 프로그램 이름
LPCTSTR lpszTitle = _T("Mabinogi Maximizer");
// 마비노기 세팅 레지스트리 경로
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

	asprintf(msg, _T("%s, 에러코드: %u\n%s%s"), comment, err, szKor, szEng);
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

	// 정상적으로 설치되었을 때 쉽게 경로를 구할 수 있는 정석
	result = SHRegGetPath(HKEY_CURRENT_USER, regMabinogi, _T(""), mabinogiPath, NULL);
	lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		return true;
	}

	// 프로그램에서 사용자가 지정한 런처를 저장한 레지를 가져옴
	result = SHRegGetPath(HKEY_CURRENT_USER, regMabinogi, _T("LauncherPath_"), mabinogiPath, NULL);
	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		return true;
	}

	// 설치하지 않았을 때 실행하고 남는 찌꺼기를 찾는 방법
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

// Mabinogi.exe경로를 다양한 방법으로 찾아서 넣어줌
LPCTSTR GetMabinogiPath()
{
	// 캐싱
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
	OFN.lpstrTitle = _T("마비노기 실행파일을 선택해주세요.");
	OFN.lpstrFilter = _T("실행 파일\0*.exe\0모든 파일\0*.*\0");
	OFN.lpstrFile = launcherPath;
	OFN.nMaxFile = MAX_PATH;
	OFN.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&OFN))
	{
		// SHRegSetPath를 쓰면 변수로 확장이 일어나서 안 됨.
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
	// WS_CAPTION = WS_BORDER | WS_DLGFRAME인데, 둘 중 하나라도 없으면 타이틀 바가 사라진다.
	// 동시에 최대화할 때 작업표시줄도 가려버리게 되는데, 이걸 해결할 방법이 마땅치 않다.
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
		MessageBox(hWnd, _T("창 모드로 바꾸고 실행해주세요."), lpszTitle, MB_OK);
		return;
	}
	style ^= toggleStyle;

	ApplyWindowstyle(hWnd, style);
	// 이 함수 호출 순서가 중요한데, ShowWindow를 나중에 호출시키면 완전 전체화면으로 시작한다.
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
		if (StrCmp(mes, _T("주의해주세요!")) != 0)
		{
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		HWND& hMsgBox = reinterpret_cast<HWND&>(wParam);
		HWND hStaticWarning = FindWindowEx((HWND)wParam, NULL, _T("Static"), NULL);
		if (GetWindowText(hStaticWarning, mes, _countof(mes)))
		{
			if (StrStrI(mes, _T("고객님의 시스템에 설치되어있는 그래픽 카드는")) &&
				StrStrI(mes, _T("계속 실행하시겠습니까?")))
			{
				HWND hButtonOK = FindWindowEx(hMsgBox, NULL, _T("Button"), _T("예(&Y)"));
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
		if (StrStrI(mes, _T("고객님의 시스템에 설치되어있는 그래픽 카드는")) &&
			StrStrI(mes, _T("계속 실행하시겠습니까?")))
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
	while (hMsgBox = FindWindowEx(NULL, hMsgBox, NULL, _T("주의해주세요!")))
	{
		HWND hStaticWarning = FindWindowEx(hMsgBox, NULL, _T("Static"), NULL);
		if (hStaticWarning &&
			IsIgnorableWarningMsgBox(hStaticWarning))
		{
			HWND hButtonOK = FindWindowEx(hMsgBox, NULL, _T("Button"), _T("예(&Y)"));
			SendDlgItemMessage(hMsgBox, GetDlgCtrlID(hButtonOK), BM_CLICK, NULL, NULL);
		}
	}
}

void CALLBACK MonitoringTimer(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	KillTimer(hWnd, idEvent);

	HWND hMabiWin = FindWindow(_T("Mabinogi"), _T("마비노기"));
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

	// ShellExecute는 runas로 관리자 권한을 얻어올 수 있지만 shell32.dll의존성을 추가하게 됨
	// 이건 아예 UAC Execution Level: requireAdministrator니까 상관 없음
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

	// 실행하지 못하면 타이머에서 IsMabinogiAlive가 알아서 종료해 줄 것임.
	if (!IsMabinogiAlive())
	{
    puts("Launch Mabinogi!");
		LaunchMabinogi();
	}

	// Sleep() 무한반복도 있지만, 걔는 WaitForInput으로 간주가 안 되어서인지 커서가
	// 백그라운드 작업 중으로 잠깐 남아있는 현상 발견. 고로 타이머루틴.
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
