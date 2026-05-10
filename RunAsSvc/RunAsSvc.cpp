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

// RunAsSvc Install [ServiceName] [Options]
// RunAsSvc Uninstall [ServiceName]
// RunAsSvc Start [ServiceName]
// RunAsSvc Stop [ServiceName]
// RunAsSvc Enable [ServiceName]
// RunAsSvc Disable [ServiceName]

// Private
// RunAsSvc Instance [ServiceName]

// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\[ServiceName]
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services[ServiceName]\RunAsSvc
// - REG_SZ: ApplicationName
// - REG_SZ: CommandLine
// - REG_SZ: CurrentDirectory
// - REG_MULTI_SZ: EnvironmentVariables
// - REG_DWORD: TerminateChildProcessesOnServiceStop
// - REG_DWORD: RestartOnApplicationExit
// - REG_DWORD: GracefulTerminationTimeout
// - REG_SZ: StandardOutputRedirectFilePath
// - REG_SZ: StandardErrorRedirectFilePath

int main()
{
    std::wprintf(L"Hello World! - RunAsSvc");

    return 0;
}
