/*
 * PROJECT:   NanaRun
 * FILE:      NanaRunWindows.cpp
 * PURPOSE:   Implementation for NanaRun (Windows)
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: MouriNaruto (KurikoMouri@outlook.jp)
 */

#include <Windows.h>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    ::MessageBoxW(
        nullptr,
        L"Hello World!",
        L"NanaRun (Windows)",
        0);

    return 0;
}
