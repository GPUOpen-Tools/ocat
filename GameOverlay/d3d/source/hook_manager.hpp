// Copyright 2016 Patrick Mours.All rights reserved.
//
// https://github.com/crosire/gameoverlay
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met :
//
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT
// SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <windows.h>
#include <string>
#include "hook.hpp"

#define VTABLE(object) (*reinterpret_cast<GameOverlay::hook::address **>(object))

namespace GameOverlay {
/// <summary>
/// Install hook for the specified target function.
/// </summary>
/// <param name="target">The address of the target function.</param>
/// <param name="replacement">The address of the hook function.</param>
/// <returns>The status of the hook installation.</returns>
bool install_hook(hook::address target, hook::address replacement);
/// <summary>
/// Install hook for the specified virtual function table entry.
/// </summary>
/// <param name="vtable">The virtual function table pointer of the object to targeted
/// object.</param>
/// <param name="offset">The index of the target function in the virtual function table.</param>
/// <param name="replacement">The address of the hook function.</param>
/// <returns>The status of the hook installation.</returns>
bool install_hook(hook::address vtable[], unsigned int offset, hook::address replacement);
__declspec(dllexport) bool replace_vtable_hook(hook::address vtable[], unsigned int offset, hook::address replacement);
/// <summary>
/// Uninstall all previously installed hooks.
/// </summary>
void uninstall_hook();
/// <summary>
/// Register the matching exports in the specified module and install or delay their hooking.
/// </summary>
/// <param name="path">The file path to the target module.</param>
bool register_module(const std::wstring &path);
void register_additional_module(const std::wstring &module_name);
__declspec(dllexport) void add_function_hooks(const std::wstring &module_name, const HMODULE replacement_module);
bool InstallCreateProcessHook();
void HookAllModules();

/// <summary>
/// Find the original/trampoline function for the specified hook.
/// </summary>
/// <param name="replacement">The address of the hook function which was previously used to install
/// a hook.</param>
/// <returns>The address of original/trampoline function.</returns>
__declspec(dllexport) hook::address find_hook_trampoline(hook::address replacement);
template <typename T>
inline T find_hook_trampoline(T replacement)
{
  return reinterpret_cast<T>(find_hook_trampoline(reinterpret_cast<hook::address>(replacement)));
}
}
