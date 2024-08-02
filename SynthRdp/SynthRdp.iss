; -- 64BitThreeArch.iss --
; Demonstrates how to install a program built for three different
; architectures (x86, x64, Arm64) using a single installer.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=Hyper-V Enhanced Session Proxy Service (SynthRdp)
AppVersion=1.0.92.0
AppPublisher=M2-Team
AppPublisherURL=https://github.com/M2Team/NanaRun
WizardStyle=modern
DefaultDirName={autopf}\M2-Team\NanaRun
DefaultGroupName=NanaRun
UninstallDisplayIcon={app}\SynthRdp.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\Output\SynthRdpInstallationImage
; "ArchitecturesInstallIn64BitMode=x64compatible or arm64" instructs
; Setup to use "64-bit install mode" on x64-compatible systems and
; Arm64 systems, meaning Setup should use the native 64-bit Program
; Files directory and the 64-bit view of the registry. On all other
; OS architectures (e.g., 32-bit x86), Setup will use "32-bit
; install mode".
ArchitecturesInstallIn64BitMode=x64compatible or arm64
SetupLogging=yes
OutputBaseFilename=SynthRdpInstaller

[Files]
; In order of preference, we want to install:
; - Arm64 binaries on Arm64 systems
; - else, x64 binaries on x64-compatible systems
; - else, x86 binaries

; Place all Arm64-specific files here, using 'Check: PreferArm64Files' on each entry.
Source: "..\Output\Binaries\Release\ARM64\SynthRdp.exe"; DestDir: "{sysnative}"; DestName: "SynthRdp.exe"; Check: PreferArm64Files

; Place all x64-specific files here, using 'Check: PreferX64Files' on each entry.
; Only the first entry should include the 'solidbreak' flag.
Source: "..\Output\Binaries\Release\x64\SynthRdp.exe"; DestDir: "{sysnative}"; DestName: "SynthRdp.exe"; Check: PreferX64Files; Flags: solidbreak

; Place all x86-specific files here, using 'Check: PreferX86Files' on each entry.
; Only the first entry should include the 'solidbreak' flag.
Source: "..\Output\Binaries\Release\Win32\SynthRdp.exe"; DestDir: "{sysnative}"; DestName: "SynthRdp.exe"; Check: PreferX86Files; Flags: solidbreak

; Place all common files here.
; Only the first entry should include the 'solidbreak' flag.
;Source: "MyProg.chm"; DestDir: "{app}"; Flags: solidbreak
;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Code]

function PreferArm64Files: Boolean;
begin
  Result := IsArm64;
end;

function PreferX64Files: Boolean;
begin
  Result := not PreferArm64Files and IsX64Compatible;
end;

function PreferX86Files: Boolean;
begin
  Result := not PreferArm64Files and not PreferX64Files;
end;

[Run]

Filename: "{sysnative}\SynthRdp.exe"; Parameters: "Uninstall"; Flags: runhidden logoutput
Filename: "{sysnative}\SynthRdp.exe"; Parameters: "Install"; Flags: runhidden logoutput
Filename: "{sysnative}\SynthRdp.exe"; Parameters: "Config Set DisableRemoteDesktop False"; Flags: runhidden logoutput
Filename: "{sysnative}\SynthRdp.exe"; Parameters: "Config Set EnableUserAuthentication False"; Flags: runhidden logoutput
Filename: "{sysnative}\SynthRdp.exe"; Parameters: "Config Set DisableBlankPassword False"; Flags: runhidden logoutput

[UninstallRun]

Filename: "{sysnative}\SynthRdp.exe"; Parameters: "Uninstall"; Flags: runhidden logoutput; RunOnceId: RemoveService