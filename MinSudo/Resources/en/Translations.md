```
* PROJECT:   NanaRun
* FILE:      Translations.md
* PURPOSE:   The English translation for MinSudo
*
* LICENSE:   The MIT License
*
* DEVELOPER: MouriNaruto (KurikoMouri@outlook.jp)
```

- InvalidCommandLineError
```
[Error] Invalid command line parameters. (Use '/?', '-H' or '--Help' option for
usage.)

```
- CommandLineNotice
```
[Info] Target Command Line: 
```
- Stage0Notice
```
[Info] Enter the Stage 0 (non-elevated).

```
- Stage0Failed
```
[Error] ShellExecuteExW failed.

```
- Stage1Notice
```
[Info] Enter the Stage 1 (elevated).


```
- Stage1Failed
```
[Error] CreateProcessW failed.

```
- CommandLineHelp
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
