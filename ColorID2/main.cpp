// Full implementation with color matching, highlight blending, and RGB display using GDI+
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

HINSTANCE hInst;
HWND hwndOverlay = NULL, hwndExit = NULL;
HBITMAP hSnapshot = NULL;
int selectedScreen = 0;
bool appRunning = true;
Bitmap* snapshotBitmap = nullptr;

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ExitWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

ATOM RegisterOverlayClass();
ATOM RegisterExitWindowClass();
void CreateOverlayWindow(int screenIdx);
void CreateExitWindow();
void UpdateOverlay();
double ColorDistance(const Color& c1, const Color& c2);

// Utility functions
int GetMonitorCount();
BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam);
HBITMAP CaptureScreen(int screenIdx);
BOOL GetMonitorInfoFromIndex(int index, MONITORINFOEX* info);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    hInst = hInstance;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    RegisterOverlayClass();
    RegisterExitWindowClass();

    CreateExitWindow();

    MSG msg;
    while (appRunning && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hSnapshot)
        DeleteObject(hSnapshot);
    if (snapshotBitmap)
        delete snapshotBitmap;

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

ATOM RegisterOverlayClass() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "OverlayWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    return RegisterClassA(&wc);
}

ATOM RegisterExitWindowClass() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = ExitWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "ExitWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    return RegisterClassA(&wc);
}

