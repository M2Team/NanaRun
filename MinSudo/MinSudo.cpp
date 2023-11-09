/*
 * PROJECT:   NanaRun
 * FILE:      MinSudo.cpp
 * PURPOSE:   Implementation for MinSudo
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: MouriNaruto (KurikoMouri@outlook.jp)
 */

#include <Windows.h>

#include <WtsApi32.h>
#pragma comment(lib, "WtsApi32.lib")

#include <Userenv.h>
#pragma comment(lib, "Userenv.lib")

#include <cstdint>
#include <cwchar>

#include <map>
#include <string>
#include <vector>

#include <Mile.Project.Version.h>

#include "resource.h"

#include <Mile.Helpers.CppBase.h>

namespace
{
    void SplitCommandLineEx(
        std::wstring const& CommandLine,
        std::vector<std::wstring> const& OptionPrefixes,
        std::vector<std::wstring> const& OptionParameterSeparators,
        std::wstring& ApplicationName,
        std::map<std::wstring, std::wstring>& OptionsAndParameters,
        std::wstring& UnresolvedCommandLine)
    {
        ApplicationName.clear();
        OptionsAndParameters.clear();
        UnresolvedCommandLine.clear();

        size_t arg_size = 0;
        for (auto& SplitArgument : Mile::SplitCommandLineWideString(CommandLine))
        {
            // We need to process the application name at the beginning.
            if (ApplicationName.empty())
            {
                // For getting the unresolved command line, we need to cumulate
                // length which including spaces.
                arg_size += SplitArgument.size() + 1;

                // Save
                ApplicationName = SplitArgument;
            }
            else
            {
                bool IsOption = false;
                size_t OptionPrefixLength = 0;

                for (auto& OptionPrefix : OptionPrefixes)
                {
                    if (0 == _wcsnicmp(
                        SplitArgument.c_str(),
                        OptionPrefix.c_str(),
                        OptionPrefix.size()))
                    {
                        IsOption = true;
                        OptionPrefixLength = OptionPrefix.size();
                    }
                }

                if (IsOption)
                {
                    // For getting the unresolved command line, we need to cumulate
                    // length which including spaces.
                    arg_size += SplitArgument.size() + 1;

                    // Get the option name and parameter.

                    wchar_t* OptionStart = &SplitArgument[0] + OptionPrefixLength;
                    wchar_t* ParameterStart = nullptr;

                    for (auto& OptionParameterSeparator
                        : OptionParameterSeparators)
                    {
                        wchar_t* Result = wcsstr(
                            OptionStart,
                            OptionParameterSeparator.c_str());
                        if (nullptr == Result)
                        {
                            continue;
                        }

                        Result[0] = L'\0';
                        ParameterStart = Result + OptionParameterSeparator.size();

                        break;
                    }

                    // Save
                    OptionsAndParameters[(OptionStart ? OptionStart : L"")] =
                        (ParameterStart ? ParameterStart : L"");
                }
                else
                {
                    // Get the approximate location of the unresolved command line.
                    // We use "(arg_size - 1)" to ensure that the program path
                    // without quotes can also correctly parse.
                    wchar_t* search_start =
                        const_cast<wchar_t*>(CommandLine.c_str()) + (arg_size - 1);

                    // Get the unresolved command line. Search for the beginning of
                    // the first parameter delimiter called space and exclude the
                    // first space by adding 1 to the result.
                    wchar_t* command = wcsstr(search_start, L" ") + 1;

                    // Omit the space. (Thanks to wzzw.)
                    while (command && *command == L' ')
                    {
                        ++command;
                    }

                    // Save
                    if (command)
                    {
                        UnresolvedCommandLine = command;
                    }

                    break;
                }
            }
        }
    }

    std::wstring GetCurrentProcessModulePath()
    {
        // 32767 is the maximum path length without the terminating null character.
        std::wstring Path(32767, L'\0');
        Path.resize(::GetModuleFileNameW(
            nullptr, &Path[0], static_cast<DWORD>(Path.size())));
        return Path;
    }

    std::wstring GetWorkingDirectory()
    {
        // 32767 is the maximum path length without the terminating null character.
        std::wstring Path(32767, L'\0');
        Path.resize(::GetCurrentDirectoryW(
            static_cast<DWORD>(Path.size()), &Path[0]));
        return Path;
    }

    void WriteToConsole(
        std::wstring_view const& String)
    {
        HANDLE ConsoleOutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);

