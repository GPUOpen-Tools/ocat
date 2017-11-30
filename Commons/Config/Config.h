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

#pragma once
#include <string>
#include <vector>

struct Config {
  unsigned int hotkey_ = 0x7A;
  unsigned int toggleOverlayHotKey_ = 0x50; // P
  unsigned int recordingTime_ = 0;
  bool recordAllProcesses_ = true;
  unsigned int overlayPosition_ = 2;

  float startDisplayTime_ = 1.0f;
  float endDisplayTime_ = 10.0f;

  bool Load(const std::wstring& path);
};

struct Provider {
	std::string name = "defaultProvider";
	GUID guid; //needs to be set, otherwise invalid provider
	std::string handler;
	std::vector<USHORT> eventIDs = std::vector<USHORT>();
	int traceLevel = 4;
	uint64_t matchAnyKeyword = 0;
	uint64_t matchAllKeyword = 0;
};

struct ConfigCapture {
	std::vector<Provider> provider;

	bool Load(const std::wstring& path);

private:
	void CreateDefault(const std::wstring& fileName);
};