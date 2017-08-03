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

Wrapper::PresentMonWrapper::PresentMonWrapper()
{
  presentMonInterface_ = new PresentMonInterface();
  overlayInterface_ = new OverlayInterface();
  overlayInterface_->Init();
}

Wrapper::PresentMonWrapper::~PresentMonWrapper() { this->!PresentMonWrapper(); }
Wrapper::PresentMonWrapper::!PresentMonWrapper() { delete overlayInterface_; delete presentMonInterface_; }


void Wrapper::PresentMonWrapper::StartCaptureExe(String^ exe, String^ cmdArgs)
{
  std::wstring exeW = msclr::interop::marshal_as<std::wstring>(exe);
  std::wstring cmdArgsW = msclr::interop::marshal_as<std::wstring>(cmdArgs);
  overlayInterface_->StartProcess(exeW, cmdArgsW);
}

void Wrapper::PresentMonWrapper::StartCaptureAll()
{
  overlayInterface_->StartGlobal();
}

void Wrapper::PresentMonWrapper::StartRecording(IntPtr hwnd)
{
  presentMonInterface_->StartCapture(reinterpret_cast<HWND>(hwnd.ToPointer()));
}

void Wrapper::PresentMonWrapper::StopCapture(array<int> ^ overlayThreads)
{
  presentMonInterface_->StopCapture();

  std::vector<int> tempThreads;
  for each(int thread in overlayThreads) { tempThreads.push_back(thread); }
  overlayInterface_->StopCapture(tempThreads);
}

void Wrapper::PresentMonWrapper::FreeInjectedDlls(array<int>^ injectedProcesses)
{
  std::vector<int> tempProcesses;
  for each(int processID in injectedProcesses) { tempProcesses.push_back(processID); }
  overlayInterface_->FreeInjectedDlls(tempProcesses);
}

void Wrapper::PresentMonWrapper::KeyEvent() 
{ 
    presentMonInterface_->KeyEvent(); 
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

bool Wrapper::PresentMonWrapper::ProcessFinished()
{
  return presentMonInterface_->ProcessFinished();
}

int Wrapper::PresentMonWrapper::GetPresentMonRecordingStopMessage()
{
  return presentMonInterface_->GetPresentMonRecordingStopMessage();
}
