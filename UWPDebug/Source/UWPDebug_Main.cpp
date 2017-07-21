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

#include <Windows.h>
#include <string>

#include "Overlay\DLLInjection.h"
#include "Utility\Constants.h"
#include "Utility\ProcessHelper.h"
#include "Logging\MessageLog.h"
#include "Utility\FileDirectory.h"

typedef NTSTATUS(NTAPI NTRESUMEPROCESS)(HANDLE ProcessHandle);

int main(int argc, char** argv)
{
  g_messageLog.Log(MessageLog::LOG_DEBUG, "UWP", "Entered main");

  DWORD processID = 0;
  std::wstring dllDirectory;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-p")) {
      processID = atoi(argv[++i]);
    }
    else if (!strcmp(argv[i], "-d")) {
      const std::string temp(argv[++i]);
      // add the \ that was removed to pass this as argument
      dllDirectory = ConvertUTF8StringToUTF16String(temp) + L"\\" + g_fileDirectory.GetFolderW(FileDirectory::DIR_BIN);
    }
  }

  if (processID == 0) {
    return -1;
  }
  else if (dllDirectory.empty()) {
    return -1;
  }

  if (!InjectDLL(processID, dllDirectory)) {
    printf("Failed injecting the dll\n");
  }

  // resume process
  NTRESUMEPROCESS* NtResumeProcess = reinterpret_cast<NTRESUMEPROCESS*>(
      GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtResumeProcess"));
  if (NtResumeProcess) {
    const auto rc = NtResumeProcess(GetProcessHandleFromID(processID, PROCESS_SUSPEND_RESUME));
    if (rc < 0) {
      printf("failed resuming process\n");
    }
  }
  else {
    printf("Failed getting ntresumprocess function\n");
  }

  return 0;
}