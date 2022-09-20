#include "pch.h"
#include "App.h"
#include "Resource.h"

namespace ThisApp
{
    App::App(HINSTANCE hInstance)
        : m_hInstance(hInstance),
        m_hWndMain(nullptr),
        m_hWndImage(nullptr),
        m_hWndData(nullptr)
    {
        m_filepaths.reserve(m_maxSize);
    }

    HWND App::GetMainWindow()
    {
        return m_hWndMain;
    }

    int App::Initialize(HINSTANCE hInstance)
    {
        // Register the main window class and create the window

        WNDCLASSEX wc;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = s_WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        //wc.hIcon = static_cast<HICON>(LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED));
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMAGELENS));
        //wc.hCursor = static_cast<HCURSOR>(LoadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED));
        wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
        wc.lpszClassName = L"m_hWndMain";
        wc.hbrBackground = nullptr;
        wc.lpszMenuName = MAKEINTRESOURCE(IDC_IMAGELENS);
        wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

        if (!RegisterClassEx(&wc))
        {
            MessageBoxEx(nullptr, L"RegisterClassEx failed with m_hWndMain", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        unsigned int dpi = GetDpiForWindow(GetDesktopWindow());
        RECT windowDimensions;
        windowDimensions.left = 0;
        windowDimensions.top = 0;
        windowDimensions.right = static_cast<int>(std::ceil(800.0f * (dpi / 96.0f)));
        windowDimensions.bottom = static_cast<int>(std::ceil(600.0f * (dpi / 96.0f)));
        if (!AdjustWindowRectExForDpi(&windowDimensions, WS_OVERLAPPEDWINDOW, true, 0u, dpi))
        {
            MessageBoxEx(nullptr, L"AdjustWindowRectExForDpi failed", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        // 816, 659
        m_hWndMain = CreateWindowEx(0, L"m_hWndMain", L"ImageLens", WS_OVERLAPPEDWINDOW, 20, 20, windowDimensions.right - windowDimensions.left, windowDimensions.bottom - windowDimensions.top, nullptr, nullptr, hInstance, this);
        if (m_hWndMain == nullptr)
        {
            MessageBoxEx(nullptr, L"CreateWindowEx failed with m_hWndMain", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        // Initialize the WIC factory

        int hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_wicFactory));
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"CoCreateInstance failed with CLSID_WICImagingFactory", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        // Create the D2D1Factory
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2d1Factory);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"D2D1CreateFactory failed in App::Initialize", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        // Create the HwndRenderTarget for m_hWndMain

        m_generalRTProps.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        m_generalRTProps.pixelFormat = D2D1::PixelFormat();
        m_generalRTProps.dpiX = 0.0f;
        m_generalRTProps.dpiY = 0.0f;
        m_generalRTProps.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        m_generalRTProps.minLevel = D2D1_FEATURE_LEVEL_10;

        m_hwndRTPropsMain.hwnd = m_hWndMain;
        RECT rc;
        if (!GetClientRect(m_hWndMain, &rc))
        {
            MessageBoxEx(nullptr, L"GetClientRect failed with m_hWndMain in App::Initialize", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }
        m_hwndRTPropsMain.pixelSize = D2D1::SizeU(rc.right, rc.bottom);
        m_hwndRTPropsMain.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

        hr = m_d2d1Factory->CreateHwndRenderTarget(m_generalRTProps, m_hwndRTPropsMain, &m_hwndRTMain);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"CreateHwndRenderTarget failed (m_hwndRTMain) in App::Initialize", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        // Register the class for m_hWndImage and create the window

        WNDCLASSEX wcWndImage;
        wcWndImage.cbSize = sizeof(WNDCLASSEX);
        wcWndImage.style = CS_HREDRAW | CS_VREDRAW;
        wcWndImage.lpfnWndProc = s_WndProcImage;
        wcWndImage.cbClsExtra = 0;
        wcWndImage.cbWndExtra = 0;
        wcWndImage.hInstance = hInstance;
        wcWndImage.hIcon = nullptr;
        wcWndImage.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_ARROW));
        wcWndImage.lpszClassName = L"m_hWndImage";
        wcWndImage.hbrBackground = nullptr;
        wcWndImage.lpszMenuName = nullptr;
        wcWndImage.hIconSm = nullptr;

        if (!RegisterClassEx(&wcWndImage))
        {
            MessageBoxEx(nullptr, L"RegisterClassEx failed with m_hWndImage", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        m_hWndImage = CreateWindowEx(0, L"m_hWndImage", nullptr, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, 0, 0, rc.right, rc.bottom * 0.70, m_hWndMain, reinterpret_cast<HMENU>(m_id_wndImage), m_hInstance, this);
        if (m_hWndImage == nullptr)
        {
            MessageBoxEx(nullptr, L"CreateWindowEx failed with m_hWndImage", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        m_hwndRTPropsImage.hwnd = m_hWndImage;
        m_hwndRTPropsImage.pixelSize = D2D1::SizeU(rc.right - GetSystemMetrics(SM_CXVSCROLL), (rc.bottom * 0.70) - GetSystemMetrics(SM_CYHSCROLL) - 3);
        m_hwndRTPropsImage.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
        hr = m_d2d1Factory->CreateHwndRenderTarget(m_generalRTProps, m_hwndRTPropsImage, &m_hwndRTImage);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"CreateHwndRenderTarget failed (m_hwndRTImage) in App::Initialize", L"Error", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        hr = m_hwndRTImage->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whitebrush);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"CreateSolidColorBrush failed in App::Initialize", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        m_dstRect.left = 0.0f;
        m_dstRect.top = 0.0f;
        m_dstRect.right = 0.0f;
        m_dstRect.bottom = 0.0f;

        m_drawSrcRect.left = 0.0f;
        m_drawSrcRect.top = 0.0f;
        m_drawSrcRect.right = 0.0f;
        m_drawSrcRect.bottom = 0.0f;

        // Set SCROLLINFO for m_hWndImage

        SCROLLINFO WndImageSiHor;
        WndImageSiHor.cbSize = sizeof(SCROLLINFO);
        WndImageSiHor.fMask = SIF_ALL;
        WndImageSiHor.nMin = 0;
        WndImageSiHor.nMax = 0;
        WndImageSiHor.nPage = rc.right - GetSystemMetrics(SM_CXVSCROLL);
        WndImageSiHor.nPos = 0;
        SetScrollInfo(m_hWndImage, SB_HORZ, &WndImageSiHor, false);

        SCROLLINFO WndImageSiVer;
        WndImageSiVer.cbSize = sizeof(SCROLLINFO);
        WndImageSiVer.fMask = SIF_ALL;
        WndImageSiVer.nMin = 0;
        WndImageSiVer.nMax = 0;
        WndImageSiVer.nPage = (rc.bottom * 0.70) - GetSystemMetrics(SM_CYHSCROLL) - 3;
        WndImageSiVer.nPos = 0;
        SetScrollInfo(m_hWndImage, SB_VERT, &WndImageSiVer, false);

        // Register the class for m_hWndData and create the window

        WNDCLASSEX wcWndData;
        wcWndData.cbSize = sizeof(WNDCLASSEX);
        wcWndData.style = CS_HREDRAW | CS_VREDRAW;
        wcWndData.lpfnWndProc = s_WndProcData;
        wcWndData.cbClsExtra = 0;
        wcWndData.cbWndExtra = 0;
        wcWndData.hInstance = hInstance;
        wcWndData.hIcon = nullptr;
        wcWndData.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_ARROW));
        wcWndData.lpszClassName = L"m_hWndData";
        wcWndData.hbrBackground = nullptr;
        wcWndData.lpszMenuName = nullptr;
        wcWndData.hIconSm = nullptr;

        if (!RegisterClassEx(&wcWndData))
        {
            MessageBoxEx(nullptr, L"RegisterClassEx failed (m_hWndData)", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        m_hWndData = CreateWindowEx(0, L"m_hWndData", nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, (rc.bottom * 0.70) + 1, rc.right, (rc.bottom * 0.30) - 1, m_hWndMain, reinterpret_cast<HMENU>(m_id_wndData), m_hInstance, this);
        if (m_hWndImage == nullptr)
        {
            MessageBoxEx(nullptr, L"CreateWindowEx failed (m_hWndData)", L"Error information", MB_ICONERROR | MB_OK, 0);
            return E_FAIL;
        }

        m_hwndRTPropsData.hwnd = m_hWndData;
        m_hwndRTPropsData.pixelSize = D2D1::SizeU(rc.right, (rc.bottom * 0.30) - 1);
        m_hwndRTPropsData.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
        hr = m_d2d1Factory->CreateHwndRenderTarget(m_generalRTProps, m_hwndRTPropsData, &m_hwndRTData);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"CreateHwndRenderTarget failed (m_hwndRTData) in App::Initialize", L"Error", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        hr = m_hwndRTData->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Lavender), &m_lavenderbrush);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"m_lavenderbrush creation failed", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        // Set SCROLLINFO for m_hWndData
        SCROLLINFO WndDataSiVer;
        WndDataSiVer.cbSize = sizeof(SCROLLINFO);
        WndDataSiVer.fMask = SIF_ALL;
        WndDataSiVer.nMin = 0;
        WndDataSiVer.nMax = 0;
        WndDataSiVer.nPage = (rc.bottom * 0.30) - 1;
        WndDataSiVer.nPos = 0;
        SetScrollInfo(m_hWndData, SB_VERT, &WndDataSiVer, false);

        // Create DirectWrite resources

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_dwriteFactory.p), reinterpret_cast<IUnknown**>(&m_dwriteFactory));
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"DWriteCreateFactory failed", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        hr = m_dwriteFactory->CreateTextFormat(L"Verdana", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"", &m_dwriteTextFormat);
        if (FAILED(hr))
        {
            MessageBoxEx(nullptr, L"m_dwriteFactory->CreateTextFormat failed", L"Error information", MB_ICONERROR | MB_OK, 0);
            return hr;
        }

        return hr;
    }

    long long CALLBACK App::s_WndProc(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam)
    {
        App* app = nullptr;

        if (msg == WM_CREATE)
        {
            CREATESTRUCT* creationInfo = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<App*>(creationInfo->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<long long>(app));

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        else
        {
            app = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            if (app)
            {
                return app->WndProc(hWnd, msg, wParam, lParam);
            }
            else
            {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }
    }

    long long CALLBACK App::WndProc(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam)
    {
        switch (msg)
        {
            case WM_SIZE:
            {
                if (m_hwndRTMain)
                {
                    unsigned int width = LOWORD(lParam);
                    unsigned int height = HIWORD(lParam);
                    m_hwndRTMain->Resize(D2D1::SizeU(width, height));

                    if (!MoveWindow(m_hWndImage, 0, 0, width, height * 0.70, false))
                    {
                        MessageBoxEx(m_hWndMain, L"MoveWindow failed (m_hWndImage)", L"Error information", MB_ICONERROR | MB_OK, 0);
                        return 0;
                    }
                    m_hwndRTImage->Resize(D2D1::SizeU(width - GetSystemMetrics(SM_CXVSCROLL), (height * 0.70) - GetSystemMetrics(SM_CYHSCROLL) - 3));

                    if (!MoveWindow(m_hWndData, 0, (height * 0.70) + 1, width, (height * 0.30) - 1, false))
                    {
                        MessageBoxEx(m_hWndMain, L"MoveWindow failed (m_hWndData)", L"Error information", MB_ICONERROR | MB_OK, 0);
                        return 0;
                    }
                    m_hwndRTData->Resize(D2D1::SizeU(width, (height * 0.30) - 1));

                    // For scrolling
                    if (m_bitmap)
                    {
                        // For scrolling of x-axis in m_hWndImage

                        SCROLLINFO WndImageSiHor;
                        WndImageSiHor.cbSize = sizeof(SCROLLINFO);
                        WndImageSiHor.fMask = SIF_ALL;
                        GetScrollInfo(m_hWndImage, SB_HORZ, &WndImageSiHor);

                        WndImageSiHor.nPage = m_hwndRTImage->GetPixelSize().width;

                        int horThumbPosMax = WndImageSiHor.nMax - (WndImageSiHor.nPage - 1);
                        WndImageSiHor.nPos = std::min(horThumbPosMax, WndImageSiHor.nPos);

                        SetScrollInfo(m_hWndImage, SB_HORZ, &WndImageSiHor, false);

                        // For scrolling of y-axis in m_hWndImage

                        SCROLLINFO WndImageSiVer;
                        WndImageSiVer.cbSize = sizeof(SCROLLINFO);
                        WndImageSiVer.fMask = SIF_ALL;
                        GetScrollInfo(m_hWndImage, SB_VERT, &WndImageSiVer);

                        WndImageSiVer.nPage = m_hwndRTImage->GetPixelSize().height;

                        int verThumbPosMax = WndImageSiVer.nMax - (WndImageSiVer.nPage - 1);
                        WndImageSiVer.nPos = std::min(verThumbPosMax, WndImageSiVer.nPos);

                        SetScrollInfo(m_hWndImage, SB_VERT, &WndImageSiVer, false);

                        // For scrolling of y-axis in m_hWndData

                        UpdateRectanglesOnResize();
                    }

                    InvalidateRect(hWnd, nullptr, false);
                }

                return 0;
            }

            case WM_ERASEBKGND:
            {
                return 1;
            }

            case WM_PAINT:
            {
                int hr = CreateDDRsExceptD2DBitmap();
                if (FAILED(hr))
                {
                    return 0;
                }

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                m_hwndRTMain->BeginDraw();

                m_hwndRTMain->SetTransform(D2D1::IdentityMatrix());
                m_hwndRTMain->Clear(D2D1::ColorF(D2D1::ColorF::Black));

                hr = m_hwndRTMain->EndDraw();
                if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
                {
                    DiscardDDRs();
                }

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDM_OPEN:
                    {
                        CComPtr<IFileOpenDialog> fopenDlg;
                        int hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fopenDlg));
                        if (FAILED(hr))
                        {
                            MessageBoxEx(hWnd, L"CoCreateInstance failed with CLSID_FileOpenDialog", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        // HD Photo Format not supported (.wdp)
                        // Digital Negative not supported (.dng)
                        // JPEG XR not supported (.jxr, .wdp)
                        // TODO: Add support for SVG, APNG, WebP, AVIF, TGA
                        // TODO: Maybe add support for DirectDraw Surface? (.dds)
                        const COMDLG_FILTERSPEC filters[] =
                        {
                            { L"PNG files", L"*.png" },
                            { L"BMP files", L"*.bmp;*.dib" },
                            { L"JPEG files", L"*.jpg;*.jpeg;*.jfif;*.pjpeg;*.pjp;*.jpe" },
                            { L"TIFF files", L"*.tiff;*.tif" },
                            { L"ICO files", L"*.ico;*.cur" },
                            { L"GIF files", L"*.gif" }
                        };

                        hr = fopenDlg->SetFileTypes(6u, filters);
                        if (FAILED(hr))
                        {
                            MessageBoxEx(hWnd, L"fOpenDlg->SetFileTypes failed in WM_COMMAND->IDM_OPEN", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        hr = fopenDlg->Show(m_hWndMain);
                        if (FAILED(hr))
                        {
                            //MessageBoxEx(hWnd, L"fopenDlg->Show failed in WM_COMMAND->IDM_OPEN", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        CComPtr<IShellItem> shItem;
                        hr = fopenDlg->GetResult(&shItem);
                        if (FAILED(hr))
                        {
                            MessageBoxEx(hWnd, L"fopenDlg->GetResult failed", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        wchar_t* temp_filepath = nullptr;
                        hr = shItem->GetDisplayName(SIGDN_FILESYSPATH, &temp_filepath);
                        if (FAILED(hr))
                        {
                            MessageBoxEx(hWnd, L"shItem->GetDisplayName failed", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        std::wstring filepath(temp_filepath);
                        CoTaskMemFree(temp_filepath);
                        temp_filepath = nullptr;

                        std::wstring currentDir = filepath.substr(0, filepath.find_last_of(L'\\'));

                        // if the user selected an image that is in the same directory...
                        if (currentDir.compare(m_currentDir) == 0)
                        {
                            // if the user selected the image currently displayed...
                            if (m_currentImg.compare(filepath) == 0)
                            {
                                return 0;
                            }

                            // update m_formatConverter
                            hr = CreateWicBitmap(filepath);
                            if (FAILED(hr))
                            {
                                MessageBoxEx(hWnd, L"CreateBitmaps failed", L"Error information", MB_ICONERROR | MB_OK, 0);
                                return 0;
                            }
                            m_bitmap.Release();

                            // Bookkeeping only if everything went well
                            m_currentImg = filepath;

                            for (int i = 0; i < m_filepaths.size(); ++i)
                            {
                                if (m_filepaths[i].compare(filepath) == 0)
                                {
                                    m_imgIndex = i;
                                    break;
                                }
                            }

                            UpdateImageScrolls();
                            UpdateRectangles();

                            InvalidateRect(m_hWndImage, nullptr, false);
                            return 0;
                        }

                        // get the paths of the images (of supported format) inside the directory
                        HANDLE hFile = nullptr;
                        WIN32_FIND_DATA ffd;
                        hFile = FindFirstFileEx(std::wstring(currentDir).append(L"\\*.*").data(), FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr, 0);
                        if (hFile == INVALID_HANDLE_VALUE)
                        {
                            MessageBoxEx(hWnd, L"FindFirstFileEx returned INVALID_HANDLE_VALUE", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        m_filepaths.clear();
                        m_imgIndex = -1;
                        int emplaced_elems = 0;
                        do
                        {
                            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            {
                                // do nothing
                            }
                            else
                            {
                                if (IsFileSupported(ffd.cFileName))
                                {
                                    m_filepaths.push_back(std::wstring(currentDir).append(1, L'\\').append(ffd.cFileName));
                                    ++emplaced_elems;

                                    if (emplaced_elems > m_maxSize - 4)
                                    {
                                        m_maxSize += 100;
                                        m_filepaths.reserve(m_maxSize);
                                    }
                                }
                            }

                        } while (FindNextFile(hFile, &ffd) != 0);
                        if (GetLastError() != ERROR_NO_MORE_FILES)
                        {
                            m_filepaths.clear();
                            MessageBoxEx(hWnd, L"FindNextFile failed", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }

                        // update m_formatConverter
                        hr = CreateWicBitmap(filepath);
                        if (FAILED(hr))
                        {
                            MessageBoxEx(hWnd, L"CreateBitmaps failed 2", L"Error information", MB_ICONERROR | MB_OK, 0);
                            return 0;
                        }
                        m_bitmap.Release();

                        // Bookkeeping must happen only if everything went well
                        m_currentImg = filepath;
                        m_currentDir = currentDir;
                        for (int i = 0; i < m_filepaths.size(); ++i)
                        {
                            if (m_filepaths[i].compare(filepath) == 0)
                            {
                                m_imgIndex = i;
                                break;
                            }
                        }

                        UpdateImageScrolls();
                        UpdateRectangles();

                        InvalidateRect(m_hWndImage, nullptr, false);
                        return 0;
                    }
                }

                return 0;
            }

            case WM_KEYDOWN:
            {
                switch (wParam)
                {
                    case VK_LEFT:
                    {
                        if (m_imgIndex != -1)
                        {
                            m_imgIndex = m_imgIndex == 0 ? m_filepaths.size() - 1 : --m_imgIndex;
                            m_currentImg = m_filepaths[m_imgIndex];
                            if (FAILED(CreateWicBitmap(m_currentImg)))
                            {
                                MessageBoxEx(hWnd, L"CreateWicBitmap failed in VK_LEFT", L"Error information", MB_ICONERROR | MB_OK, 0);
                            }

                            m_bitmap.Release();
                            UpdateImageScrolls();
                            UpdateRectangles();
                            InvalidateRect(m_hWndImage, nullptr, false);
                        }

                        return 0;
                    }

                    case VK_RIGHT:
                    {
                        if (m_imgIndex != -1)
                        {
                            m_imgIndex = m_imgIndex == m_filepaths.size() - 1 ? 0 : ++m_imgIndex;
                            m_currentImg = m_filepaths[m_imgIndex];
                            if (FAILED(CreateWicBitmap(m_currentImg)))
                            {
                                MessageBoxEx(hWnd, L"CreateWicBitmap failed in VK_LEFT", L"Error information", MB_ICONERROR | MB_OK, 0);
                            }

                            m_bitmap.Release();
                            UpdateImageScrolls();
                            UpdateRectangles();
                            InvalidateRect(m_hWndImage, nullptr, false);
                        }

                        return 0;
                    }
                }

                return 0;
            }

            case WM_CLOSE:
            {
                DestroyWindow(hWnd);
                return 0;
            }

            case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            }

            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }

    long long CALLBACK App::s_WndProcImage(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam)
    {
        App* app = nullptr;

        if (msg == WM_CREATE)
        {
            CREATESTRUCT* creationInfo = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<App*>(creationInfo->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<long long>(app));

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        else
        {
            app = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            if (app)
            {
                return app->WndProcImage(hWnd, msg, wParam, lParam);
            }
            else
            {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }
    }

    long long CALLBACK App::WndProcImage(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam)
    {
        switch (msg)
        {
            case WM_SIZE:
            {
                return 0;
            }

            case WM_ERASEBKGND:
            {
                return 1;
            }

            case WM_PAINT:
            {
                int hr = CreateDDRsExceptD2DBitmap();
                if (FAILED(hr))
                {
                    return 0;
                }

                if (!m_bitmap && m_formatConverter)
                {
                    hr = m_hwndRTImage->CreateBitmapFromWicBitmap(m_formatConverter.p, &m_bitmap);
                    if (FAILED(hr))
                    {
                        return 0;
                    }
                }

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                m_hwndRTImage->BeginDraw();

                m_hwndRTImage->SetTransform(D2D1::IdentityMatrix());
                m_hwndRTImage->Clear(D2D1::ColorF(D2D1::ColorF::SaddleBrown));

                if (m_bitmap)
                {
                    SCROLLINFO hor;
                    hor.cbSize = sizeof(SCROLLINFO);
                    hor.fMask = SIF_POS;
                    GetScrollInfo(m_hWndImage, SB_HORZ, &hor);

                    SCROLLINFO ver;
                    ver.cbSize = sizeof(SCROLLINFO);
                    ver.fMask = SIF_POS;
                    GetScrollInfo(m_hWndImage, SB_VERT, &ver);

                    m_hwndRTImage->DrawBitmap(m_bitmap.p, m_dstRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1::RectF(hor.nPos, ver.nPos, m_drawSrcRect.right + hor.nPos, m_drawSrcRect.bottom + ver.nPos));
                }
                else
                {
                    D2D1_SIZE_F size = m_hwndRTImage->GetSize();
                    //const wchar_t text[] = L"There is no bitmap";
                    m_hwndRTImage->DrawText(L"There is no bitmap", 19, m_dwriteTextFormat.p, D2D1::RectF(0.0f, 0.0f, size.width, size.height), m_whitebrush.p);
                }

                hr = m_hwndRTImage->EndDraw();
                if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
                {
                    DiscardDDRs();
                    //MessageBoxEx(m_hWndMain, L"Drawing failed with m_hWndRTImage", L"Error information", MB_ICONERROR | MB_OK, 0);
                }

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_HSCROLL:
            {
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_ALL;
                GetScrollInfo(m_hWndImage, SB_HORZ, &si);

                switch (LOWORD(wParam))
                {
                    case SB_LINELEFT:
                    {
                        si.nPos -= 30;
                        break;
                    }

                    case SB_LINERIGHT:
                    {
                        si.nPos += 30;
                        break;
                    }

                    case SB_PAGELEFT:
                    {
                        si.nPos -= si.nPage;
                        break;
                    }

                    case SB_PAGERIGHT:
                    {
                        si.nPos += si.nPage;
                        break;
                    }
                    /*
                    case SB_THUMBPOSITION:
                    {
                        si.nPos = HIWORD(wParam);
                        break;
                    }
                    */
                    case SB_THUMBTRACK:
                    {
                        si.nPos = si.nTrackPos;
                        break;
                    }

                    default:
                        break;
                }

                si.fMask = SIF_POS;
                SetScrollInfo(m_hWndImage, SB_HORZ, &si, true);

                InvalidateRect(m_hWndImage, nullptr, false);
                return 0;
            }

            case WM_VSCROLL:
            {
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_ALL;
                GetScrollInfo(m_hWndImage, SB_VERT, &si);

                switch (LOWORD(wParam))
                {
                    case SB_LINEUP:
                    {
                        si.nPos -= 30;
                        break;
                    }

                    case SB_LINEDOWN:
                    {
                        si.nPos += 30;
                        break;
                    }

                    case SB_PAGEUP:
                    {
                        si.nPos -= si.nPage;
                        break;
                    }

                    case SB_PAGEDOWN:
                    {
                        si.nPos += si.nPage;
                        break;
                    }

                    case SB_THUMBTRACK:
                    {
                        si.nPos = si.nTrackPos;
                        break;
                    }

                    default:
                        break;
                }

                si.fMask = SIF_POS;
                SetScrollInfo(m_hWndImage, SB_VERT, &si, true);

                InvalidateRect(m_hWndImage, nullptr, false);
                return 0;
            }

            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }

    long long CALLBACK App::s_WndProcData(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam)
    {
        App* app = nullptr;

        if (msg == WM_CREATE)
        {
            CREATESTRUCT* creationInfo = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<App*>(creationInfo->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<long long>(app));

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        else
        {
            app = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            if (app)
            {
                return app->WndProcData(hWnd, msg, wParam, lParam);
            }
            else
            {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }

    }

    long long CALLBACK App::WndProcData(HWND hWnd, unsigned int msg, unsigned long long wParam, long long lParam)
    {
        switch (msg)
        {
            case WM_SIZE:
            {
                return 0;
            }

            case WM_ERASEBKGND:
            {
                return 1;
            }

            case WM_PAINT:
            {
                int hr = CreateDDRsExceptD2DBitmap();
                if (FAILED(hr))
                {
                    return 0;
                }

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                m_hwndRTData->BeginDraw();

                m_hwndRTData->SetTransform(D2D1::IdentityMatrix());
                m_hwndRTData->Clear(D2D1::ColorF(D2D1::ColorF::Gray));

                D2D1_SIZE_U wndImgPixSz = m_hwndRTImage->GetPixelSize();
                std::wstring txtImgDimensions(L"WndImage: ");
                txtImgDimensions.append(std::to_wstring(wndImgPixSz.width));
                txtImgDimensions.append(L" x ");
                txtImgDimensions.append(std::to_wstring(wndImgPixSz.height));

                D2D1_SIZE_F wndDataDipSz = m_hwndRTData->GetSize();
                m_hwndRTData->DrawText(txtImgDimensions.data(), txtImgDimensions.size() + 1, m_dwriteTextFormat.p, D2D1::RectF(0.0f, 0.0f, wndDataDipSz.width, wndDataDipSz.height), m_lavenderbrush.p);

                D2D1_SIZE_U wndDataPixSz = m_hwndRTData->GetPixelSize();
                std::wstring txtDataDimensions(L"WndData: ");
                txtDataDimensions.append(std::to_wstring(wndDataPixSz.width));
                txtDataDimensions.append(L" x ");
                txtDataDimensions.append(std::to_wstring(wndDataPixSz.height));
                m_hwndRTData->DrawText(txtDataDimensions.data(), txtDataDimensions.size() + 1, m_dwriteTextFormat.p, D2D1::RectF(0.0f, 50.0f, wndDataDipSz.width, wndDataDipSz.height), m_lavenderbrush.p);

                hr = m_hwndRTData->EndDraw();
                if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
                {
                    DiscardDDRs();
                }

                EndPaint(hWnd, &ps);
                return 0;
            }

            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }

    int App::CreateWicBitmap(const std::wstring& filepath)
    {
        CComPtr<IWICBitmapDecoder> decoder;
        int hr = m_wicFactory->CreateDecoderFromFilename(filepath.data(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr))
        {
            return hr;
        }

        CComPtr<IWICBitmapFrameDecode> decodedFrame;
        hr = decoder->GetFrame(0u, &decodedFrame);
        if (FAILED(hr))
        {
            return hr;
        }

        m_formatConverter.Release();
        hr = m_wicFactory->CreateFormatConverter(&m_formatConverter);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = m_formatConverter->Initialize(decodedFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    bool App::IsFileSupported(wchar_t* filepath) const
    {
        if (PathMatchSpecEx(filepath, L"*png;*bmp;*dib;*jpg;*jpeg;*jfif;*pjpeg;*pjp;*jpe;*tiff;*tif;*ico;*cur;*gif", PMSF_MULTIPLE) == S_OK)
        {
            return true;
        }

        return false;
    }

    int App::CreateDDRsExceptD2DBitmap()
    {
        if (!m_hwndRTMain)
        {
            // Create the render targets

            int hr = m_d2d1Factory->CreateHwndRenderTarget(m_generalRTProps, m_hwndRTPropsMain, &m_hwndRTMain);
            if (FAILED(hr))
            {
                return hr;
            }

            hr = m_d2d1Factory->CreateHwndRenderTarget(m_generalRTProps, m_hwndRTPropsImage, &m_hwndRTImage);
            if (FAILED(hr))
            {
                return hr;
            }

            hr = m_d2d1Factory->CreateHwndRenderTarget(m_generalRTProps, m_hwndRTPropsData, &m_hwndRTData);
            if (FAILED(hr))
            {
                return hr;
            }

            // Create the brushes

            hr = m_hwndRTImage->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whitebrush);
            if (FAILED(hr))
            {
                return hr;
            }

            hr = m_hwndRTData->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Lavender), &m_lavenderbrush);
            if (FAILED(hr))
            {
                return hr;
            }

            return hr;
        }

        return S_OK;
    }

    void App::DiscardDDRs()
    {
        m_hwndRTMain.Release();
        m_hwndRTImage.Release();
        m_hwndRTData.Release();

        m_whitebrush.Release();
        m_bitmap.Release();
    }

    void App::Cleanup()
    {
        DiscardDDRs();
    }

    void App::UpdateImageScrolls()
    {
        // For scrolling of x-axis in m_hWndImage

        SCROLLINFO hor;
        hor.cbSize = sizeof(SCROLLINFO);
        hor.fMask = SIF_ALL;
        GetScrollInfo(m_hWndImage, SB_HORZ, &hor);

        unsigned int imgWidth;
        unsigned int imgHeight;
        m_formatConverter->GetSize(&imgWidth, &imgHeight);
        hor.nMin = 0; // For some reason I have to assign 0 again for the scrollbar to work
        hor.nMax = imgWidth;

        D2D1_SIZE_U client = m_hwndRTImage->GetPixelSize();
        hor.nPage = client.width;

        int horThumbPosMax = hor.nMax - (hor.nPage - 1);
        hor.nPos = std::min(horThumbPosMax, hor.nPos);

        SetScrollInfo(m_hWndImage, SB_HORZ, &hor, false);

        // For scrolling of y-axis in m_hWndImage

        SCROLLINFO ver;
        ver.cbSize = sizeof(SCROLLINFO);
        ver.fMask = SIF_ALL;
        GetScrollInfo(m_hWndImage, SB_VERT, &ver);

        ver.nMin = 0; // Just in case
        ver.nMax = imgHeight;
        ver.nPage = client.height;

        int verThumbPosMax = ver.nMax - (ver.nPage - 1);
        ver.nPos = std::min(verThumbPosMax, ver.nPos);

        SetScrollInfo(m_hWndImage, SB_VERT, &ver, false);
    }

    void App::UpdateRectangles()
    {
        unsigned int imgWidth;
        unsigned int imgHeight;
        m_formatConverter->GetSize(&imgWidth, &imgHeight);

        D2D1_SIZE_F window = m_hwndRTImage->GetSize();

        m_drawSrcRect.left = 0.0f;
        m_drawSrcRect.top = 0.0f;

        if (imgWidth > window.width)
        {
            m_dstRect.right = window.width;
            m_drawSrcRect.right = window.width;
        }
        else
        {
            m_dstRect.right = imgWidth;
            m_drawSrcRect.right = imgWidth;
        }

        if (imgHeight > window.height)
        {
            m_dstRect.bottom = window.height;
            m_drawSrcRect.bottom = window.height;
        }
        else
        {
            m_dstRect.bottom = imgHeight;
            m_drawSrcRect.bottom = imgHeight;
        }
    }

    void App::UpdateRectanglesOnResize()
    {
        D2D1_SIZE_F window = m_hwndRTImage->GetSize();
        unsigned int imgWidth;
        unsigned int imgHeight;
        m_formatConverter->GetSize(&imgWidth, &imgHeight);

        if (imgWidth > window.width)
        {
            m_dstRect.right = window.width;
            m_drawSrcRect.right = window.width;
        }
        else
        {
            m_dstRect.right = imgWidth;
            m_drawSrcRect.right = imgWidth;
        }

        if (imgHeight > window.height)
        {
            m_dstRect.bottom = window.height;
            m_drawSrcRect.bottom = window.height;
        }
        else
        {
            m_dstRect.bottom = imgHeight;
            m_drawSrcRect.bottom = imgHeight;
        }

        SCROLLINFO hor;
        hor.cbSize = sizeof(SCROLLINFO);
        hor.fMask = SIF_POS;
        GetScrollInfo(m_hWndImage, SB_HORZ, &hor);

        SCROLLINFO ver;
        ver.cbSize = sizeof(SCROLLINFO);
        ver.fMask = SIF_POS;
        GetScrollInfo(m_hWndImage, SB_VERT, &ver);

        m_drawSrcRect.left = hor.nPos;
        m_drawSrcRect.top = ver.nPos;
    }

    void App::UpdateImageData()
    {
        // To be implemented
    }
}