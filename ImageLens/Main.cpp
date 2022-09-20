#include "pch.h"
#include "Resource.h"

#include "App.h"

//int main(int argc, char* argv[], char* envp[])
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* pCmdLine, int nCmdShow)
{
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
    {
        return -1;
    }

    {
        ThisApp::App app(hInstance);
        if (FAILED(app.Initialize(hInstance)))
        {
            MessageBoxEx(nullptr, L"App initialization failed", L"Error information", MB_ICONERROR | MB_OK, 0);
            return -2;
        }

        ShowWindow(app.GetMainWindow(), nCmdShow);

        MSG message {};
        while (GetMessage(&message, nullptr, 0, 0))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        app.Cleanup();
    }

    CoUninitialize();
    return 0;
}