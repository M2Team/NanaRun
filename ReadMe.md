# ![NanaRun](Assets/NanaRun.png) NanaRun

Application runtime environment customization utility

## Development Status of Components

- [x] MinSudo
- [ ] NanaEAM
- [ ] NanaKit
- [ ] NanaRun (Windows)
- [ ] NanaRun (Console)
- [ ] NanaRun (SDK)

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
