call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat"
msbuild Tracy.sln /p:Configuration="Release" /p:Platform="x64" -maxcpucount:3
copy /Y x64\Release\Tracy.exe ..\..\..
