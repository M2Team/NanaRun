/*
 * PROJECT:   NanaRun
 * FILE:      SynthRdp.cpp
 * PURPOSE:   Implementation for Hyper-V Enhanced Session Proxy Service
 *
 * LICENSE:   The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#include <cstdint>

#include <cstdio>
#include <cwchar>

int main()
{
    std::wprintf(
        L"Hyper-V VMBus Enhanced Session Service\n"
        L"================================================================\n"
        L"Hello World!\n");

    std::getchar();

    return 0;
}
