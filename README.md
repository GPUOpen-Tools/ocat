# OCAT
The Open Capture and Analytics Tool (OCAT) provides an FPS overlay and performance measurement for D3D11, D3D12, and Vulkan


## Downloads

For downloads, look into the releases section: https://github.com/GPUOpen-Tools/OCAT/releases


## Setup

This setup requires:
- [Visual Studio 2015](https://www.visualstudio.com) or [Visual Studio 2017](https://www.visualstudio.com)
- [Vulkan SDK](https://vulkan.lunarg.com/)
- [Python 3.6](https://www.python.org/downloads/release/python-360/)
- [.Net 4.5](https://www.microsoft.com/de-de/download/details.aspx?id=30653)

Open Powershell in an appropriate directory and perform the following steps:

```
git clone https://github.com/GPUOpen-Tools/OCAT
cd .\OCAT\
.\pre-build.ps1
.\build.ps1
```

The pre-build script pulls the MinHook submodule and downloads the redistributable files.
It also builds the documentation. You can use the Visual Studio solution OCAT.sln afterwards.
The build script performs the necessary steps to create the installer.
After a successful build you can find the artifact in OCAT-Installer\bin\Release\OCAT.exe.