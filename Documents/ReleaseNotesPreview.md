# NanaRun Preview Release Notes

For stable versions, please read [NanaRun Release Notes](ReleaseNotes.md).

**NanaRun 1.0 Preview 3 (1.0.92.0)**

- Introduce the SynthRdp tool.
- Add short form command line options support for MinSudo. (Suggested by
  he3als.)
- Update icon assets. (Designed by Shomnipotence.)
- Adjust several implementations.

**NanaRun 1.0 Preview 2 (1.0.18.0)**

- Remove standard handle settings because child process will inherit the 
  parent's console for MinSudo.
- Update application icon. (Designed by Shomnipotence.)
- Make sure ignores CTRL+C signals for MinSudo itself to solve unexcepted
  behaviors.
- Fix current directory issue when put MinSudo into System32 folder. (Thanks to
  Slemoon.)
- Add System and TrustedInstaller mode for MinSudo.
- Add enable all privileges mode support for MinSudo.

**NanaRun 1.0 Preview 1 (1.0.5.0)**

- Implement MinSudo.