        DWORD NumberOfCharsWritten = 0;
        if (!::WriteConsoleW(
            ConsoleOutputHandle,
            String.data(),
            static_cast<DWORD>(String.size()),
            &NumberOfCharsWritten,
            nullptr))
        {
            std::string CurrentCodePageString = Mile::ToString(
                ::GetConsoleOutputCP(),
                String);

            ::WriteFile(
                ConsoleOutputHandle,
                CurrentCodePageString.c_str(),
                static_cast<DWORD>(CurrentCodePageString.size()),
                &NumberOfCharsWritten,
                nullptr);
        }
    }

    typedef struct _RESOURCE_INFO
    {
        DWORD Size;
        LPVOID Pointer;
    } RESOURCE_INFO, * PRESOURCE_INFO;

    BOOL SimpleLoadResource(
        _Out_ PRESOURCE_INFO ResourceInfo,
        _In_opt_ HMODULE ModuleHandle,
        _In_ LPCWSTR Type,
        _In_ LPCWSTR Name)
    {
        if (!ResourceInfo)
        {
            ::SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        std::memset(
            ResourceInfo,
            0,
            sizeof(RESOURCE_INFO));

        HRSRC ResourceFind = ::FindResourceExW(
            ModuleHandle,
            Type,
            Name,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
        if (!ResourceFind)
        {
            return FALSE;
        }

        ResourceInfo->Size = ::SizeofResource(
            ModuleHandle,
            ResourceFind);
        if (ResourceInfo->Size == 0)
        {
            return FALSE;
        }

        HGLOBAL ResourceLoad = ::LoadResource(
            ModuleHandle,
            ResourceFind);
        if (!ResourceLoad)
        {
            return FALSE;
        }

        ResourceInfo->Pointer = ::LockResource(
            ResourceLoad);

        return TRUE;
    }

    std::map<std::string, std::wstring> ParseStringDictionary(
        std::string_view const& Content)
    {
        constexpr std::string_view KeySeparator = "\r\n- ";
        constexpr std::string_view ValueStartSeparator = "\r\n```\r\n";
        constexpr std::string_view ValueEndSeparator = "\r\n```";

        std::map<std::string, std::wstring> Result;

        if (Content.empty())
        {
            return Result;
        }

        const char* Start = Content.data();
        const char* End = Start + Content.size();

        while (Start < End)
        {
            const char* KeyStart = std::strstr(
                Start,
                KeySeparator.data());
            if (!KeyStart)
            {
                break;
            }
            KeyStart += KeySeparator.size();

            const char* KeyEnd = std::strstr(
                KeyStart,
                ValueStartSeparator.data());
            if (!KeyEnd)
            {
                break;
            }

            const char* ValueStart =
                KeyEnd + ValueStartSeparator.size();

            const char* ValueEnd = std::strstr(
                ValueStart,
                ValueEndSeparator.data());
            if (!ValueEnd)
            {
                break;
            }

            Start = ValueEnd + ValueEndSeparator.size();

            Result.emplace(std::pair(
                std::string(KeyStart, KeyEnd - KeyStart),
                Mile::ToWideString(
                    CP_UTF8,
                    std::string(ValueStart, ValueEnd - ValueStart))));
        }

        return Result;
    }

    DWORD GetActiveSessionID()
    {
        DWORD Count = 0;
        PWTS_SESSION_INFOW pSessionInfo = nullptr;
        if (::WTSEnumerateSessionsW(
            WTS_CURRENT_SERVER_HANDLE,
            0,
            1,
            &pSessionInfo,
            &Count))
        {
            for (DWORD i = 0; i < Count; ++i)
            {
                if (pSessionInfo[i].State == WTS_CONNECTSTATE_CLASS::WTSActive)
                {
                    return pSessionInfo[i].SessionId;
                }
            }

            ::WTSFreeMemory(pSessionInfo);
        }

        return static_cast<DWORD>(-1);
    }

    BOOL CreateSystemToken(
        _In_ DWORD DesiredAccess,
        _Out_ PHANDLE TokenHandle)
    {
        // If the specified process is the System Idle Process (0x00000000), the
        // function fails and the last error code is ERROR_INVALID_PARAMETER.
        // So this is why 0 is the default value of dwLsassPID and dwWinLogonPID.

        // For fix the issue that @_kod0k and @DennyAmaro mentioned in
        // https://forums.mydigitallife.net/threads/59268/page-28#post-1672011 and
        // https://forums.mydigitallife.net/threads/59268/page-28#post-1674985.
        // Mile::CreateSystemToken will try to open the access token from lsass.exe
        // for maximum privileges in the access token, and try to open the access
        // token from winlogon.exe of current active session as fallback.

        // If no source process of SYSTEM access token can be found, the error code
        // will be HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER).

        DWORD dwLsassPID = 0;
        DWORD dwWinLogonPID = 0;
        PWTS_PROCESS_INFOW pProcesses = nullptr;
        DWORD dwProcessCount = 0;
        DWORD dwSessionID = ::GetActiveSessionID();

        if (::WTSEnumerateProcessesW(
            WTS_CURRENT_SERVER_HANDLE,
            0,
            1,
            &pProcesses,
            &dwProcessCount))
        {
            for (DWORD i = 0; i < dwProcessCount; ++i)
            {
                PWTS_PROCESS_INFOW pProcess = &pProcesses[i];

                if ((!pProcess->pProcessName) ||
                    (!pProcess->pUserSid) ||
                    (!::IsWellKnownSid(
                        pProcess->pUserSid,
                        WELL_KNOWN_SID_TYPE::WinLocalSystemSid)))
                {
                    continue;
                }

                if ((0 == dwLsassPID) &&
                    (0 == pProcess->SessionId) &&
                    (0 == ::_wcsicmp(L"lsass.exe", pProcess->pProcessName)))
                {
                    dwLsassPID = pProcess->ProcessId;
                    continue;
                }

                if ((0 == dwWinLogonPID) &&
                    (dwSessionID == pProcess->SessionId) &&
                    (0 == ::_wcsicmp(L"winlogon.exe", pProcess->pProcessName)))
                {
                    dwWinLogonPID = pProcess->ProcessId;
                    continue;
                }
            }

            ::WTSFreeMemory(pProcesses);
        }

        BOOL Result = FALSE;
        HANDLE SystemProcessHandle = nullptr;

        SystemProcessHandle = ::OpenProcess(
            PROCESS_QUERY_INFORMATION,
            FALSE,
            dwLsassPID);
        if (!SystemProcessHandle)
        {
            SystemProcessHandle = ::OpenProcess(
                PROCESS_QUERY_INFORMATION,
                FALSE,
                dwWinLogonPID);
        }

        if (SystemProcessHandle)
        {
            HANDLE SystemTokenHandle = nullptr;
            if (::OpenProcessToken(
                SystemProcessHandle,
                TOKEN_DUPLICATE,
                &SystemTokenHandle))
            {
                Result = ::DuplicateTokenEx(
                    SystemTokenHandle,
                    DesiredAccess,
                    nullptr,
                    SecurityIdentification,
                    TokenPrimary,
                    TokenHandle);

                ::CloseHandle(SystemTokenHandle);
            }

            ::CloseHandle(SystemProcessHandle);
        }

        return Result;
    }

    BOOL OpenProcessTokenByProcessId(
        _In_ DWORD ProcessId,
        _In_ DWORD DesiredAccess,
        _Out_ PHANDLE TokenHandle)
    {
        BOOL Result = FALSE;

        HANDLE ProcessHandle = ::OpenProcess(
            PROCESS_QUERY_INFORMATION,
            FALSE,
            ProcessId);
        if (ProcessHandle)
        {
            Result = ::OpenProcessToken(
                ProcessHandle,
                DesiredAccess,
                TokenHandle);

            ::CloseHandle(ProcessHandle);
        }

        return Result;
    }

    BOOL OpenServiceProcessToken(
        _In_ LPCWSTR ServiceName,
        _In_ DWORD DesiredAccess,
        _Out_ PHANDLE TokenHandle)
    {
        BOOL Result = FALSE;

        SERVICE_STATUS_PROCESS ServiceStatus;
        if (::MileStartService(
            ServiceName,
            &ServiceStatus))
        {
            Result = ::OpenProcessTokenByProcessId(
                ServiceStatus.dwProcessId,
                DesiredAccess,
                TokenHandle);
        }

        return Result;
    }

    BOOL AdjustTokenPrivilegesSimple(
        _In_ HANDLE TokenHandle,
        _In_ PLUID_AND_ATTRIBUTES Privileges,
        _In_ DWORD PrivilegeCount)
    {
        BOOL Result = FALSE;

        if (Privileges && PrivilegeCount)
        {
            DWORD PrivilegesSize = sizeof(LUID_AND_ATTRIBUTES) * PrivilegeCount;
            DWORD TokenPrivilegesSize = PrivilegesSize + sizeof(DWORD);

            PTOKEN_PRIVILEGES TokenPrivileges =
                reinterpret_cast<PTOKEN_PRIVILEGES>(
                    ::MileAllocateMemory(TokenPrivilegesSize));
            if (TokenPrivileges)
            {
                TokenPrivileges->PrivilegeCount = PrivilegeCount;
                std::memcpy(
                    TokenPrivileges->Privileges,
                    Privileges,
                    PrivilegesSize);

                ::AdjustTokenPrivileges(
                    TokenHandle,
                    FALSE,
                    TokenPrivileges,
                    TokenPrivilegesSize,
                    nullptr,
                    nullptr);
                Result = (ERROR_SUCCESS == ::GetLastError());

                ::MileFreeMemory(TokenPrivileges);
            }
            else
            {
                ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        else
        {
            ::SetLastError(ERROR_INVALID_PARAMETER);
        }

        return Result;
    }

    BOOL GetTokenInformationWithMemory(
        _In_ HANDLE TokenHandle,
        _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
        _Out_ PVOID* OutputInformation)
    {
        if (!OutputInformation)
        {
            ::SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        *OutputInformation = nullptr;

        BOOL Result = FALSE;

        DWORD Length = 0;
        ::GetTokenInformation(
            TokenHandle,
            TokenInformationClass,
            nullptr,
            0,
            &Length);
        if (ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
        {
            *OutputInformation = ::MileAllocateMemory(Length);
            if (*OutputInformation)
            {
                Result = ::GetTokenInformation(
                    TokenHandle,
                    TokenInformationClass,
                    *OutputInformation,
                    Length,
                    &Length);
                if (!Result)
                {
                    ::MileFreeMemory(*OutputInformation);
                    *OutputInformation = nullptr;
                }
            }
            else
            {
                ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        return Result;
    }

    BOOL AdjustTokenAllPrivileges(
        _In_ HANDLE TokenHandle,
        _In_ DWORD Attributes)
    {
        BOOL Result = FALSE;

        PTOKEN_PRIVILEGES pTokenPrivileges = nullptr;
        if (::GetTokenInformationWithMemory(
            TokenHandle,
            TokenPrivileges,
            reinterpret_cast<PVOID*>(&pTokenPrivileges)))
        {
            for (DWORD i = 0; i < pTokenPrivileges->PrivilegeCount; ++i)
            {
                pTokenPrivileges->Privileges[i].Attributes = Attributes;
            }

            Result = ::AdjustTokenPrivilegesSimple(
                TokenHandle,
                pTokenPrivileges->Privileges,
                pTokenPrivileges->PrivilegeCount);

            ::MileFreeMemory(pTokenPrivileges);
        }

        return Result;
    }

    enum class TargetProcessTokenLevel : std::uint32_t
    {
        Standard = 0,
        System = 1,
        TrustedInstaller = 2,
    };

    BOOL SimpleCreateProcess(
        _In_ TargetProcessTokenLevel TokenLevel,
        _In_ bool Privileged,
        _Inout_ LPWSTR lpCommandLine,
        _In_opt_ LPCWSTR lpCurrentDirectory,
        _In_ LPSTARTUPINFOW lpStartupInfo,
        _Out_ LPPROCESS_INFORMATION lpProcessInformation)
    {
        BOOL Result = FALSE;
        DWORD Error = ERROR_SUCCESS;

        HANDLE CurrentProcessTokenHandle = INVALID_HANDLE_VALUE;
        HANDLE ImpersonatedCurrentProcessTokenHandle = INVALID_HANDLE_VALUE;
        LUID_AND_ATTRIBUTES RawPrivilege;
        DWORD SessionID = static_cast<DWORD>(-1);
        HANDLE SystemTokenHandle = INVALID_HANDLE_VALUE;
        HANDLE ImpersonatedSystemTokenHandle = INVALID_HANDLE_VALUE;
        HANDLE TrustedInstallerTokenHandle = INVALID_HANDLE_VALUE;
        HANDLE TargetTokenHandle = INVALID_HANDLE_VALUE;
        LPVOID EnvironmentBlock = nullptr;

        auto Handler = Mile::ScopeExitTaskHandler([&]()
        {
            if (EnvironmentBlock)
            {
                ::DestroyEnvironmentBlock(EnvironmentBlock);
            }

            if (TargetTokenHandle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(TargetTokenHandle);
            }

            if (TrustedInstallerTokenHandle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(TrustedInstallerTokenHandle);
            }

            if (ImpersonatedSystemTokenHandle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(ImpersonatedSystemTokenHandle);
            }

            if (SystemTokenHandle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(SystemTokenHandle);
            }

            if (ImpersonatedCurrentProcessTokenHandle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(ImpersonatedCurrentProcessTokenHandle);
            }

            if (CurrentProcessTokenHandle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(CurrentProcessTokenHandle);
            }

            ::SetThreadToken(nullptr, nullptr);

            if (!Result)
            {
                ::SetLastError(Error);
            }
        });

        if (!::OpenProcessToken(
            ::GetCurrentProcess(),
            MAXIMUM_ALLOWED,
            &CurrentProcessTokenHandle))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (!::DuplicateTokenEx(
            CurrentProcessTokenHandle,
            MAXIMUM_ALLOWED,
            nullptr,
            SecurityImpersonation,
            TokenImpersonation,
            &ImpersonatedCurrentProcessTokenHandle))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (!::LookupPrivilegeValueW(
            nullptr,
            SE_DEBUG_NAME,
            &RawPrivilege.Luid))
        {
            Error = ::GetLastError();
            return Result;
        }

        RawPrivilege.Attributes = SE_PRIVILEGE_ENABLED;

        if (!::AdjustTokenPrivilegesSimple(
            ImpersonatedCurrentProcessTokenHandle,
            &RawPrivilege,
            1))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (!::SetThreadToken(
            nullptr,
            ImpersonatedCurrentProcessTokenHandle))
        {
            Error = ::GetLastError();
            return Result;
        }

        SessionID = ::GetActiveSessionID();
        if (SessionID == static_cast<DWORD>(-1))
        {
            Error = ERROR_NO_TOKEN;
            return Result;
        }

        if (!::CreateSystemToken(
            MAXIMUM_ALLOWED,
            &SystemTokenHandle))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (!::DuplicateTokenEx(
            SystemTokenHandle,
            MAXIMUM_ALLOWED,
            nullptr,
            SecurityImpersonation,
            TokenImpersonation,
            &ImpersonatedSystemTokenHandle))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (!::AdjustTokenAllPrivileges(
            ImpersonatedSystemTokenHandle,
            SE_PRIVILEGE_ENABLED))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (!::SetThreadToken(
            nullptr,
            ImpersonatedSystemTokenHandle))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (TargetProcessTokenLevel::Standard == TokenLevel)
        {
            if (!::DuplicateTokenEx(
                CurrentProcessTokenHandle,
                MAXIMUM_ALLOWED,
                nullptr,
                SecurityIdentification,
                TokenPrimary,
                &TargetTokenHandle))
            {
                Error = ::GetLastError();
                return Result;
            }
        }
        else if (TargetProcessTokenLevel::System == TokenLevel)
        {
            if (!::DuplicateTokenEx(
                SystemTokenHandle,
                MAXIMUM_ALLOWED,
                nullptr,
                SecurityIdentification,
                TokenPrimary,
                &TargetTokenHandle))
            {
                Error = ::GetLastError();
                return Result;
            }
        }
        else if (TargetProcessTokenLevel::TrustedInstaller == TokenLevel)
        {
            if (!::OpenServiceProcessToken(
                L"TrustedInstaller",
                MAXIMUM_ALLOWED,
                &TrustedInstallerTokenHandle))
            {
                Error = ::GetLastError();
                return Result;
            }

            if (!::DuplicateTokenEx(
                TrustedInstallerTokenHandle,
                MAXIMUM_ALLOWED,
                nullptr,
                SecurityIdentification,
                TokenPrimary,
                &TargetTokenHandle))
            {
                Error = ::GetLastError();
                return Result;
            }
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
            return Result;
        }

        if (!::SetTokenInformation(
            TargetTokenHandle,
            TokenSessionId,
            (PVOID)&SessionID,
            sizeof(DWORD)))
        {
            Error = ::GetLastError();
            return Result;
        }

        if (Privileged)
        {
            if (!::AdjustTokenAllPrivileges(
                TargetTokenHandle,
                SE_PRIVILEGE_ENABLED))
            {
                Error = ::GetLastError();
                return Result;
            }
        }

        if (!::CreateEnvironmentBlock(
            &EnvironmentBlock,
            CurrentProcessTokenHandle,
            TRUE))
        {
            Error = ::GetLastError();
            return Result;
        }

        Result = ::CreateProcessAsUserW(
            TargetTokenHandle,
            nullptr,
            lpCommandLine,
            nullptr,
            nullptr,
            TRUE,
            CREATE_UNICODE_ENVIRONMENT,
            EnvironmentBlock,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);

        return Result;
    }
}

int main()
{
    // Fall back to English in unsupported environment. (Temporary Hack)
    // Reference: https://github.com/M2Team/NSudo/issues/56
    switch (PRIMARYLANGID(::GetThreadUILanguage()))
    {
    case LANG_ENGLISH:
    case LANG_CHINESE:
        break;
    default:
        ::SetThreadUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
        break;
    }

    std::map<std::string, std::wstring> StringDictionary;

    RESOURCE_INFO ResourceInfo = { 0 };
    if (::SimpleLoadResource(
        &ResourceInfo,
        ::GetModuleHandleW(nullptr),
        L"Translations",
        MAKEINTRESOURCEW(IDR_TRANSLATIONS)))
    {
        StringDictionary = ::ParseStringDictionary(std::string_view(
            reinterpret_cast<const char*>(ResourceInfo.Pointer),
            ResourceInfo.Size));
    }

    std::wstring ApplicationName;
    std::map<std::wstring, std::wstring> OptionsAndParameters;
    std::wstring UnresolvedCommandLine;

    ::SplitCommandLineEx(
        std::wstring(GetCommandLineW()),
        std::vector<std::wstring>{ L"-", L"/", L"--" },
        std::vector<std::wstring>{ L"=", L":" },
        ApplicationName,
        OptionsAndParameters,
        UnresolvedCommandLine);

    bool NoLogo = false;
    bool Verbose = false;
    std::wstring WorkDir;
    TargetProcessTokenLevel TargetLevel = TargetProcessTokenLevel::Standard;
    bool Privileged = false;

    for (auto& OptionAndParameter : OptionsAndParameters)
    {
        if (0 == _wcsicmp(
            OptionAndParameter.first.c_str(),
            L"NoLogo"))
        {
            NoLogo = true;
        }
        else if (0 == _wcsicmp(
            OptionAndParameter.first.c_str(),
            L"Verbose"))
        {
            Verbose = true;
        }
        else if (0 == _wcsicmp(
            OptionAndParameter.first.c_str(),
            L"WorkDir"))
        {
            WorkDir = OptionAndParameter.second;
        }
        else if (0 == _wcsicmp(
            OptionAndParameter.first.c_str(),
            L"System"))
        {
            TargetLevel = TargetProcessTokenLevel::System;
        }
        else if (0 == _wcsicmp(
            OptionAndParameter.first.c_str(),
            L"TrustedInstaller"))
        {
            TargetLevel = TargetProcessTokenLevel::TrustedInstaller;
        }
        else if (0 == _wcsicmp(
            OptionAndParameter.first.c_str(),
            L"Privileged"))
        {
            Privileged = true;
        }
    }

    bool ShowHelp = false;
    bool ShowInvalidCommandLine = false;

    if (1 == OptionsAndParameters.size() && UnresolvedCommandLine.empty())
    {
        auto OptionAndParameter = *OptionsAndParameters.begin();

        if (0 == _wcsicmp(OptionAndParameter.first.c_str(), L"?") ||
            0 == _wcsicmp(OptionAndParameter.first.c_str(), L"H") ||
            0 == _wcsicmp(OptionAndParameter.first.c_str(), L"Help"))
        {
            ShowHelp = true;
        }
        else if (0 == _wcsicmp(OptionAndParameter.first.c_str(), L"Version"))
        {
            ::WriteToConsole(
                L"MinSudo " MILE_PROJECT_VERSION_STRING L" (Build "
                MILE_PROJECT_MACRO_TO_STRING(MILE_PROJECT_VERSION_BUILD) L")"
                L"\r\n");
            return 0;
        }
        else if (!(
            NoLogo ||
            Verbose ||
            !WorkDir.empty() ||
            TargetLevel != TargetProcessTokenLevel::Standard ||
            Privileged))
        {
            ShowInvalidCommandLine = true;
        }
    }

    if (!NoLogo)
    {
        ::WriteToConsole(
            L"MinSudo " MILE_PROJECT_VERSION_STRING L" (Build "
            MILE_PROJECT_MACRO_TO_STRING(MILE_PROJECT_VERSION_BUILD) L")" L"\r\n"
            L"(c) M2-Team and Contributors. All rights reserved.\r\n"
            L"\r\n");
    }

    if (ShowHelp)
    {
        ::WriteToConsole(StringDictionary["CommandLineHelp"]);

        return 0;
    }
    else if (ShowInvalidCommandLine)
    {
        ::WriteToConsole(StringDictionary["InvalidCommandLineError"]);

        return E_INVALIDARG;
    }

    ApplicationName = ::GetCurrentProcessModulePath();
    if (UnresolvedCommandLine.empty())
    {
        UnresolvedCommandLine = L"cmd.exe";
    }

    if (Verbose)
    {
        std::wstring VerboseInformation;
        VerboseInformation += StringDictionary["CommandLineNotice"];
        VerboseInformation += UnresolvedCommandLine;
        VerboseInformation += L"\r\n";
        ::WriteToConsole(VerboseInformation.c_str());
    }

    if (WorkDir.empty())
    {
        WorkDir = std::wstring(::GetWorkingDirectory());
    }
    if (L'\\' == WorkDir.back())
    {
        WorkDir.pop_back();
    }

    if (::MileIsCurrentProcessElevated())
    {
        ::FreeConsole();
        ::AttachConsole(ATTACH_PARENT_PROCESS);

        if (Verbose)
        {
            ::WriteToConsole(StringDictionary["Stage1Notice"]);
        }

        STARTUPINFOW StartupInfo = { 0 };
        PROCESS_INFORMATION ProcessInformation = { 0 };
        StartupInfo.cb = sizeof(STARTUPINFOW);
        if (::SimpleCreateProcess(
            TargetLevel,
            Privileged,
            const_cast<LPWSTR>(UnresolvedCommandLine.c_str()),
            WorkDir.c_str(),
            &StartupInfo,
            &ProcessInformation))
        {
            // Make sure ignores CTRL+C signals after creating the child
            // process. Because that state is heritable, but we want to make
            // child process support CTRL+C.
            ::SetConsoleCtrlHandler(nullptr, TRUE);

            ::CloseHandle(ProcessInformation.hThread);
            ::WaitForSingleObjectEx(ProcessInformation.hProcess, INFINITE, FALSE);
            ::CloseHandle(ProcessInformation.hProcess);
        }
        else
        {
            ::WriteToConsole(StringDictionary["Stage1Failed"]);
        }
    }
    else
    {
        if (Verbose)
        {
            ::WriteToConsole(StringDictionary["Stage0Notice"]);
        }

        std::wstring TargetCommandLine = L"--NoLogo ";
        if (Verbose)
        {
            TargetCommandLine += L"--Verbose ";
        }
        TargetCommandLine += L"--WorkDir=\"";
        TargetCommandLine += WorkDir;
        TargetCommandLine += L"\" ";
        if (TargetLevel == TargetProcessTokenLevel::System)
        {
            TargetCommandLine += L"--System ";
        }
        else if (TargetLevel == TargetProcessTokenLevel::TrustedInstaller)
        {
            TargetCommandLine += L"--TrustedInstaller ";
        }
        if (Privileged)
        {
            TargetCommandLine += L"--Privileged ";
        }
        TargetCommandLine += UnresolvedCommandLine;

        SHELLEXECUTEINFOW Information = { 0 };
        Information.cbSize = sizeof(SHELLEXECUTEINFOW);
        Information.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
        Information.lpVerb = L"runas";
        Information.lpFile = ApplicationName.c_str();
        Information.lpParameters = TargetCommandLine.c_str();
        if (::ShellExecuteExW(&Information))
        {
            // Make sure ignores CTRL+C signals after creating the child
            // process. Because that state is heritable, but we want to make
            // child process support CTRL+C.
            ::SetConsoleCtrlHandler(nullptr, TRUE);

            ::WaitForSingleObjectEx(Information.hProcess, INFINITE, FALSE);
            ::CloseHandle(Information.hProcess);
        }
        else
        {
            ::WriteToConsole(StringDictionary["Stage0Failed"]);
        }
    }

    return 0;
}
