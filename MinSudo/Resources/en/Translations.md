```
* PROJECT:   NanaRun
* FILE:      Translations.md
* PURPOSE:   The English translation for MinSudo
*
* LICENSE:   The MIT License
*
* DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
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
Format: MinSudo [ Options ] Command

Options:

  --NoLogo Suppress copyright message.
  --Verbose Show detailed information.
  --WorkDir=[ Path ] Set working directory.
  --System Run as System instead of Administrator.
  --TrustedInstaller Run as TrustedInstaller instead of Administrator.
  --Privileged Enable all privileges.

  --Version Show version information.

  /? Show this content.
  -H Show this content.
  --Help Show this content.

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
