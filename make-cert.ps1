@echo off
set EXE=neel.exe

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe" sign ^
    /f mycert.pfx ^
    /p 1234 ^
    /fd sha256 ^
    /tr http://timestamp.digicert.com ^
    /td sha256 ^
    %EXE%

pause
