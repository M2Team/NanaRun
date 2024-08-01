/*
 * PROJECT:   NanaRun
 * FILE:      SynthRdp.cpp
 * PURPOSE:   Implementation for Hyper-V Enhanced Session Proxy Service
 *
 * LICENSE:   The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#define _WINSOCKAPI_
#include <Windows.h>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <Mile.Helpers.CppBase.h>

#include <Mile.HyperV.VMBus.h>
#include <Mile.HyperV.Windows.VMBusPipe.h>

#include <Mile.Project.Version.h>

EXTERN_C HANDLE WINAPI VmbusPipeClientTryOpenChannel(
    _In_ LPCGUID InterfaceType,
    _In_ LPCGUID InterfaceInstance,
    _In_ DWORD TimeoutInMsec,
    _In_ DWORD OpenMode)
{
    VMBUS_PIPE_CHANNEL_INFO ChannelInfo = { 0 };
    if (::VmbusPipeClientWaitChannel(
        InterfaceType,
        InterfaceInstance,
        TimeoutInMsec,
        &ChannelInfo))
    {
        return ::VmbusPipeClientOpenChannel(
            &ChannelInfo,
            OpenMode);
    }

    return INVALID_HANDLE_VALUE;
}

namespace
{
    static SERVICE_STATUS_HANDLE volatile g_ServiceStatusHandle = nullptr;
    static bool volatile g_ServiceIsRunning = true;
    static bool volatile g_InteractiveMode = false;
}

SOCKET SynthRdpConnectToServer()
{
    SOCKET Result = INVALID_SOCKET;

    std::string ServerHost = "127.0.0.1";
    {
        std::wstring Buffer(32767, L'\0');
        DWORD Length = static_cast<DWORD>(Buffer.size());
        if (ERROR_SUCCESS == ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\"
            L"SynthRdp\\Configurations",
            L"ServerHost",
            RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            const_cast<wchar_t*>(Buffer.c_str()),
            &Length))
        {
            Buffer.resize(std::wcslen(Buffer.c_str()));
            ServerHost = Mile::ToString(CP_UTF8, Buffer);
        }
    }

    std::string ServerPort = "3389";
    {
        DWORD Data = 0;
        DWORD Length = sizeof(DWORD);
        if (ERROR_SUCCESS == ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\"
            L"SynthRdp\\Configurations",
            L"ServerPort",
            RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            &Data,
            &Length))
        {
            ServerPort = Mile::FormatString("%hu", Data);
        }
    }

    int LastError = 0;

    addrinfo AddressHints = { 0 };
    AddressHints.ai_family = AF_INET;
    AddressHints.ai_socktype = SOCK_STREAM;
    AddressHints.ai_protocol = IPPROTO_TCP;
    addrinfo* AddressInfo = nullptr;
    LastError = ::getaddrinfo(
        ServerHost.c_str(),
        ServerPort.c_str(),
        &AddressHints,
        &AddressInfo);
    if (0 == LastError)
    {
        for (addrinfo* Current = AddressInfo;
            nullptr != Current;
            Current = Current->ai_next)
        {
            SOCKET Socket = ::WSASocketW(
                Current->ai_family,
                Current->ai_socktype,
                Current->ai_protocol,
                nullptr,
                0,
                WSA_FLAG_OVERLAPPED);
            if (INVALID_SOCKET == Socket)
            {
                LastError = ::WSAGetLastError();
                break;
            }

            if (SOCKET_ERROR != ::WSAConnect(
                Socket,
                Current->ai_addr,
                static_cast<int>(Current->ai_addrlen),
                nullptr,
                nullptr,
                nullptr,
                nullptr))
            {
                Result = Socket;
                break;
            }

            LastError = ::WSAGetLastError();
            ::closesocket(Socket);
        }

        ::freeaddrinfo(AddressInfo);
    }

    if (INVALID_SOCKET == Result && 0 != LastError)
    {
        ::WSASetLastError(LastError);
    }

    return Result;
}

struct SynthRdpServiceConnectionContext
{
    std::uint8_t SendBuffer[16384];
    std::uint8_t RecvBuffer[16384];
};

void SynthRdpRedirectionWorker(
    _In_ HANDLE PipeHandle)
{
    SOCKET Socket = INVALID_SOCKET;
    SynthRdpServiceConnectionContext* Context = nullptr;

    do
    {
        Socket = ::SynthRdpConnectToServer();
        if (Socket == INVALID_SOCKET)
        {
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] SynthRdpConnectToServer failed (%d).\n",
                    ::WSAGetLastError());
            }
            break;
        }

        Context = reinterpret_cast<SynthRdpServiceConnectionContext*>(
            ::MileAllocateMemory(sizeof(SynthRdpServiceConnectionContext)));
        if (!Context)
        {
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] MileAllocateMemory failed.\n");
            }
            break;
        }

        // X.224 Connection Request PDU (Patched)
        {
            DWORD NumberOfBytesRead = 0;
            if (!::MileReadFile(
                PipeHandle,
                Context->SendBuffer,
                static_cast<DWORD>(sizeof(Context->SendBuffer)),
                &NumberOfBytesRead))
            {
                if (g_InteractiveMode)
                {
                    std::printf(
                        "[Error] MileReadFile failed (%d).\n",
                        ::GetLastError());
                }
                break;
            }

            // Set requestedProtocols to PROTOCOL_RDP (0x00000000).
            Context->SendBuffer[15] = 0x00;

            if (g_InteractiveMode)
            {
                std::wprintf(
                    L"[Info] MileSocketSend: %d Bytes.\n",
                    NumberOfBytesRead);
            }

            DWORD NumberOfBytesSent = 0;
            DWORD Flags = 0;
            if (!::MileSocketSend(
                Socket,
                Context->SendBuffer,
                NumberOfBytesRead,
                &NumberOfBytesSent,
                Flags))
            {
                if (g_InteractiveMode)
                {
                    std::printf(
                        "[Error] MileSocketSend failed (%d).\n",
                        ::WSAGetLastError());
                }
                break;
            }
        }

        bool volatile ShouldRunning = true;

        HANDLE Vmbus2TcpThread = Mile::CreateThread([&]()
        {
            for (; ShouldRunning && g_ServiceIsRunning;)
            {
                DWORD NumberOfBytesRead = 0;
                if (!::MileReadFile(
                    PipeHandle,
                    Context->SendBuffer,
                    static_cast<DWORD>(sizeof(Context->SendBuffer)),
                    &NumberOfBytesRead))
                {
                    if (g_InteractiveMode)
                    {
                        std::printf(
                            "[Error] MileReadFile failed (%d).\n",
                            ::GetLastError());
                    }
                    break;
                }

                if (g_InteractiveMode)
                {
                    std::printf(
                        "[Info] MileSocketSend: %d Bytes.\n",
                        NumberOfBytesRead);
                }

                DWORD NumberOfBytesSent = 0;
                DWORD Flags = 0;
                if (!::MileSocketSend(
                    Socket,
                    Context->SendBuffer,
                    NumberOfBytesRead,
                    &NumberOfBytesSent,
                    Flags))
                {
                    if (g_InteractiveMode)
                    {
                        std::printf(
                            "[Error] MileSocketSend failed (%d).\n",
                            ::WSAGetLastError());
                    }
                    break;
                }
            }
        });

        HANDLE Tcp2VmbusThread = Mile::CreateThread([&]()
        {
            for (; ShouldRunning && g_ServiceIsRunning;)
            {
                DWORD NumberOfBytesRecvd = 0;
                DWORD Flags = MSG_PARTIAL;
                if (!::MileSocketRecv(
                    Socket,
                    Context->RecvBuffer,
                    static_cast<DWORD>(sizeof(Context->RecvBuffer)),
                    &NumberOfBytesRecvd,
                    &Flags))
                {
                    if (g_InteractiveMode)
                    {
                        std::printf(
                            "[Error] MileSocketRecv failed (%d).\n",
                            ::WSAGetLastError());
                    }
                    break;
                }

                if (g_InteractiveMode)
                {
                    std::printf(
                        "[Info] MileWriteFile: %d Bytes.\n",
                        NumberOfBytesRecvd);
                }

                DWORD NumberOfBytesWritten = 0;
                if (!::MileWriteFile(
                    PipeHandle,
                    Context->RecvBuffer,
                    NumberOfBytesRecvd,
                    &NumberOfBytesWritten))
                {
                    if (g_InteractiveMode)
                    {
                        std::printf(
                            "[Error] MileWriteFile failed (%d).\n",
                            ::GetLastError());
                    }
                    break;
                }
            }
        });

        for (;;)
        {
            if (WAIT_TIMEOUT != ::WaitForSingleObject(
                Vmbus2TcpThread,
                100))
            {
                ShouldRunning = false;
                break;
            }

            if (WAIT_TIMEOUT != ::WaitForSingleObject(
                Tcp2VmbusThread,
                100))
            {
                ShouldRunning = false;
                break;
            }
        }

        if (Vmbus2TcpThread)
        {
            ::CloseHandle(Vmbus2TcpThread);
            Vmbus2TcpThread = nullptr;
        }

        if (Tcp2VmbusThread)
        {
            ::CloseHandle(Tcp2VmbusThread);
            Tcp2VmbusThread = nullptr;
        }

    } while (false);

    if (Context)
    {
        ::MileFreeMemory(Context);
    }

    if (Socket != INVALID_SOCKET)
    {
        ::closesocket(Socket);
    }
}

DWORD SynthRdpMain()
{
    WSADATA WSAData = { 0 };
    {
        int WSAError = ::WSAStartup(MAKEWORD(2, 2), &WSAData);
        if (NO_ERROR != WSAError)
        {
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] WSAStartup failed (%d).\n",
                    WSAError);
            }
            return WSAError;
        }
    }

    DWORD Error = ERROR_SUCCESS;

    HANDLE ControlChannelHandle = INVALID_HANDLE_VALUE;

    do
    {
        ControlChannelHandle = ::VmbusPipeClientTryOpenChannel(
            &SYNTHRDP_CONTROL_CLASS_ID,
            &SYNTHRDP_CONTROL_INSTANCE_ID,
            INFINITE,
            FILE_FLAG_OVERLAPPED);
        if (INVALID_HANDLE_VALUE == ControlChannelHandle)
        {
            Error = ::GetLastError();
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] VmbusPipeClientTryOpenChannel failed (%d).\n",
                    Error);
            }
            break;
        }

        SYNTHRDP_VERSION_REQUEST_MESSAGE Request;
        std::memset(
            &Request,
            0,
            sizeof(SYNTHRDP_VERSION_REQUEST_MESSAGE));
        Request.Header.Type = SynthrdpVersionRequest;
        Request.Header.Size = 0;
        Request.Version.AsDWORD = SYNTHRDP_VERSION_WINBLUE;
        Request.Reserved = 0;
        DWORD NumberOfBytesWritten = 0;
        if (!::MileWriteFile(
            ControlChannelHandle,
            &Request,
            sizeof(Request),
            &NumberOfBytesWritten))
        {
            Error = ::GetLastError();
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] MileWriteFile failed (%d).\n",
                    Error);
            }
            break;
        }

        if (sizeof(SYNTHRDP_VERSION_REQUEST_MESSAGE) != NumberOfBytesWritten)
        {
            Error = ERROR_INVALID_DATA;
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] SYNTHRDP_VERSION_REQUEST_MESSAGE Invalid.\n");
            }
            break;
        }

        SYNTHRDP_VERSION_RESPONSE_MESSAGE Response;
        std::memset(
            &Response,
            0,
            sizeof(SYNTHRDP_VERSION_RESPONSE_MESSAGE));
        DWORD NumberOfBytesRead = 0;
        if (!::MileReadFile(
            ControlChannelHandle,
            &Response,
            sizeof(Response),
            &NumberOfBytesRead))
        {
            Error = ::GetLastError();
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] MileReadFile failed (%d).\n",
                    Error);
            }
            break;
        }

        if (sizeof(SYNTHRDP_VERSION_RESPONSE_MESSAGE) != NumberOfBytesRead ||
            SYNTHRDP_TRUE_WITH_VERSION_EXCHANGE != Response.IsAccepted)
        {
            Error = ERROR_INVALID_DATA;
            if (g_InteractiveMode)
            {
                std::printf(
                    "[Error] SYNTHRDP_VERSION_RESPONSE_MESSAGE Invalid.\n");
            }
            break;
        }

        GUID Instances[] =
        {
            SYNTHRDP_DATA_INSTANCE_ID_1,
            SYNTHRDP_DATA_INSTANCE_ID_2,
            SYNTHRDP_DATA_INSTANCE_ID_3,
            SYNTHRDP_DATA_INSTANCE_ID_4,
            SYNTHRDP_DATA_INSTANCE_ID_5
        };
        for (size_t i = 0; g_ServiceIsRunning; ++i)
        {
            HANDLE DataChannelHandle = ::VmbusPipeClientTryOpenChannel(
                &SYNTHRDP_DATA_CLASS_ID,
                &Instances[i % (sizeof(Instances) / sizeof(*Instances))],
                50,
                FILE_FLAG_OVERLAPPED);
            if (INVALID_HANDLE_VALUE == DataChannelHandle)
            {
                continue;
            }

            ::SynthRdpRedirectionWorker(DataChannelHandle);

            ::CloseHandle(DataChannelHandle);
        }

    } while (false);

    if (INVALID_HANDLE_VALUE != ControlChannelHandle)
    {
        ::CloseHandle(ControlChannelHandle);
    }

    ::WSACleanup();

    return Error;
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
        std::printf("[Success] SynthRdpInstallService\n");
    }
    else
    {
        std::printf("[Error] SynthRdpInstallService (%d)\n", Error);
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
        std::printf("[Success] SynthRdpUninstallService\n");
    }
    else
    {
        std::printf("[Error] SynthRdpUninstallService (%d)\n", Error);
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
        std::printf("[Success] SynthRdpStartService\n");
    }
    else
    {
        std::printf("[Error] SynthRdpStartService (%d)\n", Error);
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
        std::printf("[Success] SynthRdpStopService\n");
    }
    else
    {
        std::printf("[Error] SynthRdpStopService (%d)\n", Error);
    }

    return Error;
}

int SynthRdpListConfigurations()
{
    DWORD Error = ERROR_SUCCESS;

    DWORD Data = 0;
    DWORD Length = 0;

    bool DisableRemoteDesktop = false;
    {
        Data = 0;
        Length = sizeof(DWORD);
        Error = ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server",
            L"fDenyTSConnections",
            RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            &Data,
            &Length);
        if (ERROR_SUCCESS == Error)
        {
            DisableRemoteDesktop = Data;
        }
    }

    bool EnableUserAuthentication = true;
    {
        Data = 0;
        Length = sizeof(DWORD);
        Error = ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\"
            L"WinStations\\RDP-Tcp",
            L"UserAuthentication",
            RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            &Data,
            &Length);
        if (ERROR_SUCCESS == Error)
        {
            EnableUserAuthentication = Data;
        }
    }

    bool DisableBlankPassword = false;
    {
        Data = 0;
        Length = sizeof(DWORD);
        Error = ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Lsa",
            L"LimitBlankPasswordUse",
            RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            &Data,
            &Length);
        if (ERROR_SUCCESS == Error)
        {
            DisableBlankPassword = Data;
        }
    }

    bool OverrideSystemImplementation = false;
    {
        Data = 0;
        Length = sizeof(DWORD);
        Error = ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Virtual Machine\\Guest",
            L"DisableEnhancedSessionConsoleConnection",
            RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            &Data,
            &Length);
        if (ERROR_SUCCESS == Error)
        {
            OverrideSystemImplementation = Data;
        }
    }

    std::string ServerHost = "127.0.0.1";
    {
        std::wstring Buffer(32767, L'\0');

        Length = static_cast<DWORD>(Buffer.size());
        Error = ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\"
            L"SynthRdp\\Configurations",
            L"ServerHost",
            RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            const_cast<wchar_t*>(Buffer.c_str()),
            &Length);
        if (ERROR_SUCCESS == Error)
        {
            Buffer.resize(std::wcslen(Buffer.c_str()));
            ServerHost = Mile::ToString(CP_UTF8, Buffer);
        }
    }

    std::uint16_t ServerPort = 3389;
    {
        Data = 0;
        Length = sizeof(DWORD);
        Error = ::RegGetValueW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\"
            L"SynthRdp\\Configurations",
            L"ServerPort",
            RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            &Data,
            &Length);
        if (ERROR_SUCCESS == Error)
        {
            ServerPort = static_cast<std::uint16_t>(Data);
        }
    }

    std::printf(
        "Configurations:\n"
        "\n"
        "DisableRemoteDesktop: %s\n"
        "EnableUserAuthentication: %s\n"
        "DisableBlankPassword: %s\n"
        "OverrideSystemImplementation: %s\n"
        "ServerHost: %s\n"
        "ServerPort: %hu\n"
        "\n",
        DisableRemoteDesktop ? "True" : "False",
        EnableUserAuthentication ? "True" : "False",
        DisableBlankPassword ? "True" : "False",
        OverrideSystemImplementation ? "True" : "False",
        ServerHost.c_str(),
        ServerPort);

    return Error;
}

int SynthRdpUpdateConfiguration(
    std::string const& Key,
    std::string const& Value)
{
    DWORD Error = ERROR_SUCCESS;

    if (0 == ::_stricmp(Key.c_str(), "DisableRemoteDesktop"))
    {
        DWORD Data = 1;
        if (Value.empty() ||
            0 == ::_stricmp(Value.c_str(), "True"))
        {
            // Use the default value.
        }
        else if (0 == ::_stricmp(Value.c_str(), "False"))
        {
            Data = 0;
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
        }

        if (ERROR_SUCCESS == Error)
        {
            Error = ::RegSetKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server",
                L"fDenyTSConnections",
                REG_DWORD,
                &Data,
                sizeof(DWORD));
        }
    }
    else if (0 == ::_stricmp(Key.c_str(), "EnableUserAuthentication"))
    {
        DWORD Data = 1;
        if (Value.empty() ||
            0 == ::_stricmp(Value.c_str(), "True"))
        {
            // Use the default value.
        }
        else if (0 == ::_stricmp(Value.c_str(), "False"))
        {
            Data = 0;
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
        }

        if (ERROR_SUCCESS == Error)
        {
            Error = ::RegSetKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\"
                L"WinStations\\RDP-Tcp",
                L"UserAuthentication",
                REG_DWORD,
                &Data,
                sizeof(DWORD));
        }
    }
    else if (0 == ::_stricmp(Key.c_str(), "DisableBlankPassword"))
    {
        DWORD Data = 1;
        if (Value.empty() ||
            0 == ::_stricmp(Value.c_str(), "True"))
        {
            // Use the default value.
        }
        else if (0 == ::_stricmp(Value.c_str(), "False"))
        {
            Data = 0;
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
        }

        if (ERROR_SUCCESS == Error)
        {
            Error = ::RegSetKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\Lsa",
                L"LimitBlankPasswordUse",
                REG_DWORD,
                &Data,
                sizeof(DWORD));
        }
    }
    else if (0 == ::_stricmp(Key.c_str(), "OverrideSystemImplementation"))
    {
        if (Value.empty() ||
            0 == ::_stricmp(Value.c_str(), "False"))
        {
            Error = ::RegDeleteKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Virtual Machine\\Guest",
                L"DisableEnhancedSessionConsoleConnection");
        }
        else if (0 == ::_stricmp(Value.c_str(), "True"))
        {
            DWORD Data = 1;
            Error = ::RegSetKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Virtual Machine\\Guest",
                L"DisableEnhancedSessionConsoleConnection",
                REG_DWORD,
                &Data,
                sizeof(DWORD));
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
        }
    }
    else if (0 == ::_stricmp(Key.c_str(), "ServerHost"))
    {
        if (Value.empty())
        {
            Error = ::RegDeleteKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\"
                L"SynthRdp\\Configurations",
                L"ServerHost");
        }
        else
        {
            std::wstring ServerHost = Mile::ToWideString(CP_UTF8, Value);

            Error = ::RegSetKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\"
                L"SynthRdp\\Configurations",
                L"ServerHost",
                REG_SZ,
                ServerHost.c_str(),
                static_cast<DWORD>((ServerHost.size() + 1) * sizeof(wchar_t)));
        }
    }
    else if (0 == ::_stricmp(Key.c_str(), "ServerPort"))
    {
        if (Value.empty())
        {
            Error = ::RegDeleteKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\SynthRdp\\Configurations",
                L"ServerPort");
        }
        else
        {
            DWORD Data = static_cast<std::uint16_t>(Mile::ToUInt32(Value));
            Error = ::RegSetKeyValueW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\SynthRdp\\Configurations",
                L"ServerPort",
                REG_DWORD,
                &Data,
                sizeof(DWORD));
        }
    }
    else
    {
        Error = ERROR_INVALID_PARAMETER;
    }

    if (ERROR_SUCCESS == Error)
    {
        std::printf(
            "[Success] SynthRdpUpdateConfiguration %s\n",
            Key.c_str());
    }
    else
    {
        std::printf(
            "[Error] SynthRdpUpdateConfiguration %s (%d)\n",
            Key.c_str(),
            Error);
    }

    return Error;
}

int main()
{
    ::std::printf(
        "SynthRdp " MILE_PROJECT_VERSION_UTF8_STRING " (Build "
        MILE_PROJECT_MACRO_TO_UTF8_STRING(MILE_PROJECT_VERSION_BUILD) ")" "\n"
        "(c) M2-Team and Contributors. All rights reserved.\n"
        "\n");

    std::vector<std::string> Arguments = Mile::SplitCommandLineString(
        Mile::ToString(CP_UTF8, ::GetCommandLineW()));

    bool NeedParse = (Arguments.size() > 1);

    if (!NeedParse)
    {
        g_InteractiveMode = true;

        std::printf(
            "[Info] SynthRdp will run as a console application instead of "
            "service.\n"
            "[Info] Use \"SynthRdp Help\" for more commands.\n"
            "\n");

        return ::SynthRdpMain();
    }

    int Result = 0;

    bool ParseError = false;
    bool ShowHelp = false;

    if (0 == ::_stricmp(Arguments[1].c_str(), "Help"))
    {
        ShowHelp = true;
    }
    else if (0 == ::_stricmp(Arguments[1].c_str(), "Service"))
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
    else if (0 == ::_stricmp(Arguments[1].c_str(), "Install"))
    {
        Result = ::SynthRdpInstallService();
    }
    else if (0 == ::_stricmp(Arguments[1].c_str(), "Uninstall"))
    {
        Result = ::SynthRdpUninstallService();
    }
    else if (0 == ::_stricmp(Arguments[1].c_str(), "Start"))
    {
        Result = ::SynthRdpStartService();
    }
    else if (0 == ::_stricmp(Arguments[1].c_str(), "Stop"))
    {
        Result = ::SynthRdpStopService();
    }
    else if (0 == ::_stricmp(Arguments[1].c_str(), "Config"))
    {
        ParseError = !(Arguments.size() > 2);
        if (!ParseError)
        {
            if (0 == ::_stricmp(Arguments[2].c_str(), "List"))
            {
                Result = ::SynthRdpListConfigurations();
            }
            else if (0 == ::_stricmp(Arguments[2].c_str(), "Set"))
            {
                ParseError = !(Arguments.size() > 3);
                if (!ParseError)
                {
                    Result = ::SynthRdpUpdateConfiguration(
                        Arguments[3],
                        (Arguments.size() > 4) ? Arguments[4] : std::string());
                }
            }
        }
    }
    else
    {
        ParseError = true;
    }

    if (ParseError)
    {
        std::printf(
            "[Error] Unrecognized command.\n"
            "\n");
    }

    if (ParseError || ShowHelp)
    {
        std::printf(
            "Format: SynthRdp [Command] <Option1> <Option2> ...\n"
            "\n"
            "Commands:\n"
            "\n"
            "  Help - Show this content.\n"
            "\n"
            "  Install - Install SynthRdp service.\n"
            "  Uninstall - Uninstall SynthRdp service.\n"
            "  Start - Start SynthRdp service.\n"
            "  Stop - Stop SynthRdp service.\n"
            "\n"
            "  Config List - List all configurations related to SynthRdp.\n"
            "  Config Set [Key] <Value> - Set the specific configuration\n"
            "                             with the specific value, or reset\n"
            "                             the specific configuration if you\n"
            "                             don't specify the value.\n"
            "\n"
            "Configuration Keys:\n"
            "\n"
            "  DisableRemoteDesktop <True|False>\n"
            "    Set False to enable the remote desktop for this virtual\n"
            "    machine. The default setting is True. Remote Desktop is\n"
            "    necessary for the Hyper-V Enhanced Session because it is\n"
            "    actually the RDP connection over the VMBus pipe.\n"
            "  EnableUserAuthentication <True|False>\n"
            "    Set False to allow connections without Network Level\n"
            "    Authentication. The default setting is True. Set this\n"
            "    configuration option False is necessary for using the\n"
            "    Hyper-V Enhanced Session via the SynthRdp service.\n"
            "  DisableBlankPassword <True|False>\n"
            "    Set False to enable the usage of logging on this virtual\n"
            "    machine as the account with the blank password via remote\n"
            "    desktop. The default setting is True. Set this\n"
            "    configuration option False will make you use the Hyper-V\n"
            "    Enhanced Session via the SynthRdp service happier, but\n"
            "    compromise the security.\n"
            "  OverrideSystemImplementation <True|False>\n"
            "    Set True to use the Hyper-V Enhanced Session via the\n"
            "    SynthRdp service on Windows 8.1 / Server 2012 R2 or later\n"
            "    guests which have the built-in Hyper-V Enhanced Session\n"
            "    support for this virtual machine. The default setting is\n"
            "    False. You can set this configuration option True if you\n"
            "    want to use your current virtual machine as the Hyper-V\n"
            "    Enhanced Session proxy server. You need to reboot your\n"
            "    virtual machine for applying this configuration option\n"
            "    change.\n"
            "  ServerHost <Host>\n"
            "    Set the server host for the remote desktop connection you\n"
            "    want to use in the Hyper-V Enhanced Session. The default\n"
            "    setting is 127.0.0.1.\n"
            "  ServerPort <Port>\n"
            "    Set the server port for the remote desktop connection you\n"
            "    want to use in the Hyper-V Enhanced Session. The default\n"
            "    setting is 3389.\n"
            "\n"
            "Notes:\n"
            "  - All command options are case-insensitive.\n"
            "  - SynthRdp will run as a console application instead of\n"
            "    service if you don't specify another command.\n"
            "\n"
            "Examples:\n"
            "\n"
            "  SynthRdp Install\n"
            "  SynthRdp Uninstall\n"
            "  SynthRdp Start\n"
            "  SynthRdp Stop\n"
            "\n"
            "  SynthRdp Config List\n"
            "\n"
            "  SynthRdp Config Set DisableRemoteDesktop False\n"
            "  SynthRdp Config Set EnableUserAuthentication False\n"
            "  SynthRdp Config Set DisableBlankPassword False\n"
            "  SynthRdp Config Set OverrideSystemImplementation False\n"
            "  SynthRdp Config Set ServerHost 127.0.0.1\n"
            "  SynthRdp Config Set ServerPort 3389\n"
            "\n"
            "  SynthRdp Config Set DisableRemoteDesktop\n"
            "  SynthRdp Config Set EnableUserAuthentication\n"
            "  SynthRdp Config Set DisableBlankPassword\n"
            "  SynthRdp Config Set OverrideSystemImplementation\n"
            "  SynthRdp Config Set ServerHost\n"
            "  SynthRdp Config Set ServerPort\n"
            "\n");
    }

    return Result;
}
