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
// The above copyright notice and this permission notice shall be included in all
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

#include <Windows.h>
#include <codecvt>
#include <sstream>
#include <string>

#include "Constants.h"
#include "DLLInjection.h"
#include "FileDirectory.h"
#include "MessageLog.h"
#include "ProcessHelper.h"

bool ParseArguments(int argc, char** argv, DLLInjection::Arguments& args, std::string& directory, bool& freeDLL);

// arguments are -l dll path string and -p processID dword, -free optional to try and free the dll in the process
int main(int argc, char** argv)
{
  DLLInjection::Arguments args;
  std::string directory;
  bool freeDLL = false;

  if (!ParseArguments(argc, argv, args, directory, freeDLL)) {
    MessageBox(NULL, L"Invalid arguments for DLLInjector", NULL, MB_OK);
    return false;
  }

#if _WIN64
  const std::string file = g_fileDirectory.GetDirectory(FileDirectory::DIR_LOG) + g_logFileName;
  g_messageLog.Start(file, "DLLInjector64 " + std::to_string(GetCurrentProcessId()));
#else
  const std::string file = g_fileDirectory.GetDirectory(FileDirectory::DIR_LOG) + g_logFileName;
  g_messageLog.Start(file, "DLLInjector32 " + std::to_string(GetCurrentProcessId()));
#endif

  DLLInjection dllInjection;
  if (freeDLL)
  {
    return dllInjection.FreeDLL(args);
  }
  else
  {
    return dllInjection.InjectDLL(args);
  }
}

bool ParseArguments(int argc, char** argv, DLLInjection::Arguments& args, std::string& directory, bool& freeDLL)
{
  if (argc != 5 && argc != 6) {
    return false;
  }

  for (int i = 0; i < argc; ++i) {
    if (!strcmp(argv[i], "-l")) {
      const std::string dllPath = {argv[++i]};
      args.dllPath = ConvertUTF8StringToUTF16String(dllPath);

      const auto directoryEnd = dllPath.find_last_of('\\');
      const auto topDirPos = dllPath.find_last_of('\\', directoryEnd - 1);
      directory = dllPath.substr(0, topDirPos + 1);

      args.dllName = args.dllPath.substr(directoryEnd + 1, args.dllPath.size() - directoryEnd + 1);
    }
    else if (!strcmp(argv[i], "-p")) {
      std::istringstream ss(argv[++i]);
      if (!(ss >> args.processID)) {
        return false;
      }
    }
    else if (!strcmp(argv[i], "-free"))
    {
      freeDLL = true;
    }
  }

  return true;
}