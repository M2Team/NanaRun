/*
 * PROJECT:    SynthRdp
 * FILE:       VirtualSmb.cpp
 * PURPOSE:    Implementation for Hyper-V Virtual SMB Guest Utility
 *
 * LICENSE:    The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#include <clocale>
#include <cstdio>
#include <cwchar>

int main()
{
    // I can't use that because of the limitation in VC-LTL.
    // std::setlocale(LC_ALL, "zh_CN.UTF-8");

    std::setlocale(LC_ALL, "chs");

    std::wprintf(
        L"VirtualSmb\n"
        L"================================================================\n"
        L"The F@cking MSVC 2019 toolset ruined our $1M project!\n");

    return 0;
}
