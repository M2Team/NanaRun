/*
 * PROJECT:   NanaRun
 * FILE:      SynthRdp.cpp
 * PURPOSE:   Implementation for Hyper-V Enhanced Session Proxy Service
 *
 * LICENSE:   The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#include <Mile.Helpers.CppBase.h>

#include <Mile.Project.Version.h>

DWORD SynthRdpMain()
{
    return ERROR_SUCCESS;
}

namespace
{
    std::wstring GetCurrentProcessModulePath()
    {
        // 32767 is the maximum path length without the terminating null
        // character.
        std::wstring Path(32767, L'\0');
        Path.resize(::GetModuleFileNameW(
            nullptr, &Path[0], static_cast<DWORD>(Path.size())));
        return Path;
    }

    static std::wstring g_ServiceName =
        L"SynthRdp";
    static std::wstring g_DisplayName =
        L"Hyper-V Enhanced Session Proxy Service";

    static SERVICE_STATUS_HANDLE volatile g_ServiceStatusHandle = nullptr;
    static bool volatile g_ServiceIsRunning = true;
}

void WINAPI SynthRdpServiceHandler(
    _In_ DWORD dwControl)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
    {
        SERVICE_STATUS ServiceStatus = { 0 };
        ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        ServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
        ServiceStatus.dwServiceSpecificExitCode = 0;
        ServiceStatus.dwCheckPoint = 0;
        ServiceStatus.dwWaitHint = 0;
        ::SetServiceStatus(g_ServiceStatusHandle, &ServiceStatus);

        g_ServiceIsRunning = false;

        break;
    }
    default:
        break;
    }
}

void WINAPI SynthRdpServiceMain(
    _In_ DWORD dwNumServicesArgs,
    _In_ LPWSTR* lpServiceArgVectors)
{
    UNREFERENCED_PARAMETER(dwNumServicesArgs);
    UNREFERENCED_PARAMETER(lpServiceArgVectors);

    g_ServiceStatusHandle = ::RegisterServiceCtrlHandlerW(
        g_ServiceName.c_str(),
        ::SynthRdpServiceHandler);
    if (g_ServiceStatusHandle)
    {
        SERVICE_STATUS ServiceStatus = { 0 };
        ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        ServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
        ServiceStatus.dwServiceSpecificExitCode = 0;
        ServiceStatus.dwCheckPoint = 0;
        ServiceStatus.dwWaitHint = 0;
        if (::SetServiceStatus(g_ServiceStatusHandle, &ServiceStatus))
        {
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceStatus.dwControlsAccepted = 0;
            ServiceStatus.dwWin32ExitCode = ::SynthRdpMain();
            ::SetServiceStatus(g_ServiceStatusHandle, &ServiceStatus);
        }
    }
}

int SynthRdpInstallService()
{
    DWORD Error = ERROR_SUCCESS;

    std::wstring ServiceBinaryPath =
        ::GetCurrentProcessModulePath() + L" Service";

    SC_HANDLE ServiceControlManagerHandle = ::OpenSCManagerW(
        nullptr,
        nullptr,
        SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (ServiceControlManagerHandle)
    {
        SC_HANDLE ServiceHandle = ::CreateServiceW(
            ServiceControlManagerHandle,
            g_ServiceName.c_str(),
            g_DisplayName.c_str(),
            SERVICE_QUERY_STATUS | SERVICE_START,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            ServiceBinaryPath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (ServiceHandle)
        {
            SERVICE_STATUS_PROCESS ServiceStatus = { 0 };
            if (!::MileStartServiceByHandle(
                ServiceHandle,
                0,
                nullptr,
                &ServiceStatus))
            {
                Error = ::GetLastError();
            }

            ::CloseServiceHandle(ServiceHandle);
        }
        else
        {
            Error = ::GetLastError();
        }

        ::CloseServiceHandle(ServiceControlManagerHandle);
    }
    else
    {
        Error = ::GetLastError();
    }

    if (ERROR_SUCCESS == Error)
    {
        std::printf("[Success] SynthRdpInstallService\r\n");
    }
    else
    {
        std::printf("[Error] SynthRdpInstallService (%d)\r\n", Error);
    }

    return Error;
}

int SynthRdpUninstallService()
{
    DWORD Error = ERROR_SUCCESS;

    SC_HANDLE ServiceControlManagerHandle = ::OpenSCManagerW(
        nullptr,
        nullptr,
        SC_MANAGER_CONNECT);
    if (ServiceControlManagerHandle)
    {
        SC_HANDLE ServiceHandle = ::OpenServiceW(
            ServiceControlManagerHandle,
            g_ServiceName.c_str(),
            SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);
        if (ServiceHandle)
        {
            SERVICE_STATUS_PROCESS ServiceStatus = { 0 };
            if (::MileStopServiceByHandle(ServiceHandle, &ServiceStatus))
            {
                if (!::DeleteService(ServiceHandle))
                {
                    Error = ::GetLastError();
                }
            }
            else
            {
                Error = ::GetLastError();
            }

            ::CloseServiceHandle(ServiceHandle);
        }
        else
        {
            Error = ::GetLastError();
        }

        ::CloseServiceHandle(ServiceControlManagerHandle);
    }
    else
    {
        Error = ::GetLastError();
    }

    if (ERROR_SUCCESS == Error)
    {
        std::printf("[Success] SynthRdpUninstallService\r\n");
    }
    else
    {
        std::printf("[Error] SynthRdpUninstallService (%d)\r\n", Error);
    }

    return Error;
}

int SynthRdpStartService()
{
    DWORD Error = ERROR_SUCCESS;

    SERVICE_STATUS_PROCESS ServiceStatus = { 0 };
    if (!::MileStartService(g_ServiceName.c_str(), &ServiceStatus))
    {
        Error = ::GetLastError();
    }

    if (ERROR_SUCCESS == Error)
    {
        std::printf("[Success] SynthRdpStartService\r\n");
    }
    else
    {
        std::printf("[Error] SynthRdpStartService (%d)\r\n", Error);
    }

    return Error;
}

int SynthRdpStopService()
{
    DWORD Error = ERROR_SUCCESS;

    SERVICE_STATUS_PROCESS ServiceStatus = { 0 };
    if (!::MileStopService(g_ServiceName.c_str(), &ServiceStatus))
    {
        Error = ::GetLastError();
    }

    if (ERROR_SUCCESS == Error)
    {
        std::printf("[Success] SynthRdpStopService\r\n");
    }
    else
    {
        std::printf("[Error] SynthRdpStopService (%d)\r\n", Error);
    }

    return Error;
}

int main()
{
    ::std::printf(
        "SynthRdp " MILE_PROJECT_VERSION_UTF8_STRING " (Build "
        MILE_PROJECT_MACRO_TO_UTF8_STRING(MILE_PROJECT_VERSION_BUILD) ")" "\r\n"
        "(c) M2-Team and Contributors. All rights reserved.\r\n"
        "\r\n");

    std::vector<std::string> Arguments = Mile::SplitCommandLineString(
        Mile::ToString(CP_UTF8, ::GetCommandLineW()));

    bool NeedParse = (Arguments.size() > 1);

    if (!NeedParse)
    {
        return ::SynthRdpMain();
    }

    int Result = 0;

    bool ParseError = false;
    bool ShowHelp = false;

    if (0 == _stricmp(Arguments[1].c_str(), "Help"))
    {
        ShowHelp = true;
    }
    else if (0 == _stricmp(Arguments[1].c_str(), "Service"))
    {
        ::FreeConsole();

        SERVICE_TABLE_ENTRYW ServiceStartTable[] =
        {
            {
                const_cast<wchar_t*>(g_ServiceName.c_str()),
                ::SynthRdpServiceMain
            }
        };

        return ::StartServiceCtrlDispatcherW(ServiceStartTable)
            ? ERROR_SUCCESS :
            ::GetLastError();
    }
    else if (0 == _stricmp(Arguments[1].c_str(), "Install"))
    {
        Result = ::SynthRdpInstallService();
    }
    else if (0 == _stricmp(Arguments[1].c_str(), "Uninstall"))
    {
        Result = ::SynthRdpUninstallService();
    }
    else if (0 == _stricmp(Arguments[1].c_str(), "Start"))
    {
        Result = ::SynthRdpStartService();
    }
    else if (0 == _stricmp(Arguments[1].c_str(), "Stop"))
    {
        Result = ::SynthRdpStopService();
    }
    else
    {
        ParseError = true;
    }

    if (ParseError)
    {
        std::printf(
            "[Error] Unrecognized command.\r\n"
            "\r\n");
    }

    if (ParseError || ShowHelp)
    {
        std::printf(
            "Format: SynthRdp [Command] <Option>\r\n"
            "\r\n"
            "Commands:\r\n"
            "\r\n"
            "  Help - Show this content.\r\n"
            "\r\n"
            "  Install - Install SynthRdp service\r\n"
            "  Uninstall - Uninstall SynthRdp service\r\n"
            "  Start - Start SynthRdp service\r\n"
            "  Stop - Stop SynthRdp service\r\n"
            "\r\n"
            "Notes:\r\n"
            "  - All command options are case-insensitive.\r\n"
            "  - SynthRdp will run as a console application instead of service "
            "if you don't\r\n"
            "    specify another command.\r\n"
            "\r\n"
            "Examples:\r\n"
            "\r\n"
            "  SynthRdp Install\r\n"
            "  SynthRdp Uninstall\r\n"
            "  SynthRdp Start\r\n"
            "  SynthRdp Stop\r\n"
            "\r\n");
    }

    return Result;
}
