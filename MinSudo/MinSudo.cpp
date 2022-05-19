/*
 * PROJECT:   NanaRun
 * FILE:      MinSudo.cpp
 * PURPOSE:   Implementation for MinSudo
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include <Windows.h>

#include <cstdint>
#include <cwchar>

#include <map>
#include <string>
#include <vector>

#include "Mile.Project.Properties.h"

namespace
{
    std::vector<std::wstring> SpiltCommandLine(
        std::wstring const& CommandLine)
    {
        // Initialize the SplitArguments.
        std::vector<std::wstring> SplitArguments;

        wchar_t c = L'\0';
        int copy_character;                   /* 1 = copy char to *args */
        unsigned numslash;              /* num of backslashes seen */

        std::wstring Buffer;
        Buffer.reserve(CommandLine.size());

        /* first scan the program name, copy it, and count the bytes */
        wchar_t* p = const_cast<wchar_t*>(CommandLine.c_str());

        // A quoted program name is handled here. The handling is much simpler than
        // for other arguments. Basically, whatever lies between the leading
        // double-quote and next one, or a terminal null character is simply
        // accepted. Fancier handling is not required because the program name must
        // be a legal NTFS/HPFS file name. Note that the double-quote characters
        // are not copied, nor do they contribute to character_count.
        bool InQuotes = false;
        do
        {
            if (*p == L'"')
            {
                InQuotes = !InQuotes;
                c = *p++;
                continue;
            }

            // Copy character into argument:
            Buffer.push_back(*p);

            c = *p++;
        } while (c != L'\0' && (InQuotes || (c != L' ' && c != L'\t')));

        if (c == L'\0')
        {
            p--;
        }
        else
        {
            Buffer.resize(Buffer.size() - 1);
        }

        // Save te argument.
        SplitArguments.push_back(Buffer);

        InQuotes = false;

        // Loop on each argument
        for (;;)
        {
            if (*p)
            {
                while (*p == L' ' || *p == L'\t')
                    ++p;
            }

            // End of arguments
            if (*p == L'\0')
                break;

            // Initialize the argument buffer.
            Buffer.clear();

            // Loop through scanning one argument:
            for (;;)
            {
                copy_character = 1;

                // Rules: 2N backslashes + " ==> N backslashes and begin/end quote
                // 2N + 1 backslashes + " ==> N backslashes + literal " N
                // backslashes ==> N backslashes
                numslash = 0;

                while (*p == L'\\')
                {
                    // Count number of backslashes for use below
                    ++p;
                    ++numslash;
                }

                if (*p == L'"')
                {
                    // if 2N backslashes before, start/end quote, otherwise copy
                    // literally:
                    if (numslash % 2 == 0)
                    {
                        if (InQuotes && p[1] == L'"')
                        {
                            p++; // Double quote inside quoted string
                        }
                        else
                        {
                            // Skip first quote char and copy second:
                            copy_character = 0; // Don't copy quote
                            InQuotes = !InQuotes;
                        }
                    }

                    numslash /= 2;
                }

                // Copy slashes:
                while (numslash--)
                {
                    Buffer.push_back(L'\\');
                }

                // If at end of arg, break loop:
                if (*p == L'\0' || (!InQuotes && (*p == L' ' || *p == L'\t')))
                    break;

                // Copy character into argument:
                if (copy_character)
                {
                    Buffer.push_back(*p);
                }

                ++p;
            }

            // Save te argument.
            SplitArguments.push_back(Buffer);
        }

        return SplitArguments;
    }

    void SpiltCommandLineEx(
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
        for (auto& SplitArgument : ::SpiltCommandLine(CommandLine))
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

    bool IsCurrentProcessElevated()
    {
        bool Result = false;

        HANDLE CurrentProcessAccessToken = nullptr;
        if (::OpenProcessToken(
            ::GetCurrentProcess(),
            TOKEN_ALL_ACCESS,
            &CurrentProcessAccessToken))
        {
            TOKEN_ELEVATION Information = { 0 };
            DWORD Length = sizeof(Information);
            if (::GetTokenInformation(
                CurrentProcessAccessToken,
                TOKEN_INFORMATION_CLASS::TokenElevation,
                &Information,
                Length,
                &Length))
            {
                Result = Information.TokenIsElevated;
            }

            ::CloseHandle(CurrentProcessAccessToken);
        }

        return Result;
    }

    std::string ToConsoleString(
        std::wstring const& Utf16String)
    {
        std::string ConsoleString;

        UINT CurrentCodePage = ::GetConsoleOutputCP();

        int ConsoleStringLength = ::WideCharToMultiByte(
            CurrentCodePage,
            0,
            Utf16String.data(),
            static_cast<int>(Utf16String.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
        if (ConsoleStringLength > 0)
        {
            ConsoleString.resize(ConsoleStringLength);
            ConsoleStringLength = ::WideCharToMultiByte(
                CurrentCodePage,
                0,
                Utf16String.data(),
                static_cast<int>(Utf16String.size()),
                &ConsoleString[0],
                ConsoleStringLength,
                nullptr,
                nullptr);
            ConsoleString.resize(ConsoleStringLength);
        }

        return ConsoleString;
    }

    void WriteToConsole(
        std::wstring const& String)
    {
        HANDLE ConsoleOutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);;

        DWORD NumberOfCharsWritten = 0;
        if (!::WriteConsoleW(
            ConsoleOutputHandle,
            String.c_str(),
            static_cast<DWORD>(String.size()),
            &NumberOfCharsWritten,
            nullptr))
        {
            std::string CurrentCodePageString = ::ToConsoleString(String);

            ::WriteFile(
                ConsoleOutputHandle,
                CurrentCodePageString.c_str(),
                static_cast<DWORD>(CurrentCodePageString.size()),
                &NumberOfCharsWritten,
                nullptr);
        }
    }
}

int main()
{
    std::wstring ApplicationName;
    std::map<std::wstring, std::wstring> OptionsAndParameters;
    std::wstring UnresolvedCommandLine;

    ::SpiltCommandLineEx(
        std::wstring(GetCommandLineW()),
        std::vector<std::wstring>{ L"-", L"/", L"--" },
        std::vector<std::wstring>{ L"=", L":" },
        ApplicationName,
        OptionsAndParameters,
        UnresolvedCommandLine);

    bool NoLogo = false;
    bool Verbose = false;

    for (auto& OptionAndParameter : OptionsAndParameters)
    {
        if (0 == _wcsicmp(OptionAndParameter.first.c_str(), L"NoLogo"))
        {
            NoLogo = true;
        }
        else if (0 == _wcsicmp(OptionAndParameter.first.c_str(), L"Verbose"))
        {
            Verbose = true;
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
        else if (!(NoLogo || Verbose))
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
        ::WriteToConsole(
            L"Format: MinSudo [ Options ] Command" L"\r\n"
            L"\r\n"
            L"Options:" L"\r\n"
            L"\r\n"
            L"--NoLogo Suppress copyright message."  L"\r\n"
            L"--Verbose Show detailed information."  L"\r\n"
            L"\r\n"
            L"--Version Show version information."  L"\r\n"
            L"\r\n"
            L"/? Show this content."  L"\r\n"
            L"-H Show this content."  L"\r\n"
            L"--Help Show this content."  L"\r\n"
            L"\r\n"
            L"Notes:" L"\r\n"
            L"    All command options are case-insensitive." L"\r\n"
            L"    You can use the \"/\" or \"--\" override \"-\" and use "
            L"the \"=\" override \":\" in \r\n    the command line "
            L"parameters. For example, \"/Option:Value\" and \r\n    "
            L"\"-Option=Value\" are equivalent." L"\r\n"
            L"\r\n");

        return 0;
    }
    else if (ShowInvalidCommandLine)
    {
        ::WriteToConsole(
            L"[Error] Invalid command line parameters."
            L" (Use '/?', '-H' or '--Help' option for usage.)"
            L"\r\n");

        return E_INVALIDARG;
    }

    ApplicationName = GetCurrentProcessModulePath();
    if (UnresolvedCommandLine.empty())
    {
        UnresolvedCommandLine = L"cmd.exe";
    }

    if (Verbose)
    {
        std::wstring VerboseInformation;
        VerboseInformation += L"[Info] Target Command Line: ";
        VerboseInformation += UnresolvedCommandLine;
        VerboseInformation += L"\r\n";
        ::WriteToConsole(VerboseInformation.c_str());
    }

    if (::IsCurrentProcessElevated())
    {
        ::FreeConsole();
        ::AttachConsole(ATTACH_PARENT_PROCESS);

        if (Verbose)
        {
            ::WriteToConsole(
                L"[Info] Enter the Stage 1 (elevated)." L"\r\n"
                L"\r\n");
        }

        STARTUPINFOW StartupInfo = { 0 };
        PROCESS_INFORMATION ProcessInformation = { 0 };
        StartupInfo.cb = sizeof(STARTUPINFOW);
        StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
        StartupInfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
        StartupInfo.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
        StartupInfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
        if (::CreateProcessW(
            nullptr,
            const_cast<LPWSTR>(UnresolvedCommandLine.c_str()),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            nullptr,
            &StartupInfo,
            &ProcessInformation))
        {
            ::CloseHandle(ProcessInformation.hThread);
            ::WaitForSingleObjectEx(ProcessInformation.hProcess, INFINITE, FALSE);
            ::CloseHandle(ProcessInformation.hProcess);
        }
        else
        {
            ::WriteToConsole(
                L"[Error] CreateProcessW failed."
                L"\r\n");
        }
    }
    else
    {
        if (Verbose)
        {
            ::WriteToConsole(
                L"[Info] Enter the Stage 0 (non-elevated)."
                L"\r\n");
        }

        std::wstring TargetCommandLine = L"--NoLogo ";
        if (Verbose)
        {
            TargetCommandLine += L"--Verbose ";
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
            ::WaitForSingleObjectEx(Information.hProcess, INFINITE, FALSE);
            ::CloseHandle(Information.hProcess);
        }
        else
        {
            ::WriteToConsole(
                L"[Error] ShellExecuteExW failed."
                L"\r\n");
        }
    }

    return 0;
}
