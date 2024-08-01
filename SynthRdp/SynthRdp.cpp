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
    static std::string g_ServerHost = "127.0.0.1";
    static std::string g_ServerPort = "3389";

    static SERVICE_STATUS_HANDLE volatile g_ServiceStatusHandle = nullptr;
    static bool volatile g_ServiceIsRunning = true;
}

SOCKET SynthRdpConnectToServer()
{
    SOCKET Result = INVALID_SOCKET;

    int LastError = 0;

    addrinfo AddressHints = { 0 };
    AddressHints.ai_family = AF_INET;
    AddressHints.ai_socktype = SOCK_STREAM;
    AddressHints.ai_protocol = IPPROTO_TCP;
    addrinfo* AddressInfo = nullptr;
    LastError = ::getaddrinfo(
        g_ServerHost.c_str(),
        g_ServerPort.c_str(),
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
            std::printf(
                "[Error] SynthRdpConnectToServer failed (%d).\n",
                ::WSAGetLastError());
            break;
        }

        Context = reinterpret_cast<SynthRdpServiceConnectionContext*>(
            ::MileAllocateMemory(sizeof(SynthRdpServiceConnectionContext)));
        if (!Context)
        {
            std::printf(
                "[Error] MileAllocateMemory failed.\n");
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
                std::printf(
                    "[Error] MileReadFile failed (%d).\n",
                    ::GetLastError());
                break;
            }

            // Set requestedProtocols to PROTOCOL_RDP (0x00000000).
            Context->SendBuffer[15] = 0x00;

            std::wprintf(
                L"[Info] MileSocketSend: %d Bytes.\n",
                NumberOfBytesRead);

            DWORD NumberOfBytesSent = 0;
            DWORD Flags = 0;
            if (!::MileSocketSend(
                Socket,
                Context->SendBuffer,
                NumberOfBytesRead,
                &NumberOfBytesSent,
                Flags))
            {
                std::printf(
                    "[Error] MileSocketSend failed (%d).\n",
                    ::WSAGetLastError());
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
                    std::printf(
                        "[Error] MileReadFile failed (%d).\n",
                        ::GetLastError());
                    break;
                }

                std::printf(
                    "[Info] MileSocketSend: %d Bytes.\n",
                    NumberOfBytesRead);

                DWORD NumberOfBytesSent = 0;
                DWORD Flags = 0;
                if (!::MileSocketSend(
                    Socket,
                    Context->SendBuffer,
                    NumberOfBytesRead,
                    &NumberOfBytesSent,
                    Flags))
                {
                    std::printf(
                        "[Error] MileSocketSend failed (%d).\n",
                        ::WSAGetLastError());
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
                    std::printf(
                        "[Error] MileSocketRecv failed (%d).\n",
                        ::WSAGetLastError());
                    break;
                }

                std::printf(
                    "[Info] MileWriteFile: %d Bytes.\n",
                    NumberOfBytesRecvd);

                DWORD NumberOfBytesWritten = 0;
                if (!::MileWriteFile(
                    PipeHandle,
                    Context->RecvBuffer,
                    NumberOfBytesRecvd,
                    &NumberOfBytesWritten))
                {
                    std::printf(
                        "[Error] MileWriteFile failed (%d).\n",
                        ::GetLastError());
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
            std::printf(
                "[Error] WSAStartup failed (%d).\n",
                WSAError);
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
            std::printf(
                "[Error] VmbusPipeClientTryOpenChannel failed (%d).\n",
                Error);
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
            std::printf(
                "[Error] MileWriteFile failed (%d).\n",
                Error);
            break;
        }

        if (sizeof(SYNTHRDP_VERSION_REQUEST_MESSAGE) != NumberOfBytesWritten)
        {
            Error = ERROR_INVALID_DATA;
            std::printf(
                "[Error] SYNTHRDP_VERSION_REQUEST_MESSAGE Invalid.\n");
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
            std::printf(
                "[Error] MileReadFile failed (%d).\n",
                Error);
            break;
        }

        if (sizeof(SYNTHRDP_VERSION_RESPONSE_MESSAGE) != NumberOfBytesRead ||
            SYNTHRDP_TRUE_WITH_VERSION_EXCHANGE != Response.IsAccepted)
        {
            Error = ERROR_INVALID_DATA;
            std::printf(
                "[Error] SYNTHRDP_VERSION_RESPONSE_MESSAGE Invalid.\n");
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
            L"SYSTEM\\CurrentControlSet\\Services\\SynthRdp\\Configurations",
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
            L"SYSTEM\\CurrentControlSet\\Services\\SynthRdp\\Configurations",
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
        "Configurations:\r\n"
        "\r\n"
        "DisableRemoteDesktop: %s\r\n"
        "DisableBlankPassword: %s\r\n"
        "OverrideSystemImplementation: %s\r\n"
        "ServerHost: %s\r\n"
        "ServerPort: %hu\r\n"
        "\r\n",
        DisableRemoteDesktop ? "True" : "False",
        DisableBlankPassword ? "True" : "False",
        OverrideSystemImplementation ? "True" : "False",
        ServerHost.c_str(),
        ServerPort);

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
        std::printf(
            "[Info] SynthRdp will run as a console application instead of "
            "service.\r\n"
            "[Info] Use \"SynthRdp Help\" for more commands.\r\n"
            "\r\n");

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
    else if (0 == _stricmp(Arguments[1].c_str(), "Config"))
    {
        ParseError = !(Arguments.size() > 2);
        if (!ParseError)
        {
            if (0 == _stricmp(Arguments[2].c_str(), "List"))
            {
                Result = ::SynthRdpListConfigurations();
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
            "[Error] Unrecognized command.\r\n"
            "\r\n");
    }

    if (ParseError || ShowHelp)
    {
        std::printf(
            "Format: SynthRdp [Command] <Option1> <Option2> ...\r\n"
            "\r\n"
            "Commands:\r\n"
            "\r\n"
            "  Help - Show this content.\r\n"
            "\r\n"
            "  Install - Install SynthRdp service.\r\n"
            "  Uninstall - Uninstall SynthRdp service.\r\n"
            "  Start - Start SynthRdp service.\r\n"
            "  Stop - Stop SynthRdp service.\r\n"
            "\r\n"
            "  Config List - List all configurations related to SynthRdp.\r\n"
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
