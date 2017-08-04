#pragma once

#include "Overlay.h"
#include "GlobalHook.h"

#include <vector>

class OverlayInterface
{
public:
	OverlayInterface(HWND hwnd);
	~OverlayInterface();

	void Init();

	void StartProcess(const std::wstring& executable, std::wstring& cmdArgs);
	void StartGlobal();
	void StopCapture(std::vector<int> overlayThreads);
	void FreeInjectedDlls(std::vector<int> injectedProcesses);
  bool ProcessFinished();

private:
  bool SetMessageFilter();

  static const char enableOverlayEnv_[];
	Overlay overlay_;
	GlobalHook globalHook_;
  ProcessTermination processTermination_;
};