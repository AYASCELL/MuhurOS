@echo off
chcp 65001 >nul
title Ayascell MuhurOS - Windows USB Bellek Yazici

echo =========================================================
echo    Ayascell MuhurOS - Windows USB Bellek Yazici
echo =========================================================
echo.
echo Bu arac dist klasorundeki MuhurOS dosyalarini
echo herhangi bir FAT32 USB bellege otomatik kopyalar.
echo USB'nize format atmaz, mevcut dosyalarinizi silmez!
echo.

set SCRIPT_DIR=%~dp0
if not exist "%SCRIPT_DIR%dist\vmlinuz" (
    echo [HATA] dist klasoru bulunamadi!
    pause
    exit /b 1
)

set /p USB_LETTER="Lutfen USB bellek surucu harfini girin (Ornek E veya F): "
if "%USB_LETTER%"=="" (
    echo [HATA] Surucu harfi belirtilmedi!
    pause
    exit /b 1
)

set TARGET=%USB_LETTER:~0,1%:
if not exist "%TARGET%\" (
    echo.
    echo [HATA] "%TARGET%\" surucusu bulunamadi!
    echo Lutfen USB bellegin takili ve harfin dogru oldugundan emin olun.
    echo.
    pause
    exit /b 1
)

echo.
echo [*] Dosyalar %TARGET%\ surucusune kopyalaniyor (~19 MB)...
xcopy /E /I /Y "%SCRIPT_DIR%dist\*" "%TARGET%\" >nul

echo.
echo =========================================================
echo  TEBRIKLER! Ayascell MuhurOS USB bellege yazildi!
echo =========================================================
echo.
echo  * USB'nizi herhangi bir bilgisayara (Dell, HP, Lenovo vb.) takip
echo    acilis sirasinda Boot Menusu tusuna (F12, F11, F8) basarak
echo    dogrudan baslatabilirsiniz.
echo.
echo  * Tarih ve yazilari degistirmek icin USB'deki "config.txt"
echo    dosyasini Not Defteri ile acip duzenleyebilirsiniz.
echo =========================================================
echo.
pause
