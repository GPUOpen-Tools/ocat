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

#include "hook.hpp"
#include <assert.h>
#include "MinHook.h"
#include "Logging/MessageLog.h"

namespace GameOverlay {
namespace {
unsigned long s_reference_count = 0;
}

hook::hook() : target(nullptr), replacement(nullptr), trampoline(nullptr) {}
hook::hook(address target, address replacement)
    : target(target), replacement(replacement), trampoline(nullptr)
{
}

bool hook::valid() const
{
  return target != nullptr && replacement != nullptr && target != replacement;
}
bool hook::enabled() const
{
  if (!valid()) {
    return false;
  }

  const MH_STATUS statuscode = MH_EnableHook(target);

  if (statuscode == MH_ERROR_ENABLED) {
    return true;
  }

  MH_DisableHook(target);

  return false;
}
bool hook::installed() const { return trampoline != nullptr; }
bool hook::enable(bool enable) const
{
  if (enable) {
    const MH_STATUS statuscode = MH_EnableHook(target);

    return statuscode == MH_OK || statuscode == MH_ERROR_ENABLED;
  }
  else {
    const MH_STATUS statuscode = MH_DisableHook(target);

    return statuscode == MH_OK || statuscode == MH_ERROR_DISABLED;
  }
}



hook::status hook::install()
{
  if (!valid()) {
    g_messageLog.LogVerbose("hook::install", "Unsupported function");
    return status::unsupported_function;
  }

  if (s_reference_count++ == 0) {
    MH_Initialize();
  }

  const MH_STATUS statuscode = MH_CreateHook(target, replacement, &trampoline);
  g_messageLog.LogVerbose("hook::install", "Status: " + std::string(MH_StatusToString(statuscode)));
  if (statuscode == MH_OK || statuscode == MH_ERROR_ALREADY_CREATED) {
    enable();
    return status::success;
  }

  if (--s_reference_count == 0) {
    MH_Uninitialize();
  }

  return static_cast<status>(statuscode);
}
hook::status hook::uninstall()
{
  if (!valid()) {
    return status::unsupported_function;
  }

  const MH_STATUS statuscode = MH_RemoveHook(target);

  if (statuscode == MH_ERROR_NOT_CREATED) {
    return status::success;
  }
  else if (statuscode != MH_OK) {
    return static_cast<status>(statuscode);
  }

  trampoline = nullptr;

  if (--s_reference_count == 0) {
    MH_Uninitialize();
  }

  return status::success;
}

hook::address hook::call() const
{
  assert(installed());

  return trampoline;
}
}
