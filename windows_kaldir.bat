@echo off
chcp 65001 >nul
title Ayascell MuhurOS - Windows Kaldirma Araci

echo =========================================================
echo    Ayascell MuhurOS - Windows Kaldirma Araci
echo =========================================================
echo.

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [HATA] Bu betik yonetici haklari gerektirir!
    echo Lutfen dosyaya SAG TIKLAYIP "Yonetici olarak calistir" secenegini secin.
    echo.
    pause
    exit /b 1
)

echo [1/3] Windows EFI Bolumu (S:) baglaniyor...
mountvol S: /S >nul 2>&1
if exist S:\EFI\AyascellMuhur (
    echo [2/3] EFI klasoru siliniyor (S:\EFI\AyascellMuhur)...
    rmdir /S /Q S:\EFI\AyascellMuhur >nul 2>&1
    echo      -> Klasor basariyla temizlendi.
) else (
    echo      -> AyascellMuhur klasoru bulunamadi veya zaten silinmis.
)

mountvol S: /D >nul 2>&1

echo [3/3] Windows Boot Manager (BCD) kayitlari temizleniyor...
for /f "tokens=1,2 delims= " %%a in ('bcdedit /enum firmware ^| findstr /i "{"') do (
    bcdedit /enum %%a | findstr /i "Ayascell" >nul
    if not errorlevel 1 (
        echo      -> Kayit siliniyor: %%a
        bcdedit /delete %%a >nul 2>&1
    )
)

echo.
echo =========================================================
echo  Ayascell MuhurOS Windows sisteminizden tamamen kaldirildi!
echo =========================================================
echo.
pause
