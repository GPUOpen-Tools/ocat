gameoverlay
===========

This is an overlay injector capable of displaying a simple framerate counter in Direct3D 11, 12 and Vulkan applications.
The font is stored in a bitmap in the DLL resources and therefore can be customized at will.

The build output DLL has to be loaded into the target application prior to it initializing the graphics API. There are various ways to achieve this, for instance by executing a "LoadLibrary" call remotely.
A built-in mechanism to do this is not part of the project right now. For DirectX applications it is alternativly sufficient to rename the output to dxgi.dll and have the target application load it that way (the project provides all necessary function exports of the system dxgi.dll).

## Structure

```
|-- deps [Build dependencies]
|  |-- d3dx12
|  |-- minhook [MinHook Git submodule]
|  |-- DirectX.props [Visual Studio properties file to handle d3dx12 dependency]
|  |-- GLSLang.targets [MSBuild targets file defining a build target to compile GLSL to SPIR-V]
|  |-- GLSLangItemsSchema.xml [Visual Studio project schema definition to visualize prior rule in the IDE]
|  |-- MinHook.props [Visual Studio properties file to handle MinHook dependency]
|  |-- MinHook.vcxproj [Visual Studio project to build MinHook dependency]
|  |-- Vulkan.props [Visual Studio properties file to handle Vulkan SDK dependency]
|-- res [Resource files]
|  |-- font_atlas.bmp [Contains all the digits for the FPS counter display]
|  |-- resource.rc [DLL resources description, currently contains font_atlas.bmp and the SPIR-V result of building font.vert/frag from below]
|-- source [Source code files]
|  |-- d3d
|  |  |-- com_ptr.hpp [C++ RAII wrapper for COM interfaces to simplify their management]
|  |  |-- d3d11_renderer.cpp [Direct3D 11 FPS counter renderer implementation]
|  |  |-- d3d12_renderer.cpp [Direct3D 12 FPS counter renderer implementation]
|  |  |-- dxgi.cpp [DXGI API hooks]
|  |  |-- dxgi_swapchain.cpp [IDXGISwapChainX interface implementation used to hook the swapchain instances]
|  |-- vulkan
|  |  |-- font.vert/frag [GLSL vertex and fragment shader for font rendering, compiled with custom MSBuild rule]
|  |  |-- vulkan.cpp [Vulkan API hooks]
|  |  |-- vulkan_renderer.cpp [Vulkan FPS counter renderer implementation]
|  |-- critical_section.hpp [Simple C++ wrapper for the Win32 CRITICAL_SECTION object, should probably use std::mutex instead]
|  |-- fps_counter.hpp [FPS calculation using std::chrono::high_resolution_clock]
|  |-- hook.cpp [Simple C++ wrapper for the MinHook API]
|  |-- hook_manager.cpp [Main hooking code, waits for any of the registered target DLLs to be loaded, compares exported functions and hooks all matches]
|  |-- main.cpp [Main entry point and initialization]
|  |-- moving_average.hpp [Simple moving average algorithm used to steady the FPS value]
|  |-- resource_loading.cpp [Code to extract bitmap images and raw data from the DLL resources]
|-- exports.def [Module definition file specifying the function exports used by hook_manager]
|-- gameoverlay.sln [Main Visual Studio solution file]
|-- gameoverlay.vcxproj [Main Visual Studio project file]
```

## License

All the source code is licensed under the conditions of the [BSD 2-clause license](LICENSE.txt).
