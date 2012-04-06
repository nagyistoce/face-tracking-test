#include "master.hpp"
#include "capturer.hpp"
#include "detector.hpp"
#include "render.hpp"

#include <exception>
#include <iostream>

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch( msg )
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, INT)
{
    UNREFERENCED_PARAMETER(hInst);

    Master systems;

    try
    {
        systems.add_subsystem<Capturer>();
        systems.add_subsystem<Detector>();
        systems.add_subsystem<Render>();

        systems.start();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
        PostQuitMessage(-1);
        return -1;
    }

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            systems.subsystem<Render>().render_frame();
        }
    }

    return 0;
}


