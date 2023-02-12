/*
 * PROJECT:   NanaRun
 * FILE:      NanaRunConsole.cpp
 * PURPOSE:   Implementation for NanaRun (Console)
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include <cstdint>
#include <cwchar>
#include <string>
#include <vector>

enum class AccessTokenSourceType : std::int32_t
{
    CurrentProcess = 0,
    Process = 1,
    System = 2,
    CurrentSession = 3,
    Session = 4,
    Service = 5,
    User = 6,
};

enum class MandatoryLabelType : std::int32_t
{
    Default = 0,
    Untrusted = 1,
    Low = 2,
    Medium = 3,
    MediumPlus = 4,
    High = 5,
    System = 6,
    ProtectedProcess = 7,
};

enum class ProcessPriorityType : std::int32_t
{
    Default = 0,
    Idle = 1,
    BelowNormal = 2,
    Normal = 3,
    AboveNormal = 4,
    High = 5,
    RealTime = 6,
};

enum class ShowWindowModeType : std::int32_t
{
    Default = 0,
    Show = 1,
    Hide = 2,
    Maximize = 3,
    Minimize = 4
};

struct EnvironmentConfiguration
{
    AccessTokenSourceType AccessTokenSource =
        AccessTokenSourceType::CurrentProcess;
    std::uint32_t Process = 0;
    std::uint32_t Session = 0;
    std::string ServiceName;
    std::string UserName;
    std::string UserPassword;

    bool UseLinkedAccessToken = false;
    bool UseLuaAccessToken = false;

    bool EnableAllPrivileges = false;
    bool RemoveAllPrivileges = false;
    std::vector<std::string> ExcludedPrivileges;
    std::vector<std::string> IncludedPrivileges;

    MandatoryLabelType IntegrityLevel = MandatoryLabelType::Default;

    bool InheritEnvironmentVariables = true;
    std::vector<std::pair<std::string, std::string>> EnvironmentVariables;

    ProcessPriorityType ProcessPriority = ProcessPriorityType::Default;

    ShowWindowModeType ShowWindowMode = ShowWindowModeType::Default;

    bool WaitForExit = false;

    std::string CurrentDirectory;

    bool UseCurrentConsole = false;
};

int main()
{
    std::wprintf(L"Hello World! - NanaRun (Console)");

    return 0;
}
