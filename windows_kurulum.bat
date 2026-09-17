@echo off
chcp 65001 >nul
title Ayascell MuhurOS - Windows Kurulum Sihirbazi

echo =========================================================
echo    Ayascell MuhurOS - Windows EFI Kurulum Sihirbazi
echo =========================================================
echo.

:: 1. Yonetici yetkisi kontrolu
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [HATA] Bu betik yonetici haklari gerektirir!
    echo Lutfen dosyaya SAG TIKLAYIP "Yonetici olarak calistir" secenegini secin.
    echo.
    pause
    exit /b 1
)

:: 2. dist kontrolu
set SCRIPT_DIR=%~dp0
if not exist "%SCRIPT_DIR%dist\vmlinuz" (
    echo [HATA] dist klasoru veya vmlinuz dosyasi bulunamadi!
    pause
    exit /b 1
)

:: 3. Windows EFI Bolumunu S: surucusu olarak bagla
echo [1/4] Windows EFI Sistem Bolumu (ESP) tespit ediliyor...
mountvol S: /S >nul 2>&1
if not exist S:\EFI (
    echo [HATA] Windows EFI bolumu S: harfine baglanamadi!
    pause
    exit /b 1
)
echo      -> EFI bolumu S: olarak basariyla baglandi.

:: 4. Dosyalari S:\EFI\AyascellMuhur klasorune kopyala
echo [2/4] MuhurOS dosyalari EFI bolumune kopyalaniyor (~19 MB)...
mkdir S:\EFI\AyascellMuhur 2>nul
copy /Y "%SCRIPT_DIR%dist\vmlinuz" S:\EFI\AyascellMuhur\vmlinuz >nul
copy /Y "%SCRIPT_DIR%dist\initramfs.img" S:\EFI\AyascellMuhur\initramfs.img >nul
copy /Y "%SCRIPT_DIR%dist\EFI\BOOT\BOOTX64.EFI" S:\EFI\AyascellMuhur\BOOTX64.EFI >nul

if not exist S:\EFI\AyascellMuhur\config.txt (
    copy /Y "%SCRIPT_DIR%dist\config.txt" S:\EFI\AyascellMuhur\config.txt >nul
    echo      -> config.txt olusturuldu.
) else (
    echo      -> Mevcut config.txt korundu.
)

:: 5. systemd-boot loader dosyalarini hazirla
mkdir S:\EFI\AyascellMuhur\loader\entries 2>nul
copy /Y "%SCRIPT_DIR%dist\loader\loader.conf" S:\EFI\AyascellMuhur\loader\loader.conf >nul 2>nul
copy /Y "%SCRIPT_DIR%dist\loader\entries\muhur.conf" S:\EFI\AyascellMuhur\loader\entries\muhur.conf >nul 2>nul

:: 6. Windows Boot Manager (BCD) veya UEFI kaydi olustur
echo [3/4] UEFI BIOS Boot Menusune "Ayascell MuhurOS" kaydi ekleniyor...
set GUID=
for /f "tokens=2 delims={}" %%g in ('bcdedit /create /d "Ayascell MuhurOS" /application bootapp') do set GUID={%%g}

if defined GUID (
    bcdedit /set %GUID% device partition=S: >nul
    bcdedit /set %GUID% path \EFI\AyascellMuhur\BOOTX64.EFI >nul
    bcdedit /displayorder %GUID% /addlast >nul
    echo      -> Boot kaydi olusturuldu: %GUID%
) else (
    echo      -> BCD kaydi zaten mevcut veya dogrudan UEFI tarafindan goruntulenecektir.
)

:: 7. EFI bolumu baglantisini guvenle kaldir
echo [4/4] EFI bolumu guvenle kapatiliyor...
mountvol S: /D >nul 2>&1

echo.
echo =========================================================
echo  TEBRIKLER! Ayascell MuhurOS Windows bilgisayariniza kuruldu!
echo =========================================================
echo.
echo  * Bilgisayarinizi yeniden baslatirken F12 / F11 / F8 tuşuna
echo    basarak Boot Menusunden "Ayascell MuhurOS"u secebilirsiniz.
echo.
echo  * Windows dosyalariniza, masaustunuze veya C:\ diskinize
echo    kesinlikle hicbir zarar verilmemistir.
echo.
echo  * Yapilandirma dosyasini duzenlemek veya sistemi kaldirmak
echo    isterseniz "windows_kaldir.bat" dosyasini kullanabilirsiniz.
echo =========================================================
echo.
pause
