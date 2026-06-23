/*
 * PROJECT:    NanaRun
 * FILE:       RunAsSvc.cpp
 * PURPOSE:    Implementation for Run As Windows Service Utility
 *
 * LICENSE:    The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#include <Mile.Helpers.CppBase.h>

#include <Mile.Project.Version.h>

// RunAsSvc Install [ServiceName] [Options]
// RunAsSvc Uninstall [ServiceName]
// RunAsSvc Start [ServiceName]
// RunAsSvc Stop [ServiceName]
// RunAsSvc Enable [ServiceName]
// RunAsSvc Disable [ServiceName]

// Private
// RunAsSvc Instance [ServiceName]

// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\[ServiceName]
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\[ServiceName]\RunAsSvc

// - REG_SZ: Application (Mandatory)
// - REG_SZ: Parameters
// - REG_SZ: Directory
// - REG_SZ: Environment

// - REG_DWORD: EnableLogging
// - REG_SZ: LoggingRootDirectory
//   (Default: %ALLUSERSPROFILE%\RunAsSvc\Logs\[ServiceName])
//
// [LoggingRootDirectory]\[DateTimeWithMilliseconds a.k.a. yyyyMMdd.HHmmss.fff]\
// 
// - REG_DWORD: EnableStandardOutputLogging for StandardOutput.log
// - REG_DWORD: EnableStandardErrorLogging for StandardError.log
// - REG_DWORD: EnableServiceInstanceLogging for RunAsSvc.log

// - REG_DWORD: TerminateChildProcessesOnServiceStop
// - REG_DWORD: RestartOnApplicationExit
// - REG_DWORD: GracefulTerminationTimeout

namespace
{
    static std::wstring g_ServiceName;

    static SERVICE_STATUS_HANDLE volatile g_ServiceStatusHandle = nullptr;
}

void WINAPI RunAsSvcServiceHandler(
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

        //g_ServiceIsRunning = false;

        break;
    }
    default:
        break;
    }
}

void WINAPI RunAsSvcServiceMain(
    _In_ DWORD dwNumServicesArgs,
    _In_ LPWSTR* lpServiceArgVectors)
{
    UNREFERENCED_PARAMETER(dwNumServicesArgs);
    UNREFERENCED_PARAMETER(lpServiceArgVectors);

    g_ServiceStatusHandle = ::RegisterServiceCtrlHandlerW(
        g_ServiceName.c_str(),
        ::RunAsSvcServiceHandler);
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
            // Just for temporary.
            DWORD ExitCode = ERROR_SUCCESS;

            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceStatus.dwControlsAccepted = 0;
            ServiceStatus.dwWin32ExitCode = ExitCode;
            ::SetServiceStatus(g_ServiceStatusHandle, &ServiceStatus);
        }
    }
}

int main()
{
    ::std::printf(
        "RunAsSvc " MILE_PROJECT_VERSION_UTF8_STRING " (Build "
        MILE_PROJECT_MACRO_TO_UTF8_STRING(MILE_PROJECT_VERSION_BUILD) ")" "\n"
        "(c) M2-Team and Contributors. All rights reserved.\n"
        "\n");

    std::vector<std::string> Arguments = Mile::SplitCommandLineString(
        Mile::ToString(CP_UTF8, ::GetCommandLineW()));

    bool NeedParse = (Arguments.size() > 1);

    if (!NeedParse)
    {
        std::printf(
            "Format: RunAsSvc [Command] <Option1> <Option2> ...\n"
            "\n"
            "Under Construction...\n"
            "\n");
        return 0;
    }

    if (0 == ::_stricmp(Arguments[1].c_str(), "Instance"))
    {
        // Private command for service instance. Don't show it in help content.

        if (Arguments.size() < 3)
        {
            // Silent exit because this command is private and should only be
            // called by the Service Control Manager.
            return ERROR_INVALID_PARAMETER;
        }
        g_ServiceName = Mile::ToWideString(CP_UTF8, Arguments[2]);

        ::FreeConsole();

        SERVICE_TABLE_ENTRYW ServiceStartTable[] =
        {
            {
                const_cast<wchar_t*>(g_ServiceName.c_str()),
                ::RunAsSvcServiceMain
            }
        };

        return ::StartServiceCtrlDispatcherW(ServiceStartTable)
            ? ERROR_SUCCESS :
            ::GetLastError();
    }
    else
    {
        std::printf("Under Construction...\n");
    }

    return 0;
}
