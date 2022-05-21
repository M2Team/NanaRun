```
* PROJECT:   NanaRun
* FILE:      Translations.md
* PURPOSE:   The Chinese (Simplified) translation for MinSudo
*
* LICENSE:   The MIT License
*
* DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
```

- InvalidCommandLineError
```
[错误] 无效的命令行参数。 (使用 '/?', '-H' 或 '--Help' 选项获取用法。)

```
- CommandLineNotice
```
[信息] 目标命令行: 
```
- Stage0Notice
```
[信息] 进入 0 阶段 (未提权).

```
- Stage0Failed
```
[信息] ShellExecuteExW 调用失败。

```
- Stage1Notice
```
[信息] 进入 1 阶段 (已提权).


```
- Stage1Failed
```
[错误] CreateProcessW 调用失败。

```
- CommandLineHelp
```
格式: MinSudo [ 选项 ] 命令

选项:

  --NoLogo 隐藏版权信息。
  --Verbose 显示详细信息。

  --Version 显示版本信息。

  /? 显示该内容。
  -H 显示该内容。
  --Help 显示该内容。

提示:

  - 所有命令选项不区分大小写。
  - 如果你不指定其他命令，MinSudo 将执行 "cmd.exe"。
  - 可以在命令行参数中使用 "/" 或 "--" 代替 "-" 和使用 "=" 代替 ":"。例如
    "/Option:Value" 和 "-Option=Value" 是等价的。

用例:

  如果你想在未提权的控制台下运行提权的 "whoami /all"，并且你不希望 MinSudo 显示
  版本信息。
  > MinSudo --NoLogo whoami /all

```
