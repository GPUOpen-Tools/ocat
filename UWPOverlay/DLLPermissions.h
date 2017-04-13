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

#include <memory>
#include <string>

#include <Aclapi.h>
#include <Sddl.h>
#include <ShObjIdl.h>

class DLLPermissions {
 public:
  // Sets the permissions to ALL APPLICATION PACKAGES to read and execute to be used by uwp apps
  bool SetDLLPermissions(const std::wstring& libraryName);

 private:
  // Retrieves security info and dacl
  bool GetSecurityInfo(const std::wstring& dllPath);
  // Sets sid for all application packages
  bool GetSID_AllApplicationPackages();
  // Sets the acl entries to read and execute permission and creates the updatedDacl
  bool SetAclEntries();
  // Sets security information of the dll to read and execute for ALL APPLICATION PACKAGES
  bool SetDLLSecurityInfo(std::wstring& dllPath);

  void ReleaseResources();

  PSECURITY_DESCRIPTOR securityDescriptor_ = nullptr;
  PACL dacl_ = nullptr;
  PSID sid_ = nullptr;
  PACL updatedDacl_ = nullptr;
};