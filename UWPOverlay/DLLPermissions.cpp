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

#include "Utility\Constants.h"
#include "DLLPermissions.h"
#include "Logging\MessageLog.h"
#include "Utility\ProcessHelper.h"
#include "Utility\FileDirectory.h"

bool DLLPermissions::SetDLLPermissions(const std::wstring& libraryName)
{
  g_messageLog.LogInfo("DLL Permission", " set permissions for uwp app");
  auto dllPath = g_fileDirectory.GetDirectory(DirectoryType::Bin) + libraryName;

  if (GetSecurityInfo(dllPath)) {
    if (GetSID_AllApplicationPackages()) {
      if (SetAclEntries()) {
        if (SetDLLSecurityInfo(dllPath)) {
          ReleaseResources();
          return true;
        }
      }
    }
  }

  return false;
}

bool DLLPermissions::GetSecurityInfo(const std::wstring& dllPath)
{
  auto result = GetNamedSecurityInfo(dllPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                     NULL, NULL, &dacl_, NULL, &securityDescriptor_);
  if (result != ERROR_SUCCESS) {
    ReleaseResources();
    g_messageLog.LogError("DLL Permission", " GetNamedSecurityInfo failed",
                     result);
    return false;
  }
  return true;
}

bool DLLPermissions::GetSID_AllApplicationPackages()
{
  if (!ConvertStringSidToSid(L"S-1-15-2-1", &sid_)) {
    ReleaseResources();
    g_messageLog.LogError("DLL Permission", " ConvertStringSidToSid failed ",
                     GetLastError());
    return false;
  }
  return true;
}

bool DLLPermissions::SetAclEntries()
{
  EXPLICIT_ACCESS access{};
  access.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
  access.grfAccessMode = SET_ACCESS;
  access.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
  access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
  access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
  access.Trustee.ptstrName = static_cast<LPWSTR>(sid_);

  const auto result = SetEntriesInAcl(1, &access, dacl_, &updatedDacl_);
  if (result != ERROR_SUCCESS) {
    ReleaseResources();
    g_messageLog.LogError("DLL Permission", " SetEntriesInAcl failed", result);
    return false;
  }
  return true;
}

bool DLLPermissions::SetDLLSecurityInfo(std::wstring& dllPath)
{
  const auto result = SetNamedSecurityInfo(&dllPath[0], SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                           NULL, NULL, updatedDacl_, NULL);
  if (result != ERROR_SUCCESS) {
    g_messageLog.LogError("DLL Permission", " SetNamedSecurityInfo failed",
                     result);
    ReleaseResources();
    return false;
  }
  return true;
}

void DLLPermissions::ReleaseResources()
{
  if (securityDescriptor_) {
    LocalFree(securityDescriptor_);
    securityDescriptor_ = nullptr;
    dacl_ = nullptr;
  }
  if (sid_) {
    LocalFree(sid_);
    sid_ = nullptr;
  }
  if (updatedDacl_) {
    LocalFree(updatedDacl_);
    updatedDacl_ = nullptr;
  }
}