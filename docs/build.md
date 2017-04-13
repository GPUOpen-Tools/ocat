---
title: How to build OCAT
---

# Build

## Application

1. Build __Benchmarking-Tool.sln__  
2. Platform __x64__ and __x86__ for a 64bit build. The x86 build creates only the projects that are needed as x86 for a complete x64 build. This includes the gameoverlay and the DLLInjector   
3. Make sure the Target Platform Version is __10.0.14393.0__ and the Platform Toolset __Visual Studio 2015 (v140)__  
4. Select the __Frontend__ project as Startup Project

To set the version, run `python PresentMon\setVersion.py 0.0.0` inside Benchmarking-Tool

## Installer

For building the installer, the [WiX Toolset](https://wix.codeplex.com/releases/view/624906) must be installed.

1. Build `Installer`
2. Build `Install-Bundle`
