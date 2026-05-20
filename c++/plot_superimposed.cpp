#include <iostream>
#include <vector>
#include <windows.h>
#include <time.h>
#include "Use1401.h"

#define BUFFER 1000  
#define NUM_CHANNELS 4
#define VOLT_RANGE 5.0

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

float g_data[NUM_CHANNELS][BUFFER] = {0};
int g_idx = 0;
bool g_paused = false;
DWORD g_startTime = 0;

int main()
{
    short hand = U14Open1401(0);
    if (hand < 0) {
        std::cerr << "Failed to open 1401\n";
        return -1;
    }
    g_startTime = GetTickCount();

    const char CLASS_NAME[] = "SuperPlotWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "TMS Superimposed Monitor (C++)", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700, 
        NULL, NULL, wc.hInstance, NULL);

    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_SPACE) {
                g_paused = !g_paused;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_paused) {
            U14SendString(hand, "adc,-3;");
            char line[128];
            if (U14GetString(hand, line, sizeof(line)) == U14ERR_NOERROR) {
                int v[4];
                if (sscanf(line, "%d %d %d %d", &v[0], &v[1], &v[2], &v[3]) == 4) {
                    for (int i = 0; i < 4; i++) {
                        g_data[i][g_idx] = (float)v[i] * VOLT_RANGE / 32768.0f;
                    }
                    g_idx = (g_idx + 1) % BUFFER;
                }
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        Sleep(2); 
    }

    U14Close1401(hand);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
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
        
        SYSTEMTIME st; GetLocalTime(&st);
        char headerStr[128];
        sprintf(headerStr, "Time: %02d:%02d:%02d | Elapsed: %.1fs | %s", 
                st.wHour, st.wMinute, st.wSecond, (float)(GetTickCount() - g_startTime) / 1000.0f,
                g_paused ? "PAUSED" : "RUNNING");
        
        SetTextColor(memDC, g_paused ? RGB(200, 0, 0) : RGB(50, 50, 50));
        SetBkMode(memDC, TRANSPARENT);
        TextOut(memDC, 20, 10, headerStr, strlen(headerStr));
        DeleteObject(hHeaderFont);

        // One Single Plot Area
        RECT pR = { 60, 50, w - 40, h_total - 60 };
        FillRect(memDC, &pR, (HBRUSH)GetStockObject(WHITE_BRUSH));
        
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        SelectObject(memDC, borderPen);
        MoveToEx(memDC, pR.left, pR.top, NULL); LineTo(memDC, pR.right, pR.top);
        LineTo(memDC, pR.right, pR.bottom); LineTo(memDC, pR.left, pR.bottom);
        LineTo(memDC, pR.left, pR.top);
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

        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            HPEN sPen = CreatePen(PS_SOLID, 1, colors[ch]);
            SelectObject(memDC, sPen);

            for (int i = 0; i < BUFFER - 1; i++) {
                int x1 = pR.left + (i * plot_w) / BUFFER;
                int y1 = y_center - (int)(g_data[ch][(g_idx + i) % BUFFER] * (plot_h / 12.0f));
                int x2 = pR.left + ((i + 1) * plot_w) / BUFFER;
                int y2 = y_center - (int)(g_data[ch][(g_idx + i + 1) % BUFFER] * (plot_h / 12.0f));
                MoveToEx(memDC, x1, y1, NULL); LineTo(memDC, x2, y2);
            }
            DeleteObject(sPen);
            
            // Channel Legend
            SetTextColor(memDC, colors[ch]);
            TextOut(memDC, pR.left + 10 + (ch * 100), pR.top + 10, labels[ch], 5);
        }
        SelectClipRgn(memDC, NULL); DeleteObject(hRgn);

        SetTextColor(memDC, RGB(100, 100, 100));
        TextOut(memDC, pR.left - 40, y_center - (int)(5.0f * (plot_h / 12.0f)), "+5V", 3);
        TextOut(memDC, pR.left - 40, y_center - 7, "0V", 2);
        TextOut(memDC, pR.left - 40, y_center + (int)(5.0f * (plot_h / 12.0f)) - 10, "-5V", 3);
        TextOut(memDC, w / 2 - 50, h_total - 35, "Time (Rolling Buffer)", 21);

        BitBlt(hdc, 0, 0, w, h_total, memDC, 0, 0, SRCCOPY);
        DeleteObject(memBM); DeleteDC(memDC); EndPaint(hwnd, &ps); return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
