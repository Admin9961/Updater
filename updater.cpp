#include <windows.h>
#include <wininet.h>
#include <commctrl.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "comctl32.lib")

#define WM_PROGRESS_UPDATE (WM_USER + 1)

HWND hProgressBar;
HWND hPercentageText;

std::string getLatestRelease(const std::string& owner, const std::string& repo) {
    HINTERNET hInternet = InternetOpen(L"GitHub Update Checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        MessageBox(NULL, L"Failed to initialize internet connection.", L"Error", MB_OK | MB_ICONERROR);
        return "";
    }

    std::string url = "https://api.github.com/repos/" + owner + "/" + repo + "/releases/latest"; // Script will always detect the last version by recostructing the full URL in the format "https://api.github.com/repos/{owner}/{repo}/releases/latest"
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), "User-Agent: UpdateChecker\r\n", -1L, INTERNET_FLAG_RELOAD, 0);

    if (!hConnect) {
        MessageBox(NULL, L"Failed to connect to GitHub API.", L"Error", MB_OK | MB_ICONERROR);
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[1024];
    DWORD bytesRead;
    std::string response;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return response;
}

std::string getJsonValue(const std::string& jsonResponse, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = jsonResponse.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colonPos = jsonResponse.find(":", keyPos);
    std::size_t quoteStart = jsonResponse.find("\"", colonPos);
    std::size_t quoteEnd = jsonResponse.find("\"", quoteStart + 1);

    if (quoteStart == std::string::npos || quoteEnd == std::string::npos) {
        return "";
    }

    return jsonResponse.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

void updateProgressBar(int progress, int totalSize) {
    SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
    int percentage = (int)((progress / (double)totalSize) * 100);

    std::wstringstream ws;
    ws << percentage << L"%";
    SetWindowText(hPercentageText, ws.str().c_str());
}

bool downloadFileWithProgress(HWND hwnd, const std::string& downloadURL, const std::string& localFile) {
    HINTERNET hInternet = InternetOpen(L"Downloader", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        MessageBox(hwnd, L"Failed to open internet connection.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    HINTERNET hConnect = InternetOpenUrlA(hInternet, downloadURL.c_str(), "User-Agent: UpdateChecker\r\n", -1L, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConnect) {
        MessageBox(hwnd, L"Failed to open URL.", L"Error", MB_OK | MB_ICONERROR);
        InternetCloseHandle(hInternet);
        return false;
    }

    DWORD fileSize = 0;
    DWORD fileSizeLen = sizeof(fileSize);
    HttpQueryInfo(hConnect, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &fileSize, &fileSizeLen, NULL);

    if (fileSize == 0) {
        MessageBox(hwnd, L"Failed to get file size.", L"Error", MB_OK | MB_ICONERROR);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    SendMessage(hProgressBar, PBM_SETRANGE32, 0, fileSize);

    std::ofstream outputFile(localFile, std::ios::binary);
    if (!outputFile.is_open()) {
        MessageBox(hwnd, L"Failed to open local file.", L"Error", MB_OK | MB_ICONERROR);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead;
    DWORD totalBytesRead = 0;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outputFile.write(buffer, bytesRead);
        totalBytesRead += bytesRead;
        updateProgressBar(totalBytesRead, fileSize);

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    outputFile.close();
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (totalBytesRead == fileSize) {
        MessageBox(hwnd, L"Download completed successfully.", L"Download Complete", MB_OK | MB_ICONINFORMATION);
        return true;
    }
    else {
        MessageBox(hwnd, L"Download failed.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE,
            20, 20, 360, 30, hwnd, NULL, GetModuleHandle(NULL), NULL);
        hPercentageText = CreateWindowEx(0, L"STATIC", L"0%", WS_CHILD | WS_VISIBLE | SS_CENTER,
            400, 20, 50, 30, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hProgressBar, PBM_SETRANGE32, 0, 100);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void runGUI(const std::string& downloadURL) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"UpdateCheckerClass";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"UpdateCheckerClass", L"Downloading Update", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 100, NULL, NULL, GetModuleHandle(NULL), NULL);

    ShowWindow(hwnd, SW_SHOW);

    downloadFileWithProgress(hwnd, downloadURL, "latest_version.zip");

    DestroyWindow(hwnd);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::string owner = "pyarchinit"; // This is an example, replace with actual owner
    std::string repo = "pyarchinit"; // This is an example, replace with actual repository name

    std::string jsonResponse = getLatestRelease(owner, repo);
    if (jsonResponse.empty()) {
        MessageBox(NULL, L"Failed to fetch update information.", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    std::string latestVersion = getJsonValue(jsonResponse, "tag_name");
    if (latestVersion.empty()) {
        MessageBox(NULL, L"Failed to get the latest version.", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    std::string currentVersion = "v4.6.6"; // Example assumed as the current version 

    if (latestVersion != currentVersion) {
        int response = MessageBox(NULL, L"New version detected, do you want to download?", L"Update Available", MB_YESNO | MB_ICONQUESTION);
        if (response == IDYES) {
            std::string downloadURL = getJsonValue(jsonResponse, "browser_download_url");
            if (downloadURL.empty()) {
                MessageBox(NULL, L"Failed to get the download URL.", L"Error", MB_OK | MB_ICONERROR);
                return 0;
            }

            runGUI(downloadURL);
        }
        else {
            MessageBox(NULL, L"Update canceled.", L"No Update", MB_OK | MB_ICONINFORMATION);
        }
    }
    else {
        MessageBox(NULL, L"You are up to date!", L"Up-to-date", MB_OK | MB_ICONINFORMATION);
    }

    return 0;
}