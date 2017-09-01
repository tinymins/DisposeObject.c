echo off
cls
color 0A
cd %~dp0
%~d0
REM Require Admin
set UAC=0
bcdedit>nul
if errorlevel 1 set UAC=1
if %UAC%==1 (
color CE
echo Please run this script as administrator.
pause
color 0A
exit
)
REM Admin Required
echo --------------------
echo Querying objects...
echo --------------------
for /f "tokens=2 " %%a in ('tasklist /fi "imagename eq JX3Client.exe" /nh') do (
DisposeObject.exe %%a \BaseNamedObjects\A5DFEC3F
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\0DF11825
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\5D2D1767
echo --------------------
)
for /f "tokens=2 " %%a in ('tasklist /fi "imagename eq JX3ClientX64.exe" /nh') do (
DisposeObject.exe %%a \BaseNamedObjects\A5DFEC3F
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\0DF11825
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\5D2D1767
echo --------------------
)
echo Press any key to open donate page, close window to exit.
pause
explorer "https://jx3.derzh.com/donate/"