void CreateExitWindow() {
    hwndExit = CreateWindowExA(0, "ExitWindow", "Exit Info",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
        NULL, NULL, hInst, NULL);

    CreateWindowA("STATIC", "Press Alt+Esc to exit",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 10, 280, 30, hwndExit, NULL, hInst, NULL);

    HWND combo = CreateWindowA("COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        10, 50, 100, 200, hwndExit, (HMENU)1, hInst, NULL);

    // Add RGB display text box
    CreateWindowA("EDIT", "R: 0, G: 0, B: 0",
        WS_CHILD | WS_VISIBLE | ES_READONLY | ES_CENTER,
        10, 90, 280, 30, hwndExit, (HMENU)2, hInst, NULL);

    int monitors = GetMonitorCount();
    for (int i = 0; i < monitors; ++i) {
        std::string label = "Screen " + std::to_string(i + 1);
        SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageA(combo, CB_SETCURSEL, 0, 0);

    ShowWindow(hwndExit, SW_SHOW);
    UpdateWindow(hwndExit);

    RegisterHotKey(hwndExit, 1, MOD_ALT, VK_ESCAPE);

    // Start color update thread
    std::thread([]() {
        while (appRunning && hwndExit) {
            if (snapshotBitmap && hwndOverlay) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwndOverlay, &pt);
                
                Color underCursor;
                snapshotBitmap->GetPixel(pt.x, pt.y, &underCursor);
                
                // Update RGB text
                char rgbText[32];
                sprintf_s(rgbText, "R: %d, G: %d, B: %d", 
                    underCursor.GetR(), underCursor.GetG(), underCursor.GetB());
                SetDlgItemTextA(hwndExit, 2, rgbText);
                
                // Update window background
                HBRUSH hBrush = CreateSolidBrush(RGB(underCursor.GetR(), underCursor.GetG(), underCursor.GetB()));
                SetClassLongPtr(hwndExit, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush);
                InvalidateRect(hwndExit, NULL, TRUE);
                DeleteObject(hBrush);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }).detach();
}

void CreateOverlayWindow(int screenIdx) {
    // Clean up previous overlay if it exists
    if (hwndOverlay) {
        DestroyWindow(hwndOverlay);
        hwndOverlay = NULL;
    }
    if (snapshotBitmap) {
        delete snapshotBitmap;
        snapshotBitmap = nullptr;
    }
    if (hSnapshot) {
        DeleteObject(hSnapshot);
        hSnapshot = NULL;
    }

    hSnapshot = CaptureScreen(screenIdx);
    if (!hSnapshot) return;

    snapshotBitmap = Bitmap::FromHBITMAP(hSnapshot, NULL);

    MONITORINFOEX info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoFromIndex(screenIdx, &info)) return;

    hwndOverlay = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        "OverlayWindow", NULL, WS_POPUP,
        info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left,
        info.rcMonitor.bottom - info.rcMonitor.top,
        NULL, NULL, hInst, NULL);

    SetLayeredWindowAttributes(hwndOverlay, RGB(0, 0, 0), 255, LWA_ALPHA);
    ShowWindow(hwndOverlay, SW_SHOW);
    UpdateWindow(hwndOverlay);

    // Start overlay update loop with mouse position tracking
    std::thread([=]() {
        POINT lastPos = { -1, -1 };
        while (appRunning && hwndOverlay) {
            POINT currentPos;
            GetCursorPos(&currentPos);
            if (currentPos.x != lastPos.x || currentPos.y != lastPos.y) {
                UpdateOverlay();
                lastPos = currentPos;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }).detach();
}

#undef max
/*LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT && snapshotBitmap) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Graphics graphics(hdc);

        graphics.DrawImage(snapshotBitmap, 0, 0);

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        Color underCursor;
        snapshotBitmap->GetPixel(pt.x, pt.y, &underCursor);

        int width = snapshotBitmap->GetWidth();
        int height = snapshotBitmap->GetHeight();

        Bitmap overlay(width, height, PixelFormat32bppARGB);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color pix;
                snapshotBitmap->GetPixel(x, y, &pix);
                double dist = ColorDistance(underCursor, pix);
                //BYTE alpha = (BYTE)std::max(180 - dist * 0.1, 30.0);
                //BYTE alpha = (BYTE)std::max((double)(180 - dist * 3), 30.0);
                //BYTE alpha = dist < 50 ? (BYTE)(180 - dist * 0.1) : 255; // Fully black (opaque) for non-matching colors
                BYTE alpha = (BYTE)(dist < 200 ? 255 - (dist * 100) : 200);
                overlay.SetPixel(x, y, Color(alpha, 0, 0, 0));
                //overlay.SetPixel(x, y, Color(alpha, pix.GetR(), pix.GetG(), pix.GetB()));
            }
        }

        graphics.DrawImage(&overlay, 0, 0);
        EndPaint(hwnd, &ps);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
*/
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT && snapshotBitmap) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Graphics graphics(hdc);

        graphics.DrawImage(snapshotBitmap, 0, 0);

        // Get cursor position in screen coordinates
        POINT pt;
        GetCursorPos(&pt);

        // Find which monitor the cursor is on
        HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

        // Get monitor info
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };
        GetMonitorInfo(hMonitor, &monitorInfo);

        // Get device context for the specific monitor
        HDC hdcScreen = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
        if (!hdcScreen) {
            EndPaint(hwnd, &ps);
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        // Get the color under cursor
        COLORREF color = GetPixel(hdcScreen, pt.x, pt.y);
        DeleteDC(hdcScreen);

        Color underCursor(GetRValue(color), GetGValue(color), GetBValue(color));

        int width = snapshotBitmap->GetWidth();
        int height = snapshotBitmap->GetHeight();

        Bitmap overlay(width, height, PixelFormat32bppARGB);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color pix;
                snapshotBitmap->GetPixel(x, y, &pix);
                double dist = ColorDistance(underCursor, pix);
                BYTE alpha = (BYTE)(dist < 50 ? 255 - (dist * 100) : 230);
                overlay.SetPixel(x, y, Color(alpha, 0, 0, 0));
            }
        }

        graphics.DrawImage(&overlay, 0, 0);
        EndPaint(hwnd, &ps);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ExitWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == 1) {
            HWND combo = (HWND)lParam;
            selectedScreen = SendMessageA(combo, CB_GETCURSEL, 0, 0);
            CreateOverlayWindow(selectedScreen);
        }
        break;
    case WM_HOTKEY:
        if (wParam == 1) {
            appRunning = false;
            PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        appRunning = false;
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void UpdateOverlay() {
    if (!hwndOverlay) return;
    InvalidateRect(hwndOverlay, NULL, FALSE);
}

double ColorDistance(const Color& c1, const Color& c2) {
    return sqrt(pow((int)c1.GetR() - c2.GetR(), 2) +
        pow((int)c1.GetG() - c2.GetG(), 2) +
        pow((int)c1.GetB() - c2.GetB(), 2));
}

// Implementation of monitor utility functions
std::vector<HMONITOR> g_monitors;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
    g_monitors.push_back(hMon);
    return TRUE;
}

int GetMonitorCount() {
    g_monitors.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
    return (int)g_monitors.size();
}

HBITMAP CaptureScreen(int screenIdx) {
    if (screenIdx < 0 || screenIdx >= (int)g_monitors.size()) return NULL;

    MONITORINFOEXA mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);

    GetMonitorInfo(g_monitors[screenIdx], &mi);

    int width = mi.rcMonitor.right - mi.rcMonitor.left;
    int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, width, height);
    HGDIOBJ old = SelectObject(hdcMem, hbm);
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, mi.rcMonitor.left, mi.rcMonitor.top, SRCCOPY);
    SelectObject(hdcMem, old);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hbm;
}

BOOL GetMonitorInfoFromIndex(int index, MONITORINFOEX* info) {
    if (index < 0 || index >= (int)g_monitors.size()) return FALSE;
    return GetMonitorInfo(g_monitors[index], info);
}