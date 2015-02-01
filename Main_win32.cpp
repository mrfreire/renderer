#include "DebugTimer.h"
#include "Log.h"
#include "Rasterizer.h"

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

struct AppState  // zero is initialisation
{
    BITMAPINFO m_bitmapInfo;
    RasterBuffers m_buffers;
    bool m_quit;
};

int g_bitmapHeight;
int g_bitmapWidth;
int g_bitmapBytes;

extern void Render(RasterBuffers*);

static AppState g_app;

static void ResizeBitmap(int width, int height)
{
    g_bitmapWidth = width;
    g_bitmapHeight = height;

    g_app.m_bitmapInfo.bmiHeader.biSize = sizeof(g_app.m_bitmapInfo.bmiHeader);
    g_app.m_bitmapInfo.bmiHeader.biWidth = width;
    g_app.m_bitmapInfo.bmiHeader.biHeight = height;  // bottom up
    g_app.m_bitmapInfo.bmiHeader.biPlanes = 1;
    g_app.m_bitmapInfo.bmiHeader.biBitCount = 32;
    g_app.m_bitmapInfo.bmiHeader.biCompression = BI_RGB;

    //
    // Destroy buffers
    //

    if (g_app.m_buffers.m_color)
    {
        VirtualFree(g_app.m_buffers.m_color, 0, MEM_RELEASE);
    }

    if (g_app.m_buffers.m_fragmentsTmpBuffer)
    {
        VirtualFree(g_app.m_buffers.m_fragmentsTmpBuffer, 0, MEM_RELEASE);
    }

    //
    // Create new buffers
    //

    g_app.m_buffers.m_bytesPerPixel = 4;
    g_app.m_buffers.m_colorBufferBytes = g_app.m_buffers.m_bytesPerPixel * width * height;
    g_app.m_buffers.m_color = (uint32_t*)VirtualAlloc(
        0, g_app.m_buffers.m_colorBufferBytes, MEM_COMMIT, PAGE_READWRITE);
    g_bitmapBytes = g_app.m_buffers.m_colorBufferBytes;
    
    g_app.m_buffers.m_fragmentsTmpBufferBytes = sizeof(FragmentInput) * width * height;
    g_app.m_buffers.m_fragmentsTmpBuffer = (FragmentInput*)VirtualAlloc(
        0, g_app.m_buffers.m_fragmentsTmpBufferBytes, MEM_COMMIT, PAGE_READWRITE);
    
    //g_app.m_buffers.m_depth = nullptr;
    g_app.m_buffers.m_width = width;
    g_app.m_buffers.m_height = height;

    Render(&g_app.m_buffers);
}

static void DrawBitmap(HDC hdc, RECT clientRect)
{
    StretchDIBits(
        hdc,
        0, 0, g_bitmapWidth, g_bitmapHeight,
        0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
        g_app.m_buffers.m_color,
        &g_app.m_bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

static LRESULT CALLBACK WindowCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            RECT rect;
            GetClientRect(window, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            ResizeBitmap(width, height);
        }
        break;

        case WM_CLOSE:
        {
            g_app.m_quit = true;
        }
        break;

        case WM_DESTROY:
        {
            g_app.m_quit = true;
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            DrawBitmap(deviceContext, paint.rcPaint);
            EndPaint(window, &paint);
        }
        break;

        default:
        {
            result = DefWindowProc(window, message, wparam, lparam);
        }
        break;
    }

    return result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //
    // Create window
    //

    const wchar_t kClassName[] = L"RendererWindowClass";
    const wchar_t kWindowName[] = L"Manuel Freire Renderer 2015";

    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = WindowCallback;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = kClassName;

    if (!RegisterClassW(&windowClass))
    {
        Log::Error("Cannot register window class");
        return 1;
    }

    HWND window = CreateWindowExW(
        0,
        kClassName,
        kWindowName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800 + 16,
        600 + 38,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!window)
    {
        Log::Error("Cannot create window");
        return 1;
    }

    //
    // Core loop
    //

    while (!g_app.m_quit)
    {
        //
        // Parse Windows messages
        //

        MSG message;

        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                g_app.m_quit = true;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        DebugTimer_Tic("Frame");

        //
        // Render
        //

        Render(&g_app.m_buffers);

        //
        // Draw colour buffer
        //

        DebugTimer_Tic("DrawBuffer");

        HDC hdc = GetDC(window);
        RECT rect;
        GetClientRect(window, &rect);
        DrawBitmap(hdc, rect);
        ReleaseDC(window, hdc);

        DebugTimer_TocAndPrint("DrawBuffer");

        //
        // Measure frame time
        //

        double frameTime = DebugTimer_Toc("Frame");
        wchar_t windowName[128];
        swprintf(
            windowName,
            sizeof(windowName),
            L"%s (%dx%d, %.02fms, %.0f FPS)",
            kWindowName, g_bitmapWidth, g_bitmapHeight, frameTime, 1000.0 / frameTime);
        SetWindowText(window, windowName);
    }

    return 0;
}
