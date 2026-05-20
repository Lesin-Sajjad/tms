#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif
#include <iostream>
#include <vector>
#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include <stdio.h>
#include <mmsystem.h>
#include "Use1401.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

#define BUFFER 10000  
#define NUM_CHANNELS 4
#define VOLT_RANGE 5.0
#define SAMPLES_PER_POLL 10 // Request 10 samples at once for higher effective rate

// Control IDs
#define ID_X_SLIDER 1001
#define ID_Y_SLIDER 1002

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct DataPoint {
    float values[NUM_CHANNELS];
    DWORD timestamp;
};

// Global State
DataPoint g_buffer[BUFFER];
int g_idx = 0;
bool g_paused = false;
DWORD g_startTime = 0;

// Dynamic Zoom Settings
int g_viewWindowMs = 2000;  
float g_yRangeVolts = 1.0f; 

int main()
{
    // Improve timer resolution for Sleep(1)
    timeBeginPeriod(1);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    short hand = U14Open1401(0);
    if (hand < 0) {
        std::cerr << "Failed to open 1401\n";
        return -1;
    }
    g_startTime = GetTickCount();

    const char CLASS_NAME[] = "ZoomPlotWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "TMS Interactive Zoom Monitor (High Speed)", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, 
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 800, 
        NULL, NULL, wc.hInstance, NULL);

    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto cleanup;
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_SPACE) {
                g_paused = !g_paused;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_paused) {
            // Polling multiple samples in one go if possible, 
            // but the string interface is the bottleneck.
            // We'll just run the loop as fast as possible.
            U14SendString(hand, "adc,-3;");
            char line[128];
            if (U14GetString(hand, line, sizeof(line)) == U14ERR_NOERROR) {
                int v[4];
                if (sscanf(line, "%d %d %d %d", &v[0], &v[1], &v[2], &v[3]) == 4) {
                    g_buffer[g_idx].timestamp = GetTickCount();
                    for (int i = 0; i < 4; i++) {
                        g_buffer[g_idx].values[i] = (float)v[i] * VOLT_RANGE / 32768.0f;
                    }
                    g_idx = (g_idx + 1) % BUFFER;
                }
            }
        }
        
        // Only invalidate occasionally to save CPU, but polling is priority
        static DWORD lastDraw = 0;
        DWORD now = GetTickCount();
        if (now - lastDraw > 15) { // ~60 FPS UI
            InvalidateRect(hwnd, NULL, FALSE);
            lastDraw = now;
        }
        
        // Remove Sleep(1) or use Sleep(0) for maximum polling speed
        Sleep(0); 
    }

