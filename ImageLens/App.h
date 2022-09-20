#pragma once

#include "pch.h"

namespace ThisApp
{
    template <typename T>
    void SafeRelease(T** ppT)
    {
        if (*ppT)
        {
            (*ppT)->Release();
            *ppT = nullptr;
        }
    }

    class App {
    private:
        HINSTANCE m_hInstance;
        HWND m_hWndMain;

        // Child windows
        const int m_id_wndImage = 100;
        HWND m_hWndImage;

        const int m_id_wndData = 101;
        HWND m_hWndData;

        // Bookkeeping
        std::vector<std::wstring> m_filepaths;
        int m_maxSize = 200;
        int m_imgIndex = -1;
        std::wstring m_currentImg;
        std::wstring m_currentDir;

        // WIC
        CComPtr<IWICImagingFactory> m_wicFactory;
        CComPtr<IWICFormatConverter> m_formatConverter;

        // Direct2D
        CComPtr<ID2D1Factory> m_d2d1Factory;
        CComPtr<ID2D1Bitmap> m_bitmap;

        D2D1_RECT_F m_dstRect;
        D2D1_RECT_F m_drawSrcRect;
        // Brushes
        CComPtr<ID2D1SolidColorBrush> m_whitebrush;
        CComPtr<ID2D1SolidColorBrush> m_lavenderbrush;

        // Render targets
        CComPtr<ID2D1HwndRenderTarget> m_hwndRTMain;
        CComPtr<ID2D1HwndRenderTarget> m_hwndRTImage;
        CComPtr<ID2D1HwndRenderTarget> m_hwndRTData;

        // Render target properties
        D2D1_RENDER_TARGET_PROPERTIES m_generalRTProps;
        D2D1_HWND_RENDER_TARGET_PROPERTIES m_hwndRTPropsMain;
        // = = = = =
        D2D1_HWND_RENDER_TARGET_PROPERTIES m_hwndRTPropsImage;
        D2D1_HWND_RENDER_TARGET_PROPERTIES m_hwndRTPropsData;

        // DirectWrite
        CComPtr<IDWriteFactory> m_dwriteFactory;
        CComPtr<IDWriteTextFormat> m_dwriteTextFormat;

        int CreateWicBitmap(const std::wstring& filepath);
        bool IsFileSupported(wchar_t* filepath) const;

        int CreateDDRsExceptD2DBitmap();
        void DiscardDDRs();

        void UpdateImageScrolls();

        void UpdateRectangles();
        void UpdateRectanglesOnResize();

        void UpdateImageData();
    public:
        App(HINSTANCE hInstance);

        int Initialize(HINSTANCE hInstance);
        void Cleanup();

        HWND GetMainWindow();

        static long long CALLBACK s_WndProc(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam);
        long long CALLBACK WndProc(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam);

        // m_id_wndImage window procedures
        static long long CALLBACK s_WndProcImage(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam);
        long long CALLBACK WndProcImage(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam);

        // m_id_wndData window procedures
        static long long CALLBACK s_WndProcData(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam);
        long long CALLBACK WndProcData(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam);
    };
}