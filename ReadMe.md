# ![NanaRun](Assets/NanaRun.png) NanaRun

Application runtime environment customization utility

## Development Status of Components

- [x] MinSudo
- [x] SynthRdp
- [ ] NanaEAM
- [ ] NanaKit
- [ ] NanaRun (Console)
- [ ] NanaRun (SDK)

## System Requirements

- MinSudo
  - Supported OS: Windows Vista RTM (Build 6000.16386) or later
  - Supported Platforms: x86 (32-bit and 64-bit) and ARM (64-bit)

- SynthRdp
  - Supported OS: Windows XP RTM (Build 2600) or later
  - Supported Platforms: x86 (32-bit and 64-bit) and ARM (64-bit)

## MinSudo

MinSudo is a lightweight POSIX-style Sudo implementation for Windows.

It makes user possible to use elevated console apps in non-elevated consoles.

For safety, the implementation uses the UAC for elevation and don't support
credential cache. It also don't use homemade Windows service and any IPC 
infrastructures.

Here is the usage.

```
Format: MinSudo [Options] Command

Options:

  --NoLogo, -NoL
    Suppress copyright message.

  --Verbose, -V
    Show detailed information.

  --WorkDir=[Path], -WD=[Path]
    Set working directory.

  --System, -S
    Run as System instead of Administrator.

  --TrustedInstaller, -TI
    Run as TrustedInstaller instead of Administrator.

  --Privileged, -P
    Enable all privileges.

  --Version, -Ver
    Show version information.

  /?, -H, --Help
    Show this content.

Notes:

  - All command options are case-insensitive.
  - MinSudo will execute "cmd.exe" if you don't specify another command.
  - You can use the "/" or "--" override "-" and use the "=" override ":" in 
    the command line parameters. For example, "/Option:Value" and 
    "-Option=Value" are equivalent.

Example:

  If you want to run "whoami /all" as elevated in the non-elevated Console, and
  you don't want to show version information of MinSudo.
  > MinSudo --NoLogo whoami /all
```

## SynthRdp

SynthRdp is a Hyper-V Enhanced Session Proxy Service for Windows, which can
redirect RDP over VMBus pipe to any RDP over TCP socket, because the Hyper-V
Enhanced Session is actually the RDP connection over the VMBus pipe.

### Features

- Make Hyper-V virtual machines with Windows 8 and earlier guest OSes support
  Hyper-V Enhanced Session mode.
- Act as a proxy service to make Hyper-V virtual machines with non-Windows
  guest OSes support Hyper-V Enhanced Session mode.

### Usage (Quick Start)

- Move SynthRdp.exe to "%SystemDrive%\Windows\System32" folder in your virtual
  machine which needs to use SynthRdp. The platform of SynthRdp.exe needs to be
  the same as the virtual machine guest OS.
- Open the Command Prompt as Administrator and execute the following commands.
  ```
  SynthRdp Install
  SynthRdp Config Set DisableRemoteDesktop False
  SynthRdp Config Set EnableUserAuthentication False
  SynthRdp Config Set DisableBlankPassword False
  ```

### Usage (Detailed)

```
Format: SynthRdp [Command] <Option1> <Option2> ...

Commands:

  Help - Show this content.

  Install - Install SynthRdp service.
  Uninstall - Uninstall SynthRdp service.
  Start - Start SynthRdp service.
  Stop - Stop SynthRdp service.

  Config List - List all configurations related to SynthRdp.
  Config Set [Key] <Value> - Set the specific configuration
                             with the specific value, or reset
                             the specific configuration if you
                             don't specify the value.

Configuration Keys:

  DisableRemoteDesktop <True|False>
    Set False to enable the remote desktop for this virtual
    machine. The default setting is True. Remote Desktop is
    necessary for the Hyper-V Enhanced Session because it is
    actually the RDP connection over the VMBus pipe.
  EnableUserAuthentication <True|False>
    Set False to allow connections without Network Level
    Authentication. The default setting is True. Set this
    configuration option False is necessary for using the
    Hyper-V Enhanced Session via the SynthRdp service.
  DisableBlankPassword <True|False>
    Set False to enable the usage of logging on this virtual
    machine as the account with the blank password via remote
    desktop. The default setting is True. Set this
    configuration option False will make you use the Hyper-V
    Enhanced Session via the SynthRdp service happier, but
    compromise the security.
  OverrideSystemImplementation <True|False>
    Set True to use the Hyper-V Enhanced Session via the
    SynthRdp service on Windows 8.1 / Server 2012 R2 or later
    guests which have the built-in Hyper-V Enhanced Session
    support for this virtual machine. The default setting is
    False. You can set this configuration option True if you
    want to use your current virtual machine as the Hyper-V
    Enhanced Session proxy server. You need to reboot your
    virtual machine for applying this configuration option
    change.
  ServerHost <Host>
    Set the server host for the remote desktop connection you
    want to use in the Hyper-V Enhanced Session. The default
    setting is 127.0.0.1.
  ServerPort <Port>
    Set the server port for the remote desktop connection you
    want to use in the Hyper-V Enhanced Session. The default
    setting is 3389.

Notes:
  - All command options are case-insensitive.
  - SynthRdp will run as a console application instead of
    service if you don't specify another command.

Examples:

  SynthRdp Install
  SynthRdp Uninstall
  SynthRdp Start
  SynthRdp Stop

  SynthRdp Config List

  SynthRdp Config Set DisableRemoteDesktop False
  SynthRdp Config Set EnableUserAuthentication False
  SynthRdp Config Set DisableBlankPassword False
  SynthRdp Config Set OverrideSystemImplementation False
  SynthRdp Config Set ServerHost 127.0.0.1
  SynthRdp Config Set ServerPort 3389

  SynthRdp Config Set DisableRemoteDesktop
  SynthRdp Config Set EnableUserAuthentication
  SynthRdp Config Set DisableBlankPassword
  SynthRdp Config Set OverrideSystemImplementation
  SynthRdp Config Set ServerHost
  SynthRdp Config Set ServerPort
```

## Suggestions

- Users should use the SynthRdp to connect to Windows Vista or later guests for
  decent user experiences.