cleanup:
    timeEndPeriod(1);
    U14Close1401(hand);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hXSlider, hYSlider, hXLabel, hYLabel;

    switch (uMsg) {
    case WM_CREATE: {
        hXLabel = CreateWindowEx(0, "STATIC", "X-Zoom (Time Window)", WS_CHILD | WS_VISIBLE, 
            20, 0, 150, 20, hwnd, NULL, NULL, NULL);
        hXSlider = CreateWindowEx(0, TRACKBAR_CLASS, "X Zoom", 
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ, 
            180, 0, 300, 40, hwnd, (HMENU)ID_X_SLIDER, NULL, NULL);
        SendMessage(hXSlider, TBM_SETRANGE, TRUE, MAKELONG(100, 10000));
        SendMessage(hXSlider, TBM_SETPOS, TRUE, g_viewWindowMs);

        hYLabel = CreateWindowEx(0, "STATIC", "Y-Zoom (Voltage Range)", WS_CHILD | WS_VISIBLE, 
            520, 0, 150, 20, hwnd, NULL, NULL, NULL);
        hYSlider = CreateWindowEx(0, TRACKBAR_CLASS, "Y Zoom", 
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ, 
            680, 0, 300, 40, hwnd, (HMENU)ID_Y_SLIDER, NULL, NULL);
        SendMessage(hYSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 50));
        SendMessage(hYSlider, TBM_SETPOS, TRUE, (int)(g_yRangeVolts * 10.0f));
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        MoveWindow(hXLabel, 20, h - 50, 150, 20, TRUE);
        MoveWindow(hXSlider, 180, h - 60, 300, 40, TRUE);
        MoveWindow(hYLabel, 520, h - 50, 150, 20, TRUE);
        MoveWindow(hYSlider, 680, h - 60, 300, 40, TRUE);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_HSCROLL: {
        if ((HWND)lParam == hXSlider) {
            g_viewWindowMs = (int)SendMessage(hXSlider, TBM_GETPOS, 0, 0);
        } else if ((HWND)lParam == hYSlider) {
            int pos = (int)SendMessage(hYSlider, TBM_GETPOS, 0, 0);
            g_yRangeVolts = (float)pos / 10.0f;
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect; GetClientRect(hwnd, &rect);
        int w = rect.right, h_total = rect.bottom;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h_total);
        SelectObject(memDC, memBM);

        HBRUSH hBg = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        HFONT hHeaderFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
            OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
        SelectObject(memDC, hHeaderFont);
        
        DWORD now = GetTickCount();
        char headerStr[256];
        sprintf(headerStr, "X-Window: %dms | Y-Range: +/-%.1fV | %s", 
                g_viewWindowMs, g_yRangeVolts, g_paused ? "PAUSED" : "RUNNING");
        
        SetTextColor(memDC, RGB(50, 50, 50));
        SetBkMode(memDC, TRANSPARENT);
        TextOut(memDC, 20, 10, headerStr, strlen(headerStr));
        DeleteObject(hHeaderFont);

        RECT pR = { 70, 50, w - 40, h_total - 80 };
        FillRect(memDC, &pR, (HBRUSH)GetStockObject(WHITE_BRUSH));
        
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        SelectObject(memDC, borderPen);
        Rectangle(memDC, pR.left, pR.top, pR.right, pR.bottom);
        DeleteObject(borderPen);

        int y_center = pR.top + (pR.bottom - pR.top) / 2;
        int plot_h = pR.bottom - pR.top;
        int plot_w = pR.right - pR.left;

        HPEN gridPen = CreatePen(PS_DOT, 1, RGB(220, 220, 220));
        SelectObject(memDC, gridPen);
        MoveToEx(memDC, pR.left, y_center, NULL); LineTo(memDC, pR.right, y_center);
        DeleteObject(gridPen);

        COLORREF colors[] = {RGB(0, 100, 200), RGB(0, 150, 0), RGB(200, 100, 0), RGB(200, 0, 0)};
        const char* labels[] = {"ADC 0", "ADC 1", "ADC 2", "ADC 3"};

        HRGN hRgn = CreateRectRgn(pR.left + 1, pR.top + 1, pR.right - 1, pR.bottom - 1);
        SelectClipRgn(memDC, hRgn);

        DWORD windowStart = now - g_viewWindowMs;

        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            HPEN sPen = CreatePen(PS_SOLID, 1, colors[ch]);
            SelectObject(memDC, sPen);

            bool first = true;
            for (int i = 0; i < BUFFER; i++) {
                int idx = (g_idx + i) % BUFFER;
                if (g_buffer[idx].timestamp < windowStart) continue;

                long long tDiff = (long long)g_buffer[idx].timestamp - (long long)windowStart;
                int x = pR.left + (int)(tDiff * (long long)plot_w / (long long)g_viewWindowMs);
                
                float v = g_buffer[idx].values[ch];
                int y = y_center - (int)((v / g_yRangeVolts) * (plot_h / 2.0f));

                if (first) {
                    MoveToEx(memDC, x, y, NULL);
                    first = false;
                } else {
                    LineTo(memDC, x, y);
                }
            }
            DeleteObject(sPen);
            SetTextColor(memDC, colors[ch]);
            TextOut(memDC, pR.left + 10 + (ch * 100), pR.top + 10, labels[ch], strlen(labels[ch]));
        }
        SelectClipRgn(memDC, NULL); DeleteObject(hRgn);

        SetTextColor(memDC, RGB(100, 100, 100));
        char vStr[16];
        sprintf(vStr, "+%.1fV", g_yRangeVolts); TextOut(memDC, pR.left - 60, pR.top - 7, vStr, strlen(vStr));
        TextOut(memDC, pR.left - 60, y_center - 7, " 0.0V", 5);
        sprintf(vStr, "-%.1fV", g_yRangeVolts); TextOut(memDC, pR.left - 60, pR.bottom - 7, vStr, strlen(vStr));

        BitBlt(hdc, 0, 0, w, h_total, memDC, 0, 0, SRCCOPY);
        DeleteObject(memBM); DeleteDC(memDC); EndPaint(hwnd, &ps); return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
