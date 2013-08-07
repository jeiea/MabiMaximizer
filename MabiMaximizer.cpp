#include "stdafx.h"

// �����ϸ� ��üȭ�� ���, ���Ǹ� �����ϸ� �ִ�ȭ�� Ȱ��ȭ��Ų��.
//#define FULLSCREEN

// ���α׷� �̸�
LPCTSTR lpszTitle = _T("Mabinogi Maximizer");
// ������ ���� ������Ʈ�� ���
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

LPTSTR lstrrev(LPTSTR lpString)
{
	unsigned len = lstrlen(lpString) - 1;
	unsigned mid = lstrlen(lpString) / 2;

	for (unsigned idx = 0; idx < mid; idx++)
	{
		TCHAR swap = lpString[idx];
		lpString[idx] = lpString[len - idx];
		lpString[len - idx] = swap;
	}

	return lpString;
}

void ParseMabinogiFilesRegistry(TCHAR mabinogiFiles[])
{
	TCHAR launcher[MAX_PATH] = _T("");
	TCHAR* token, *prevToken = mabinogiFiles;

	prevToken = token = mabinogiFiles;
	while (token = StrPBrk(prevToken, _T("*%")))
	{
		if (*token == _T('%'))
		{
			if (!GetParentDirectory(launcher))
			{
				launcher[0] = NULL;
			}
			else
			{
				lstrcat(launcher, _T("\\"));
			}
		}
		else
		{
			*token = NULL;
			if (lstrcmpi(prevToken, _T("Client.exe")) == 0)
			{
				lstrcpy(mabinogiFiles, launcher);
				lstrcat(mabinogiFiles, _T("Mabinogi.exe"));
				return;
			}
			lstrcat(launcher, prevToken);
			lstrcat(launcher, _T("\\"));
		}
		prevToken = token + 1;
	}
}

bool GetMabinogiPathOnRegistry(TCHAR mabinogiPath[MAX_PATH])
{
	// ���������� ��ġ�Ǿ��� �� ���� ��θ� ���� �� �ִ� ����
	LONG result;
	result = SHRegGetPath(HKEY_CURRENT_USER, regMabinogi, _T(""), mabinogiPath, NULL);

	lstrcat(mabinogiPath, _T("\\Mabinogi.exe"));
	if (result == ERROR_SUCCESS && FileExists(mabinogiPath))
	{
		return true;
	}

	// ��ġ���� �ʾ��� �� �����ϰ� ���� ��⸦ ã�� ���
	TCHAR mabinogiFiles[65535];
	DWORD cbSize = sizeof(mabinogiFiles);
	result = SHGetValue(HKEY_CURRENT_USER, regMabinogi, _T("Files"), NULL, mabinogiFiles, &cbSize);

	if (result == ERROR_SUCCESS)
	{
		ParseMabinogiFilesRegistry(mabinogiFiles);
		if (FileExists(mabinogiFiles))
		{
			lstrcpy(mabinogiPath, mabinogiFiles);
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

// ������ ��ó��θ� �ֵ� ������θ� �ֵ� ������η� �ٲ㼭 ��������
void SetMabinogiPathOnRegistry(TCHAR mabinogiPath[MAX_PATH])
{
	TCHAR mabinogiRoot[MAX_PATH];
	lstrcpy(mabinogiRoot, mabinogiPath);
	
	if (FileExists(mabinogiRoot))
	{
		GetParentDirectory(mabinogiRoot);
	}

	SHRegSetUSValue(regMabinogi, _T(""), REG_SZ, mabinogiRoot,
		lstrlen(mabinogiRoot) * sizeof(mabinogiRoot[0]), SHREGSET_FORCE_HKCU);
	//SHRegSetPath(HKEY_CURRENT_USER, regMabinogi, _T(""), mabinogiRoot, NULL);
}

// Mabinogi.exe��θ� �پ��� ������� ã�Ƽ� �־���
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
	OFN.lpstrTitle = _T("������ ���������� �������ּ���.");
	OFN.lpstrFilter = TEXT("������ ��������(Mabinogi.exe)\0Mabinogi.exe\0��� ����\0*.*\0");
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
	// ���� ��Ÿ��: WS_CAPTION | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME | WS_OVERLAPPED | WS_MINIMIZEBOX
	// WS_CAPTION == WS_BORDER | WS_DLGFRAME�ε�, �� �� �ϳ��� ������ Ÿ��Ʋ �ٰ� �������.
	// ���ÿ� �ִ�ȭ�� �� �۾�ǥ���ٵ� ���������� �Ǵµ�, �̰� �ذ��� ����� ����ġ �ʴ�.
	const LONG fullscreenStyle = WS_MAXIMIZEBOX | WS_BORDER | WS_THICKFRAME;
	const LONG maximizeStyle = WS_MAXIMIZEBOX;
	LONG style;

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
#ifdef FULLSCREEN
	style ^= fullscreenStyle;
#else
	style ^= maximizeStyle;
#endif
	SetWindowLongPtr(hWnd, GWL_STYLE, style);
	SetClassLongPtr(hWnd, GCL_STYLE, style);

	ShowWindow(hWnd, SW_MAXIMIZE);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
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
		MessageBox(NULL, _T("���μ����� ������ �� �����ϴ�."), _T("����"), MB_OK);
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

	//ShellExecute(NULL, _T("runas"), mabinogiPath, NULL, NULL, SW_SHOW);
	CreateProcess(mabinogiPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
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

	return (int)Message.wParam;
}
