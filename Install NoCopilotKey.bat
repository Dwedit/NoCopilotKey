@echo off
if not exist NoCopilotKey.exe (
    echo NoCopilotKey.exe is not found.
    echo Please extract all files from this zip file before running the installer.
    pause
    exit
)
echo This will create a Start Menu Entry in Startup for this program so it runs automatically.
echo Make sure this EXE is in the directory you want to keep it in.
echo If it isn't in its final directory, close the installer now.
echo Do not move the EXE from this directory.
pause
NoCopilotKey.exe --install
