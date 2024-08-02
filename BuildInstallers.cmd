@setlocal
@echo off

rem Change to the current folder.
cd "%~dp0"

rem Remove the SynthRdp installation image output folder for a fresh compile.
rd /s /q Output\SynthRdpInstallationImage

rem Generate SynthRdp installer
Tools\ISCC.exe SynthRdp\SynthRdp.iss

rem Copy SynthRdp AutoRun.inf
copy SynthRdp\AutoRun.inf Output\SynthRdpInstallationImage

rem Generate SynthRdp installation image
Tools\oscdimg.exe -u1 -udfver102 -lSynthRdp Output\SynthRdpInstallationImage Output\SynthRdp.iso

@endlocal