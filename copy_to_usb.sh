#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist"
TARGET_DEV="$1"

echo "=========================================================="
echo "   Ayascell MuhurOS - USB Bellek Yazma Sihirbazı"
echo "=========================================================="

# 1. Root Yetkisi Kontrolü
if [ "$EUID" -ne 0 ]; then
    echo "❌ HATA: Bu işlem disk erişimi için root yetkisi gerektirir."
    echo "Lütfen 'sudo bash $0 /dev/sdX1' şeklinde çalıştırın."
    exit 1
fi

# 2. Parametre Kontrolü
if [ -z "$TARGET_DEV" ] || [ ! -b "$TARGET_DEV" ]; then
    echo "Kullanım: sudo $0 /dev/sdX1"
    echo ""
    echo "📌 Örnek: sudo $0 /dev/sdb1"
    echo ""
    echo "Mevcut disk bölümleri:"
    lsblk -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINT | grep -E "sd|nvme"
    exit 1
fi

# 3. KORUMA: Ham Disk Engeli
DEV_TYPE=$(lsblk -no TYPE "$TARGET_DEV" 2>/dev/null | head -1 || true)
if [ "$DEV_TYPE" != "part" ]; then
    echo "❌ KRİTİK KORUMA: '$TARGET_DEV' bir disk bölümü (partition) değil, ham disktir!"
    echo "Lütfen bölüm numarası belirtin (Örnek: ${TARGET_DEV}1)."
    exit 1
fi

# 4. KORUMA: Aktif Sistem Bölümü Kontrolü
CURRENT_MOUNT=$(lsblk -no MOUNTPOINT "$TARGET_DEV" 2>/dev/null | head -1 || true)
if [ -n "$CURRENT_MOUNT" ]; then
    if [[ "$CURRENT_MOUNT" == "/" || "$CURRENT_MOUNT" =~ ^/(boot|home|etc|usr|var|root) ]]; then
        echo "❌ KRİTİK KORUMA: '$TARGET_DEV' şu anda sisteminizde '$CURRENT_MOUNT' olarak bağlı!"
        echo "Ana sistem diskine yazma yapılması KESİNLİKLE ENGELLENDİ."
        exit 1
    fi
fi

# 5. Dağıtım klasörünü kontrol et / derle
if [ ! -f "${DIST_DIR}/vmlinuz" ] || [ ! -f "${DIST_DIR}/initramfs.img" ]; then
    echo "[*] MuhurOS derleniyor..."
    "${SCRIPT_DIR}/build.sh"
fi

# 6. Güvenli Bağlama ve Kopyalama (Trap Korumalı)
MOUNT_DIR="/mnt/muhuros_usb_$$"
mkdir -p "$MOUNT_DIR"
trap 'if [ -d "$MOUNT_DIR" ]; then umount -f "$MOUNT_DIR" 2>/dev/null || true; rmdir "$MOUNT_DIR" 2>/dev/null || true; fi' EXIT INT TERM

echo "[1/3] USB bölümü bağlanıyor: $TARGET_DEV -> $MOUNT_DIR"
# Eğer daha önce başka bir yere bağlıysa unmount edip temiz bağlayalım
if [ -n "$CURRENT_MOUNT" ]; then
    umount "$TARGET_DEV" 2>/dev/null || true
fi
mount "$TARGET_DEV" "$MOUNT_DIR"

echo "[2/3] MuhurOS dosyaları kopyalanıyor (~19 MB)..."
cp -r "${DIST_DIR}/"* "$MOUNT_DIR/"

echo "[3/3] Değişiklikler diske yazılıyor (sync)..."
sync
umount "$MOUNT_DIR"
rmdir "$MOUNT_DIR"
trap - EXIT INT TERM

echo "=========================================================="
echo "🎉 BAŞARILI! Ayascell MuhurOS USB belleğe yazıldı."
echo "---------------------------------------------------------"
echo "Bu USB'yi istediğiniz bilgisayara (Dell, HP, Lenovo vb.) takıp"
echo "UEFI Boot Menüsünden (F12 / F11 / F8) doğrudan başlatabilirsiniz."
echo ""
echo "Not Defteri ile ayarları değiştirmek için USB'deki 'config.txt'"
echo "dosyasını açıp düzenleyebilirsiniz."
echo "=========================================================="
