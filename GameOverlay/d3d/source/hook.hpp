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

#pragma once

namespace GameOverlay {
struct hook {
  typedef void *address;
  enum class status {
    unknown = -1,
    success,
    not_executable = 7,
    unsupported_function,
    allocation_failure,
    memory_protection_failure,
  };

  hook();
  hook(address target, address replacement);

  /// <summary>
  /// Return whether the hook is valid.
  /// </summary>
  bool valid() const;
  /// <summary>
  /// Return whether the hook is currently enabled.
  /// </summary>
  bool enabled() const;
  /// <summary>
  /// Return whether the hook is currently installed.
  /// </summary>
  bool installed() const;
  /// <summary>
  /// Return whether the hook is not currently installed.
  /// </summary>
  bool uninstalled() const { return !installed(); }
  /// <summary>
  /// Enable or disable the hook.
  /// </summary>
  /// <param name="enable">Boolean indicating if hook should be enabled or disabled.</param>
  bool enable(bool enable = true) const;
  /// <summary>
  /// Install the hook.
  /// </summary>
  status install();
  /// <summary>
  /// Uninstall the hook.
  /// </summary>
  status uninstall();

  /// <summary>
  /// Return the trampoline function address of the hook.
  /// </summary>
  address call() const;
  template <typename T>
  inline T call() const
  {
    return reinterpret_cast<T>(call());
  }

  address target, replacement, trampoline;
};
}
