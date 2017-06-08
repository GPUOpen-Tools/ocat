Building OCAT
=============

Prerequisites
-------------

- `Visual Studio 2017 <https://www.visualstudio.com>`_
- `Vulkan SDK <https://vulkan.lunarg.com/>`_
- `Python 3.6 <https://www.python.org/downloads/release/python-360/>`_
- `.NET 4.5 <https://www.microsoft.com/en-us/download/details.aspx?id=30653>`_

Build
-----

Open Powershell in an appropriate directory and perform the following steps:

.. code-block:: PowerShell

    git clone https://github.com/GPUOpen-Tools/OCAT
    cd .\OCAT\
    .\pre-build.ps1
    .\build.ps1

The pre-build script pulls the MinHook submodule and downloads the redistributable files.
It also builds the documentation. You can use the Visual Studio solution ``OCAT.sln`` afterwards. The build script performs the necessary steps to create the installer.
After a successful build you can find the artifact in ``OCAT-Installer\bin\Release\OCAT.exe``.