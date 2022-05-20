##
## PROJECT:   NanaRun
## FILE:      Translations.md
## PURPOSE:   The English translation for MinSudo
## 
## LICENSE:   The MIT License
## 
## DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
##

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

--Version Show version information.

/? Show this content.
-H Show this content.
--Help Show this content.

Notes:
    All command options are case-insensitive.
    You can use the "/" or "--" override "-" and use the "=" override ":" in
    the command line parameters. For example, "/Option:Value" and
    "-Option=Value" are equivalent.

```
