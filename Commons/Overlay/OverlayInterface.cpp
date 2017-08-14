#include "OverlayInterface.h"
#include "..\Logging\MessageLog.h"
#include "..\Overlay\DLLInjection.h"
#include "..\Utility\FileDirectory.h"
#include "OverlayMessage.h"

bool g_ProcessFinished;
bool g_CaptureAll;
HWND g_FrontendHwnd;


OverlayInterface::OverlayInterface() 
{
  // Nothing to do.
}

OverlayInterface::~OverlayInterface()
{
  globalHook_.Deactivate();
  globalHook_.CleanupOldHooks();
  processTermination_.UnRegister();
}

bool OverlayInterface::Init(HWND hwnd)
{
  if (!g_fileDirectory.Initialize())
  {
    return false;
  }

  g_FrontendHwnd = hwnd;
	globalHook_.CleanupOldHooks();
  SetMessageFilter();
  return true;
}

void OverlayInterface::StartProcess(const std::wstring& executable, std::wstring& cmdArgs)
{
	g_messageLog.Log(MessageLog::LOG_INFO, "OverlayInterface", "Start single process");
  g_ProcessFinished = false;
  g_CaptureAll = false;
  if (overlay_.StartProcess(executable, cmdArgs, true))
  {
    processTermination_.Register(overlay_.GetProcessID());
  }
}

void OverlayInterface::StartGlobal()
{
  g_messageLog.Log(MessageLog::LOG_INFO, "OverlayInterface", "Start global hook");
  g_ProcessFinished = false;
  g_CaptureAll = true;
  globalHook_.Activate();
}

void OverlayInterface::StopCapture(std::vector<int> overlayThreads)
{
	overlay_.Stop();
  processTermination_.UnRegister();
	globalHook_.Deactivate();

	for (int threadID : overlayThreads) {
		FreeDLLOverlay(threadID);
	}

  g_ProcessFinished = true;
}

void OverlayInterface::FreeInjectedDlls(std::vector<int> injectedProcesses)
{
	for (int processID : injectedProcesses) {
		FreeDLL(processID, g_fileDirectory.GetDirectoryW(FileDirectory::DIR_BIN));
	}
}

void CALLBACK OnProcessExit(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired)
{
  // Only interested in finished process if a specific process is captured.
  g_ProcessFinished = !g_CaptureAll;
  // Send message to frontend to trigger a UI update.
  OverlayMessage::PostFrontendMessage(g_FrontendHwnd, OverlayMessageType::OVERLAY_ThreadTerminating, 0);
}

bool OverlayInterface::ProcessFinished()
{
  return g_ProcessFinished;
}

bool OverlayInterface::SetMessageFilter()
{
  CHANGEFILTERSTRUCT changeFilter;
  changeFilter.cbSize = sizeof(CHANGEFILTERSTRUCT);
  if (!ChangeWindowMessageFilterEx(g_FrontendHwnd, OverlayMessage::overlayMessageType, MSGFLT_ALLOW, &changeFilter))
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayInterface",
      "Changing window message filter failed", GetLastError());
    return false;
  }
  return true;
}