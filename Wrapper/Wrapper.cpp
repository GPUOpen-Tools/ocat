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

#include "stdafx.h"
#include <msclr\marshal_cppstd.h>
#include "Wrapper.h"

Wrapper::PresentMonWrapper::PresentMonWrapper(IntPtr hwnd)
{
  presentMonInterface_ = new PresentMonInterface(reinterpret_cast<HWND>(hwnd.ToPointer()));
}

Wrapper::PresentMonWrapper::~PresentMonWrapper() 
{
  delete presentMonInterface_; 
}

void Wrapper::PresentMonWrapper::ToggleRecording(bool recordAllProcesses, unsigned int hotkey, unsigned int timer)
{ 
    presentMonInterface_->ToggleRecording(recordAllProcesses, hotkey, timer);
}

String ^ Wrapper::PresentMonWrapper::GetRecordedProcess()
{
  String ^ processName = gcnew String(presentMonInterface_->GetRecordedProcess().c_str());
  return processName;
}

bool Wrapper::PresentMonWrapper::CurrentlyRecording()
{
  return presentMonInterface_->CurrentlyRecording();
}

int Wrapper::PresentMonWrapper::GetPresentMonRecordingStopMessage()
{
  return presentMonInterface_->GetPresentMonRecordingStopMessage();
}

Wrapper::OverlayWrapper::OverlayWrapper(IntPtr hwnd)
{
  overlayInterface_ = new OverlayInterface(reinterpret_cast<HWND>(hwnd.ToPointer()));
  overlayInterface_->Init();
}

Wrapper::OverlayWrapper::~OverlayWrapper()
{
  delete overlayInterface_;
}

void Wrapper::OverlayWrapper::StartCaptureExe(String^ exe, String^ cmdArgs)
{
  std::wstring exeW = msclr::interop::marshal_as<std::wstring>(exe);
  std::wstring cmdArgsW = msclr::interop::marshal_as<std::wstring>(cmdArgs);
  overlayInterface_->StartProcess(exeW, cmdArgsW);
}

void Wrapper::OverlayWrapper::StartCaptureAll()
{
  overlayInterface_->StartGlobal();
}

void Wrapper::OverlayWrapper::StopCapture(array<int> ^ overlayThreads)
{
  std::vector<int> tempThreads;
  for each(int thread in overlayThreads) 
  { 
    tempThreads.push_back(thread); 
  }
  overlayInterface_->StopCapture(tempThreads);
}

void Wrapper::OverlayWrapper::FreeInjectedDlls(array<int>^ injectedProcesses)
{
  std::vector<int> tempProcesses;
  for each(int processID in injectedProcesses) 
  { 
    tempProcesses.push_back(processID); 
  }
  overlayInterface_->FreeInjectedDlls(tempProcesses);
}

bool Wrapper::OverlayWrapper::ProcessFinished()
{
  return overlayInterface_->ProcessFinished();
}