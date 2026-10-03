@echo off
setlocal EnableExtensions
pushd "%~dp0"
if "%OO_PS4_TOOLCHAIN%"=="" (
  echo ERROR: OO_PS4_TOOLCHAIN is not set. Set it to your installed OpenOrbis toolchain folder.
  goto failed
)
set "SDK=%OO_PS4_TOOLCHAIN%"
set "OUT=lan_transfer\x64\Debug"
set "PKG_NAME=SFTP (SimpleFTP).pkg"
rem PkgTool.Core targets .NET Core 3.0; allow the installed newer runtime to run it.
set "DOTNET_ROLL_FORWARD=Major"
where clang++ >nul 2>nul
if errorlevel 1 if exist "C:\Program Files\LLVM\bin\clang++.exe" set "PATH=C:\Program Files\LLVM\bin;%PATH%"
if errorlevel 1 if exist "C:\Program Files (x86)\LLVM\bin\clang++.exe" set "PATH=C:\Program Files (x86)\LLVM\bin;%PATH%"
where clang++ >nul 2>nul
if errorlevel 1 (echo ERROR: clang++ was not found. LLVM may be installed outside its default folder; add its bin folder to PATH.& goto failed)
where ld.lld >nul 2>nul
if errorlevel 1 (echo ERROR: ld.lld is not on PATH. Install LLVM and add its bin folder to PATH.& goto failed)
if not exist "%SDK%\include\orbis\Net.h" (echo ERROR: OO_PS4_TOOLCHAIN does not point to an OpenOrbis toolchain folder.& goto failed)
if not exist "%SDK%\bin\windows\create-fself.exe" (echo ERROR: create-fself.exe is missing from %SDK%\bin\windows.& goto failed)
if not exist "%SDK%\bin\windows\PkgTool.Core.exe" (echo ERROR: PkgTool.Core.exe is missing from %SDK%\bin\windows.& goto failed)
if not exist "%SDK%\bin\windows\create-gp4.exe" (echo ERROR: create-gp4.exe is missing from %SDK%\bin\windows.& goto failed)
if not exist "%SDK%\samples\font\sce_module\libc.prx" (echo ERROR: OpenOrbis runtime module libc.prx is missing from samples\font\sce_module.& goto failed)
if not exist "%SDK%\samples\font\sce_module\libSceFios2.prx" (echo ERROR: OpenOrbis runtime module libSceFios2.prx is missing from samples\font\sce_module.& goto failed)
if not exist "%OUT%" mkdir "%OUT%"
if not exist "sce_module" mkdir "sce_module"
copy /y "%SDK%\samples\font\sce_module\libc.prx" "sce_module\libc.prx" >nul
if errorlevel 1 goto failed
copy /y "%SDK%\samples\font\sce_module\libSceFios2.prx" "sce_module\libSceFios2.prx" >nul
if errorlevel 1 goto failed
clang++ --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -DGRAPHICS_USES_FONT -I"%SDK%\include" -I"%SDK%\include\c++\v1" -c lan_transfer\main.cpp -o "%OUT%\main.o"
if errorlevel 1 goto failed
clang++ --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -DGRAPHICS_USES_FONT -I"%SDK%\include" -I"%SDK%\include\c++\v1" -Ilan_transfer -c lan_transfer\graphics.cpp -o "%OUT%\graphics.o"
if errorlevel 1 goto failed
ld.lld -m elf_x86_64 -pie --script "%SDK%\link.x" --eh-frame-hdr -L"%SDK%\lib" -lc -lkernel -lc++ -lSceVideoOut -lSceSysmodule -lSceFreeType -lSceNet -lSceNetCtl -lScePad -lSceUserService "%SDK%\lib\crt1.o" "%OUT%\main.o" "%OUT%\graphics.o" -o "%OUT%\lan_transfer.elf"
if errorlevel 1 goto failed
"%SDK%\bin\windows\create-fself.exe" -in "%OUT%\lan_transfer.elf" --out "%OUT%\lan_transfer.oelf" --eboot "eboot.bin" --paid 0x3800000000000011
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_new sce_sys\param.sfo
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo APP_TYPE --type Integer --maxsize 4 --value 1
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo ATTRIBUTE --type Integer --maxsize 4 --value 0
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo APP_VER --type Utf8 --maxsize 8 --value 1.00
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo CATEGORY --type Utf8 --maxsize 4 --value gd
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo CONTENT_ID --type Utf8 --maxsize 48 --value IV0000-LANT00001_00-LANTRANSFER00000
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo TITLE --type Utf8 --maxsize 128 --value "SFTP (SimpleFTP)"
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo TITLE_ID --type Utf8 --maxsize 12 --value LANT00001
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo VERSION --type Utf8 --maxsize 8 --value 1.00
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" sfo_setentry sce_sys\param.sfo SYSTEM_VER --type Integer --maxsize 4 --value 0
if errorlevel 1 goto failed
set "PKG_FILES=eboot.bin sce_sys/param.sfo sce_sys/icon0.png assets/fonts/Gontserrat-Regular.ttf sce_module/libc.prx sce_module/libSceFios2.prx"
if exist "sce_sys\pic1.png" set "PKG_FILES=%PKG_FILES% sce_sys/pic1.png"
"%SDK%\bin\windows\create-gp4.exe" -out pkg.gp4 --content-id=IV0000-LANT00001_00-LANTRANSFER00000 --files "%PKG_FILES%"
if errorlevel 1 goto failed
"%SDK%\bin\windows\PkgTool.Core.exe" pkg_build pkg.gp4 .
if errorlevel 1 goto failed
if not exist "IV0000-LANT00001_00-LANTRANSFER00000.pkg" (echo ERROR: PkgTool finished without creating the expected PKG.& goto failed)
move /y "IV0000-LANT00001_00-LANTRANSFER00000.pkg" "%PKG_NAME%" >nul
if errorlevel 1 goto failed
echo Build complete: %PKG_NAME%
pause
popd
exit /b 0

:failed
echo.
echo Build did not complete. This window will stay open so you can read the error.
pause
popd
exit /b 1
