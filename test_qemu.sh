#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist"

if [ ! -f "${DIST_DIR}/vmlinuz" ] || [ ! -f "${DIST_DIR}/initramfs.img" ]; then
    echo "[*] MuhurOS derleniyor..."
    "${SCRIPT_DIR}/build.sh"
fi

echo "=========================================================="
echo "   Ayascell MuhurOS - QEMU Test Başlatılıyor"
echo "=========================================================="
echo " - Kapatmak için klavyeden [2] tuşuna basabilir"
echo "   veya 3 dakika otomatik sayacın bitmesini bekleyebilirsiniz."
echo "=========================================================="

OVMF=""
for f in /usr/share/edk2/x64/OVMF.4m.fd /usr/share/ovmf/x64/OVMF.4m.fd /usr/share/edk2-ovmf/x64/OVMF.4m.fd /usr/share/OVMF/OVMF.fd; do
    if [ -f "$f" ]; then
        OVMF="$f"
        break
    fi
done

BIOS_ARG=""
if [ -n "$OVMF" ]; then
    BIOS_ARG="-bios $OVMF"
    echo "UEFI Firmware: $OVMF"
else
    echo "Uyarı: OVMF UEFI firmware bulunamadı, varsayılan modda deneniyor."
fi

qemu-system-x86_64 \
    -m 512M \
    $BIOS_ARG \
    -kernel "${DIST_DIR}/vmlinuz" \
    -initrd "${DIST_DIR}/initramfs.img" \
    -append "console=tty1 quiet loglevel=0 fbcon=nodefer vt.global_cursor_default=0" \
    -vga std \
    -serial mon:stdio
