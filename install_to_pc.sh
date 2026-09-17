#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist"

echo "=========================================================="
echo "   Ayascell MuhurOS - Bilgisayara Kurulum Sihirbazı"
echo "=========================================================="

if [ "$EUID" -ne 0 ]; then
    echo "⚠️ Bu işlem UEFI NVRAM ve disk erişimi için root yetkisi gerektirir."
    echo "Lütfen 'sudo bash $0' şeklinde çalıştırın."
    exit 1
fi

if [ ! -f "${DIST_DIR}/vmlinuz" ] || [ ! -f "${DIST_DIR}/initramfs.img" ]; then
    echo "[*] MuhurOS derleniyor..."
    "${SCRIPT_DIR}/build.sh"
fi

# 1. EFI Sistem Bölümünü (ESP) Tespit Et
ESP_PATH=""
if [ -d "/boot/efi/EFI" ]; then
    ESP_PATH="/boot/efi"
elif [ -d "/boot/EFI" ]; then
    ESP_PATH="/boot"
fi

if [ -z "$ESP_PATH" ]; then
    echo "❌ Hata: /boot/efi veya /boot üzerinde EFI bölümü bulunamadı."
    exit 1
fi

echo "[1/3] Mevcut EFI bölümü tespit edildi: $ESP_PATH"

DEST_DIR="$ESP_PATH/EFI/AyascellMuhur"
mkdir -p "$DEST_DIR"

echo "[2/3] MuhurOS çekirdeği, initramfs ve config.txt kopyalanıyor..."
cp -f "${DIST_DIR}/vmlinuz" "$DEST_DIR/vmlinuz"
cp -f "${DIST_DIR}/initramfs.img" "$DEST_DIR/initramfs.img"

if [ ! -f "$DEST_DIR/config.txt" ]; then
    cp -f "${DIST_DIR}/config.txt" "$DEST_DIR/config.txt"
    echo "📄 'config.txt' oluşturuldu ($DEST_DIR/config.txt)."
else
    echo "ℹ️ Mevcut config.txt korundu."
fi

# systemd-boot loader entry de ekleyelim (eğer loader/ dizini varsa)
if [ -d "$ESP_PATH/loader/entries" ]; then
    cat << EOF > "$ESP_PATH/loader/entries/ayascell-muhur.conf"
title Ayascell MuhurOS
linux /EFI/AyascellMuhur/vmlinuz
initrd /EFI/AyascellMuhur/initramfs.img
options console=tty1 quiet loglevel=0 fbcon=nodefer vt.global_cursor_default=0
EOF
    echo "ℹ️ Mevcut systemd-boot menüsüne 'ayascell-muhur.conf' kaydı eklendi."
fi

# 2. ESP disk ve bölüm numarasını bul
ESP_DEV=$(df "$ESP_PATH" | tail -1 | awk '{print $1}')
DISK_NAME=$(echo "$ESP_DEV" | sed -E 's/p?[0-9]+$//')
PART_NUM=$(echo "$ESP_DEV" | grep -o -E '[0-9]+$')

echo "[3/3] UEFI BIOS NVRAM kaydı oluşturuluyor ($DISK_NAME p$PART_NUM)..."

# Eski kayıtları temizle
for old_id in $(efibootmgr | grep "Ayascell MuhurOS" | awk '{print $1}' | tr -d 'Boot*'); do
    efibootmgr -b "$old_id" -B > /dev/null 2>&1 || true
done

# Doğrudan EFISTUB üzerinden sıfır gecikmeli boot kaydı oluştur
efibootmgr -c -d "$DISK_NAME" -p "$PART_NUM" -L "Ayascell MuhurOS" \
    -l '\EFI\AyascellMuhur\vmlinuz' \
    -u 'initrd=\EFI\AyascellMuhur\initramfs.img console=tty1 quiet loglevel=0 fbcon=nodefer vt.global_cursor_default=0'

sync
echo "=========================================================="
echo "🎉 TEBRİKLER! Ayascell MuhurOS (~19 MB) kuruldu!"
echo "---------------------------------------------------------"
echo "Bilgisayar açılırken F12 (Boot Menu) tuşuna basarak"
echo "'Ayascell MuhurOS' seçeneğini başlatabilirsiniz."
echo ""
echo "Yapılandırma dosyasını düzenlemek için:"
echo "  sudo nano $DEST_DIR/config.txt"
echo "=========================================================="
