if(!(Test-Path .\OCAT-Installer\redist)) 
{ 
    mkdir .\OCAT-Installer\redist 
}
if(!(Test-Path .\OCAT-Installer\redist\vc_redist.x64.exe)) 
{ 
    Write-Host "Download vc_redist.x64.exe"
    Invoke-WebRequest -Uri https://download.microsoft.com/download/5/7/b/57b2947c-7221-4f33-b35e-2fc78cb10df4/vc_redist.x64.exe -OutFile .\OCAT-Installer\redist\vc_redist.x64.exe
    Write-Host "Download vc_redist.x64.exe [DONE]"
}
else {
    Write-Host "vc_redist.x64.exe already up to date."
}

if(!(Test-Path .\OCAT-Installer\redist\vc_redist.x86.exe)) 
{ 
    Write-Host "Download vc_redist.x86.exe"
    Invoke-WebRequest -Uri https://download.microsoft.com/download/1/d/8/1d8137db-b5bb-4925-8c5d-927424a2e4de/vc_redist.x86.exe -OutFile .\OCAT-Installer\redist\vc_redist.x86.exe
    Write-Host "Download vc_redist.x86.exe [DONE]"
}
else {
    Write-Host "vc_redist.x86.exe already up to date."
}