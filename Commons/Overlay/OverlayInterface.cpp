#include "OverlayInterface.h"
#include "..\Logging\MessageLog.h"
#include "..\Overlay\DLLInjection.h"
#include "..\Utility\FileDirectory.h"

const char OverlayInterface::enableOverlayEnv_[] = "OCAT_OVERLAY_ENABLED";

OverlayInterface::OverlayInterface()
{}

OverlayInterface::~OverlayInterface(){}

void OverlayInterface::Init()
{
	globalHook_.CleanupOldHooks();
}

void OverlayInterface::StartProcess(const std::wstring& executable, std::wstring& cmdArgs)
{
	g_messageLog.Log(MessageLog::LOG_INFO, "OverlayInterface", "Start single process");
	overlay_.StartProcess(executable, cmdArgs, IsEnabled());
}

void OverlayInterface::StartGlobal()
{
  if (IsEnabled())
  {
    g_messageLog.Log(MessageLog::LOG_INFO, "OverlayInterface", "Start global hook");
    globalHook_.Activate();
  }
}

void OverlayInterface::StopCapture(std::vector<int> overlayThreads)
{
	overlay_.Stop();
	globalHook_.Deactivate();

	for (int threadID : overlayThreads) {
		FreeDLLOverlay(threadID);
	}
}

void OverlayInterface::FreeInjectedDlls(std::vector<int> injectedProcesses)
{
	for (int processID : injectedProcesses) {
		FreeDLL(processID, g_fileDirectory.GetDirectoryW(FileDirectory::DIR_BIN));
	}
}

bool OverlayInterface::IsEnabled()
{
  size_t bufferSize = 0;
  getenv_s(&bufferSize, nullptr, 0, enableOverlayEnv_);
  //env variable not set, enable overlay
  if (bufferSize == 0)
  {
    return true;
  }

  std::string buffer;
  buffer.resize(bufferSize);
  getenv_s(&bufferSize, &buffer[0], bufferSize, enableOverlayEnv_);

  int result;
  //if conversion fails enable overlay
  try
  {
    result = std::stoi(buffer);
  }
  catch (std::invalid_argument)
  {
    return true;
  }
  catch (std::out_of_range)
  {
    return true;
  }

  return result != 0;
}