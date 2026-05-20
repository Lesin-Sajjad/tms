#include <iostream>
#include <vector>
#include <windows.h>
#include <time.h>
#include "Use1401.h"

#define BUFFER 600
#define NUM_CHANNELS 4
#define VOLT_RANGE 5.0

// Simple window procedure for our plot
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global data for the plot
float g_data[NUM_CHANNELS][BUFFER] = {0};
int g_idx = 0;
bool g_paused = false;
DWORD g_startTime = 0;

int main()
{
    // 1. Initialize 1401
    short hand = U14Open1401(0);
    if (hand < 0) {
        std::cerr << "Failed to open 1401\n";
        return -1;
    }
    std::cout << "1401 connected. Press SPACE to Pause/Resume.\n";
    g_startTime = GetTickCount();

    // 2. Create Window for Plotting
    const char CLASS_NAME[] = "PlotWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "TMS C++ Plot (ADC 0-3)", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 700, 
        NULL, NULL, wc.hInstance, NULL);

    // 3. Main Loop
    MSG msg = {};
    bool running = true;
    while (running) {
        // Handle Windows Messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_SPACE) {
                g_paused = !g_paused;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_paused) {
            // Poll 1401
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

        // Trigger Repaint
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
        
        RECT rect;
        GetClientRect(hwnd, &rect);
        int w = rect.right;
        int h_total = rect.bottom;

        // Double buffering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h_total);
        SelectObject(memDC, memBM);

        // Background
        HBRUSH hBg = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        // Header Info (Clock & Status)
        HFONT hHeaderFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
            OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
        SelectObject(memDC, hHeaderFont);
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeStr[128];
        sprintf(timeStr, "Time: %02d:%02d:%02d.%03d | Elapsed: %.1fs", 
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, 
                (float)(GetTickCount() - g_startTime) / 1000.0f);
        
        SetTextColor(memDC, RGB(80, 80, 80));
        SetBkMode(memDC, TRANSPARENT);
        TextOut(memDC, 20, 10, timeStr, strlen(timeStr));

        if (g_paused) {
            SetTextColor(memDC, RGB(255, 0, 0));
            TextOut(memDC, w - 120, 10, "[ PAUSED ]", 10);
        } else {
            SetTextColor(memDC, RGB(0, 150, 0));
            TextOut(memDC, w - 120, 10, "[ RUNNING ]", 11);
        }
        DeleteObject(hHeaderFont);

        const char* titles[] = {"EMG 1 (ADC 0)", "EMG 2 (ADC 1)", "EMG 3 (ADC 2)", "TMS TRIGGER (ADC 3)"};
        COLORREF colors[] = {RGB(0, 100, 200), RGB(0, 150, 0), RGB(200, 100, 0), RGB(200, 0, 0)};
        
        int margin_x = 50;
        int margin_y = 50;
        int plot_h = (h_total - margin_y - 30) / NUM_CHANNELS;
        int plot_w = w - 2 * margin_x;

        HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
            OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
        SelectObject(memDC, hFont);

        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            RECT plotRect = { margin_x, margin_y + ch * plot_h + 10, 
                             margin_x + plot_w, margin_y + (ch + 1) * plot_h - 10 };
            
            // 1. Plot Background (White Box)
            FillRect(memDC, &plotRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
            
            // 2. Plot Outline
            HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
            SelectObject(memDC, borderPen);
            MoveToEx(memDC, plotRect.left, plotRect.top, NULL);
            LineTo(memDC, plotRect.right, plotRect.top);
            LineTo(memDC, plotRect.right, plotRect.bottom);
            LineTo(memDC, plotRect.left, plotRect.bottom);
            LineTo(memDC, plotRect.left, plotRect.top);
            DeleteObject(borderPen);

            int y_center = plotRect.top + (plotRect.bottom - plotRect.top) / 2;
            int h_inner = (plotRect.bottom - plotRect.top);

            // 3. Grid / Zero Line
            HPEN gridPen = CreatePen(PS_DOT, 1, RGB(220, 220, 220));
            SelectObject(memDC, gridPen);
            MoveToEx(memDC, plotRect.left, y_center, NULL);
            LineTo(memDC, plotRect.right, y_center);
            DeleteObject(gridPen);

            // 4. Labels & Scale
            SetTextColor(memDC, RGB(50, 50, 50));
            TextOut(memDC, margin_x, plotRect.top - 18, titles[ch], strlen(titles[ch]));
            
            SetTextColor(memDC, RGB(150, 150, 150));
            TextOut(memDC, margin_x - 30, y_center - (int)(5.0f * (h_inner / 12.0f)), "+5V", 3);
            TextOut(memDC, margin_x - 30, y_center - 7, "0V", 2);
            TextOut(memDC, margin_x - 30, y_center + (int)(5.0f * (h_inner / 12.0f)) - 10, "-5V", 3);

            // 5. Signal
            HPEN signalPen = CreatePen(PS_SOLID, 1, colors[ch]);
            SelectObject(memDC, signalPen);

            // Clipping region to keep signal inside the box
            HRGN hRgn = CreateRectRgn(plotRect.left + 1, plotRect.top + 1, plotRect.right - 1, plotRect.bottom - 1);
            SelectClipRgn(memDC, hRgn);

            for (int i = 0; i < BUFFER - 1; i++) {
                int real_idx = (g_idx + i) % BUFFER;
                int next_idx = (g_idx + i + 1) % BUFFER;

                int x1 = plotRect.left + (i * plot_w) / BUFFER;
                int y1 = y_center - (int)(g_data[ch][real_idx] * (h_inner / 12.0f));
                int x2 = plotRect.left + ((i + 1) * plot_w) / BUFFER;
                int y2 = y_center - (int)(g_data[ch][next_idx] * (h_inner / 12.0f));

                MoveToEx(memDC, x1, y1, NULL);
                LineTo(memDC, x2, y2);
            }
            SelectClipRgn(memDC, NULL);
            DeleteObject(hRgn);
            DeleteObject(signalPen);
        }

        TextOut(memDC, w / 2 - 30, h_total - 25, "Time (Samples)", 14);

        BitBlt(hdc, 0, 0, w, h_total, memDC, 0, 0, SRCCOPY);
        DeleteObject(hFont);
        DeleteObject(memBM);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
