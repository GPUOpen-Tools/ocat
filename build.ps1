Write-Host "Build OCAT x64"
msbuild OCAT.sln /t:restore /t:Build /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
Write-Host "Build OCAT x64 [DONE]"

Write-Host "Build OCAT x86"
msbuild OCAT.sln /t:restore /t:Build /p:Configuration=Release /p:Platform=x86 /verbosity:minimal
Write-Host "Build OCAT x86 [DONE]"

Write-Host "Build Installer"
msbuild OCAT.sln /t:Installer /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
Write-Host "Build Installer [DONE]"

Write-Host "Build Install Bundle"
msbuild OCAT.sln /t:Install-Bundle /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
Write-Host "Build Install Bundle [DONE]"