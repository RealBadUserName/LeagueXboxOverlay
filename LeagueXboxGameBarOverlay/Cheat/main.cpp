#pragma once
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <windows.h>
#include <vector>
#include <psapi.h>
#include <TlHelp32.h>
#include <vector>
#include <inttypes.h>
#include <string>

//forsleep and thread
#include <chrono>
#include <thread>

//my edit
DWORD nOfBytes;
ULONG_PTR cKey;
LPOVERLAPPED pid;
HANDLE game_handle;
LPCWSTR processName = L"Overlay";

void Lock(HANDLE hMutex);
void Unlock(HANDLE hMutex);

//main.cpp definitions
#define MEMSIZE 90024 



struct EllipseM
{
    float x;
    float y;
    float radiusX;
    float radiusY;
};


std::vector<EllipseM> Ellipses;

std::wstring GetAppToken(uint32_t process_id)
{
    DWORD dwSize = 0;
    DWORD dwResult;
    HANDLE hToken;
    PTOKEN_APPCONTAINER_INFORMATION pAppCoInfo;
    WCHAR wcsDebug[1024] = { 0, };
    WCHAR* pwSID = NULL;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);

    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
    {
        dwResult = GetLastError();
        swprintf_s(wcsDebug, _countof(wcsDebug), L"OpenProcessToken Error(%u) PID(%d)\n", dwResult, process_id);
        printf("Overlay.exe not open in xbox game bar");
        return 0;
    }

    if (!GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS)TokenAppContainerSid, NULL, dwSize, &dwSize))
    {
        dwResult = GetLastError();
        if (dwResult != ERROR_INSUFFICIENT_BUFFER)
        {
            swprintf_s(wcsDebug, _countof(wcsDebug), L"GetTokenInformation Error %u\n", dwResult);
            return 0;
        }
    }

    pAppCoInfo = (PTOKEN_APPCONTAINER_INFORMATION)GlobalAlloc(GPTR, dwSize);

    if (!GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS)TokenAppContainerSid, pAppCoInfo, dwSize, &dwSize))
    {
        dwResult = GetLastError();
        swprintf_s(wcsDebug, _countof(wcsDebug), L"GetTokenInformation Error %u\n", dwResult);
        return 0;
    }

    typedef BOOL(WINAPI* tConvertSidToStringSid)(PSID, LPTSTR*);

    tConvertSidToStringSid pConvertSidToStringSid = (tConvertSidToStringSid)GetProcAddress(LoadLibrary(L"Advapi32.dll"), "ConvertSidToStringSidW");

    wchar_t* psz;
    PSID sid = pAppCoInfo->TokenAppContainer;
    if (pConvertSidToStringSid(sid, &psz)) {
        return psz;
    }

    return 0;
}


auto HandleReceiver(HANDLE* io_port) {
    char buffer[MAX_PATH];
    bool thingon = true;
    while (GetQueuedCompletionStatus(*io_port, &nOfBytes, &cKey, &pid, -1))
        if (nOfBytes == 6) {
            game_handle = OpenProcess(PROCESS_ALL_ACCESS, true, (DWORD)pid);
            GetModuleFileNameExA(game_handle, 0, buffer, MAX_PATH);
            printf("Games handle : %08x for %d (%s)\n", game_handle, (uint32_t)pid, buffer);
        }
}

void ProcessFinder(LPCWSTR processName) {
    auto pid = 0UL;
    auto desk_hwnd = FindWindow(0, processName); //auto desk_hwnd = FindWindow(0, L"Steam"); for steam games else GetShellWindow();
    auto ret = GetWindowThreadProcessId(desk_hwnd, &pid);
    auto exp_handle = OpenProcess(PROCESS_ALL_ACCESS, true, pid);
    auto io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
    auto job_object = CreateJobObjectW(0, 0);
    auto job_io_port = JOBOBJECT_ASSOCIATE_COMPLETION_PORT{ 0, io_port };
    auto result = SetInformationJobObject(job_object, JobObjectAssociateCompletionPortInformation, &job_io_port, sizeof(job_io_port));
    result = AssignProcessToJobObject(job_object, exp_handle);
    auto threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)HandleReceiver, &io_port, 0, 0);
    WaitForSingleObject(threadHandle, 1);
    CloseHandle(exp_handle);
}

void Lock(HANDLE hMutex) {
    if (hMutex != 0)
        WaitForSingleObject(hMutex, INFINITY);
}

void Unlock(HANDLE hMutex) {
    if (hMutex != 0)
        ReleaseMutex(hMutex);
}

void SceneUpdater() {
    for (float i = 0; i < 2560; i++) {
        std::cout << i << std::endl;
        //reset
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SceneUpdater, 0, 0, 0);

    ProcessFinder(processName);
    std::wstring appToken = GetAppToken((uint32_t)pid);
    std::wstring smHandle = L"AppContainerNamedObjects\\" + appToken + L"\\SharedMemory";
    std::wstring mHandle = L"AppContainerNamedObjects\\" + appToken + L"\\Mutex";

    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, NULL, mHandle.c_str());
    if (hMutex == 0) {
        printf("OpenMutex error: %d\n", GetLastError());
        return 0;
    }

    HANDLE hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct EllipseM), smHandle.c_str());

    if (hMapping == 0) {
        printf("CreateFileMapping error: %d\n", GetLastError());
        return 1;
    }

        
    int Esize = Ellipses.size();
    EllipseM* EArray = new EllipseM[Esize];
    EllipseM* ellipse = (EllipseM*)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    while (GetAsyncKeyState(VK_END) == 0) {
        Lock(hMutex);
        
        POINT p;
        
        if (ellipse) {
            {

                if (GetCursorPos(&p))
                {
                    ellipse->x = (float)p.x;
                    ellipse->y = (float)p.y;
                    ellipse->radiusX = 350;
                    ellipse->radiusY= 150;
                    std::cout << "x: " << (float)p.x << " y: " << (float)p.y << " Ellipse address: " << ellipse << std::endl;
                }
            }
        }
        else {
            std::cout << "Lost box address or mutex is out of memeory space" << std::endl;
        }
       

        Unlock(hMutex);
            
    }

    CloseHandle(hMapping);
    CloseHandle(hMutex);

    return 2;
}