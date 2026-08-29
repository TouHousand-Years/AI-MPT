# Establish the smallest local build-and-run loop

Parent: ../map.md
Type: task
Status: resolved
Blocked by: 12, 20

## Question

What is the smallest existing x64 OpenMPT configuration that can be built,
started, and debugged on the owner's current machine for the chosen vertical
slice, and what concise local prerequisites and commands let later prototypes
repeat that loop without requiring pinned toolchains, clean runners, CI, every
helper architecture, or parity with OpenMPT's original development setup?

## Comments

This task gathers only the practical foothold needed to make the next design
prototype concrete. It does not establish a distribution or reproducibility
contract.

### Work log — 2026-08-29

The first candidate, `build/vs2022win11/OpenMPT.vcxproj`, built successfully but
was rejected at startup with “Your system does not meet the minimum
requirements for this variant of OpenMPT.” The project defines the Windows 11
24H2 minimum target, while the owner's machine is Windows 11 23H2 build 22631.
The failure is therefore a target-selection error rather than a missing runtime
or CPU feature.

The corrected candidate is `build/vs2022win10/OpenMPT.vcxproj` at `Debug|x64`,
retargeted at build time from its unavailable Windows SDK 22621 to the installed
SDK 26100. It writes the application to
`bin/debug/vs2022-win10-static/amd64/OpenMPT.exe`. Building this project avoids
the unrelated `all.sln` surface and other helper architectures; its project
references still bring in the libraries the desktop application needs.

## Answer

### Smallest verified foothold

A full Visual Studio IDE is not required. The verified local loop uses the
owner's existing VS Code and Microsoft C/C++ extension together with Visual
Studio Build Tools 2022. The installed Build Tools components needed by this
snapshot are MSBuild, the v143 x64/x86 C++ tools, x64/x86 MFC and ATL, Windows
SDK 10.0.26100, and the corresponding Spectre-mitigated x64/x86 libraries,
including MFC. OpenMPT's desktop target uses static MFC, so changing editors
cannot remove these native build dependencies.

From the repository root, build only the desktop project:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' `
  'openmpt-original_ref\build\vs2022win10\OpenMPT.vcxproj' `
  /m /nr:false /t:Build `
  /p:Configuration=Debug /p:Platform=x64 `
  /p:WindowsTargetPlatformVersion=10.0.26100.0 /v:minimal
```

Run the resulting executable directly:

```powershell
& 'openmpt-original_ref\bin\debug\vs2022-win10-static\amd64\OpenMPT.exe'
```

The local `.vscode/tasks.json` and `.vscode/launch.json` encode the same build
and a `cppvsdbg` launch with `stopAtEntry: true`. In VS Code, `Ctrl+Shift+B`
builds the target and `F5` builds, launches, and stops at the entry point.

### Verification

- MSBuild completed `Debug|x64` successfully with node reuse disabled.
- A direct launch produced the responsive main window
  `OpenMPT 1.33.00.25 DEBUG` on Windows 11 23H2.
- VS Code launched that same executable under `vsdbg`, stopped at the startup
  location, continued into a responsive OpenMPT main window, and stopped
  cleanly with no OpenMPT, `vsdbg`, or MSBuild process left behind.
- The build warns that `subwcrev` and the generated exact SVN revision are
  unavailable. That warning is accepted because this repository treats the
  imported source as a Starting Snapshot, not as a pinned-SVN reproducibility
  contract.

This is intentionally only a personal local foothold. It does not build
`all.sln`, other architectures, installers, CI, or a pinned clean-room
toolchain. On this host, do not substitute the `vs2022win11` project unless the
operating system is upgraded to a version satisfying that project's 24H2
minimum.
