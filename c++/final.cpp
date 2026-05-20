#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <iostream>
#include <stdio.h>
#include "Use1401.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"winmm.lib")

#define BUFFER 10000
#define NUM_CHANNELS 4
#define VOLT_RANGE 5.0

#define STREAM_BLOCK 256
#define STREAM_BYTES (STREAM_BLOCK * NUM_CHANNELS * sizeof(short))

#define ID_X_SLIDER 1001
#define ID_Y_SLIDER 1002

LRESULT CALLBACK WindowProc(HWND,UINT,WPARAM,LPARAM);

struct DataPoint{
    float values[NUM_CHANNELS];
    DWORD timestamp;
};

DataPoint g_buffer[BUFFER];
int g_idx=0;

short g_streamBuffer[STREAM_BLOCK*NUM_CHANNELS];

DWORD g_startTime=0;
bool g_paused=false;

int g_viewWindowMs=2000;
float g_yRangeVolts=1.0f;

short g_hand;

int main(){

timeBeginPeriod(1);

INITCOMMONCONTROLSEX icex;
icex.dwSize=sizeof(icex);
icex.dwICC=ICC_BAR_CLASSES;
InitCommonControlsEx(&icex);

g_hand=U14Open1401(0);

if(g_hand<0){
std::cout<<"Failed to open 1401\n";
return -1;
}

U14SendString(g_hand,"KILL,ADCMEM;");
Sleep(20);

U14SendString(g_hand,"ADCMEM,I,2,0,65536,0 3,1,C,10,10;");

g_startTime=GetTickCount();

const char CLASS_NAME[]="TMSPlot";

WNDCLASS wc={};
wc.lpfnWndProc=WindowProc;
wc.hInstance=GetModuleHandle(NULL);
wc.lpszClassName=CLASS_NAME;
wc.hCursor=LoadCursor(NULL,IDC_ARROW);
wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);

RegisterClass(&wc);

HWND hwnd=CreateWindowEx(
0,CLASS_NAME,
"TMS EMG Monitor (Fast ADCMEM)",
WS_OVERLAPPEDWINDOW|WS_VISIBLE,
CW_USEDEFAULT,CW_USEDEFAULT,
1100,800,
NULL,NULL,wc.hInstance,NULL
);

MSG msg={};

while(true){

while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
if(msg.message==WM_QUIT)goto cleanup;

if(msg.message==WM_KEYDOWN && msg.wParam==VK_SPACE)
g_paused=!g_paused;

TranslateMessage(&msg);
DispatchMessage(&msg);
}

if(!g_paused){

short err=U14ToHost(
g_hand,
(char*)g_streamBuffer,
STREAM_BYTES,
0,
0
);

if(err==U14ERR_NOERROR){

DWORD now=GetTickCount();

for(int i=0;i<STREAM_BLOCK;i++){

g_buffer[g_idx].timestamp=now;

for(int ch=0;ch<NUM_CHANNELS;ch++){

short v=g_streamBuffer[i*NUM_CHANNELS+ch];

g_buffer[g_idx].values[ch]=
(float)v*VOLT_RANGE/32768.0f;

}

g_idx=(g_idx+1)%BUFFER;
}

}
}

static DWORD lastDraw=0;
DWORD now=GetTickCount();

if(now-lastDraw>15){
InvalidateRect(hwnd,NULL,FALSE);
lastDraw=now;
}

}

cleanup:

U14SendString(g_hand,"KILL,ADCMEM;");
Sleep(10);

U14Close1401(g_hand);

timeEndPeriod(1);

return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){

static HWND xSlider,ySlider;

switch(msg){

case WM_CREATE:
{
xSlider=CreateWindowEx(
0,TRACKBAR_CLASS,"",
WS_CHILD|WS_VISIBLE|TBS_AUTOTICKS,
180,740,300,40,
hwnd,(HMENU)ID_X_SLIDER,NULL,NULL);

SendMessage(xSlider,TBM_SETRANGE,TRUE,MAKELONG(100,10000));
SendMessage(xSlider,TBM_SETPOS,TRUE,g_viewWindowMs);

ySlider=CreateWindowEx(
0,TRACKBAR_CLASS,"",
WS_CHILD|WS_VISIBLE|TBS_AUTOTICKS,
680,740,300,40,
hwnd,(HMENU)ID_Y_SLIDER,NULL,NULL);

SendMessage(ySlider,TBM_SETRANGE,TRUE,MAKELONG(1,50));
SendMessage(ySlider,TBM_SETPOS,TRUE,(int)(g_yRangeVolts*10));

return 0;
}
case WM_HSCROLL:
{
if((HWND)lParam==xSlider)
g_viewWindowMs=SendMessage(xSlider,TBM_GETPOS,0,0);

if((HWND)lParam==ySlider)
g_yRangeVolts=SendMessage(ySlider,TBM_GETPOS,0,0)/10.0f;

return 0;
}
case WM_PAINT:
{
PAINTSTRUCT ps;
HDC hdc=BeginPaint(hwnd,&ps);

RECT r;
GetClientRect(hwnd,&r);

FillRect(hdc,&r,(HBRUSH)GetStockObject(WHITE_BRUSH));

int left=70;
int top=40;
int right=r.right-40;
int bottom=r.bottom-100;

Rectangle(hdc,left,top,right,bottom);

int center=(top+bottom)/2;
int height=bottom-top;
int width=right-left;

COLORREF colors[4]={
RGB(0,100,200),
RGB(0,150,0),
RGB(200,100,0),
RGB(200,0,0)
};

DWORD now=GetTickCount();
DWORD windowStart=now-g_viewWindowMs;

for(int ch=0;ch<NUM_CHANNELS;ch++){

HPEN pen=CreatePen(PS_SOLID,1,colors[ch]);
SelectObject(hdc,pen);

bool first=true;

for(int i=0;i<BUFFER;i++){

int idx=(g_idx+i)%BUFFER;

if(g_buffer[idx].timestamp<windowStart)continue;

long long tDiff=(long long)g_buffer[idx].timestamp-windowStart;

int x=left+(int)(tDiff*width/g_viewWindowMs);

float v=g_buffer[idx].values[ch];

int y=center-(int)((v/g_yRangeVolts)*(height/2));

if(first){
MoveToEx(hdc,x,y,NULL);
first=false;
}
else{
LineTo(hdc,x,y);
}

}

DeleteObject(pen);

}

EndPaint(hwnd,&ps);

return 0;
}
case WM_DESTROY:
{
PostQuitMessage(0);
return 0;
}
}

return DefWindowProc(hwnd,msg,wParam,lParam);
}   




