cd docs
call build.bat
cd ..
msbuild Benchmarking-Tool.sln /t:Build /p:Configuration=Release;Platform=x64 /p:Configuration=Release;Platform=x86
msbuild Benchmarking-Tool.sln /t:Installer /p:Configuration=Release;Platform=x86
msbuild Benchmarking-Tool.sln /t:Install-Bundle /p:Configuration=Release;Platform=x86