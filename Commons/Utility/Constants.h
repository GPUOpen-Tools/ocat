//
// Copyright(c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <string>

const std::wstring g_globalHookProcess32 = L"GlobalHook32.exe";
const std::wstring g_globalHookProcess64 = L"GlobalHook64.exe";

const std::string g_globalHookFunction32 = "_GlobalHookProc@12";
const std::string g_globalHookFunction64 = "GlobalHookProc";

const std::wstring g_injectorProcess32 = L"DLLInjector32";
const std::wstring g_injectorProcess64 = L"DLLInjector64";

const std::wstring g_libraryName32 = L"GameOverlay32.dll";
const std::wstring g_libraryName64 = L"GameOverlay64.dll";

const std::wstring g_logFileName = L"PresentMonLog";