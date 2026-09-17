#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist"
TARGET_DEV="$1"

echo "=========================================================="
echo "   Ayascell MuhurOS - Özel Mühür Bölümüne Kurulum Aracı"
echo "=========================================================="

# 1. Root Yetkisi Kontrolü
if [ "$EUID" -ne 0 ]; then
    echo "❌ HATA: Bu işlem disk ve bölüm erişimi için root yetkisi gerektirir."
    echo "Lütfen 'sudo bash $0 /dev/nvmeXnYpZ' şeklinde çalıştırın."
    exit 1
fi

# 2. Parametre ve Aygıt Kontrolü (Manuel Seçim)
if [ -z "$TARGET_DEV" ] || [ ! -b "$TARGET_DEV" ]; then
    echo "Kullanım: sudo $0 /dev/sdXY  (veya /dev/nvmeXnYpZ)"
    echo ""
    echo "📌 Örnekler:"
    echo "   sudo $0 /dev/sda3"
    echo "   sudo $0 /dev/nvme0n1p4"
    echo "   sudo $0 /dev/nvme1n1p3"
    echo ""
    echo "Mevcut disk bölümleriniz:"
    lsblk -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINT | grep -E "sd|nvme"
    echo ""
    read -p "Lütfen kurmak istediğiniz mühür bölümünü yazın (Örn: /dev/nvme1n1p3): " TARGET_DEV
fi

if [ -z "$TARGET_DEV" ] || [ ! -b "$TARGET_DEV" ]; then
    echo "❌ HATA: Geçerli bir disk bölümü belirtilmedi!"
    exit 1
fi

# 3. KORUMA: Ham Disk (disk) Kontrolü (Bölüm olmalı, tüm disk olamaz!)
DEV_TYPE=$(lsblk -no TYPE "$TARGET_DEV" 2>/dev/null | head -1 || true)
if [ "$DEV_TYPE" != "part" ]; then
    echo "❌ KRİTİK KORUMA: '$TARGET_DEV' bir disk bölümü (partition) değil, disk aygıtının kendisidir!"
    echo "Tüm diske zarar vermemek için işlem durduruldu."
    echo "Lütfen sonuna bölüm numarası ekleyerek çalıştırın (Örnek: ${TARGET_DEV}1 veya ${TARGET_DEV}p1)."
    exit 1
fi

# 4. KORUMA: Aktif Sistem Bölümü Kontrolü (Root /, /boot, /home vb. ezilemez!)
CURRENT_MOUNT=$(lsblk -no MOUNTPOINT "$TARGET_DEV" 2>/dev/null | head -1 || true)
if [ -n "$CURRENT_MOUNT" ]; then
    if [[ "$CURRENT_MOUNT" == "/" || "$CURRENT_MOUNT" =~ ^/(boot|home|etc|usr|var|root) ]]; then
        echo "❌ KRİTİK KORUMA: '$TARGET_DEV' şu anda sisteminizde '$CURRENT_MOUNT' olarak bağlı!"
        echo "İşletim sisteminizi bozmamak için bu bölüme kurulum yapılması KESİNLİKLE ENGELLENDİ."
        exit 1
    fi
fi

# 5. Dağıtım klasörünü kontrol et / derle
if [ ! -f "${DIST_DIR}/vmlinuz" ] || [ ! -f "${DIST_DIR}/initramfs.img" ]; then
    echo "[*] MuhurOS derleniyor..."
    "${SCRIPT_DIR}/build.sh"
fi

# 6. Bölümün Boyutunu ve Dosya Sistemini İncele
PART_SIZE=$(lsblk -no SIZE "$TARGET_DEV" 2>/dev/null | head -1 || true)
FS_TYPE=$(lsblk -no FSTYPE "$TARGET_DEV" 2>/dev/null | head -1 || true)
PART_LABEL=$(lsblk -no LABEL "$TARGET_DEV" 2>/dev/null | head -1 || true)

echo "[1/4] Hedef bölüm inceleniyor:"
echo "      -> Aygıt: $TARGET_DEV"
echo "      -> Boyut: $PART_SIZE"
echo "      -> Dosya Sistemi: ${FS_TYPE:-Yok / Bilinmiyor}"
[ -n "$PART_LABEL" ] && echo "      -> Etiket: $PART_LABEL"

# 7. FAT32 Doğrulaması ve Formatlama Koruması
if [ "$FS_TYPE" != "vfat" ] && [ "$FS_TYPE" != "fat" ]; then
    echo ""
    echo "⚠️ UYARI: '$TARGET_DEV' bölümü FAT32 olarak biçimlendirilmemiş."
    echo "UEFI BIOS'un bu bölümü bağımsız bir önyükleme diski olarak tanıması için FAT32 şarttır."
    echo ""
    read -p "Bu bölüm 'MUHUROS' etiketiyle FAT32 olarak biçimlendirilsin mi? [e/H]: " CONFIRM
    if [[ "$CONFIRM" =~ ^[eEçÇyY]$ ]]; then
        # Eğer geçici olarak bağlıysa ayır
        umount "$TARGET_DEV" 2>/dev/null || true
        echo "[*] '$TARGET_DEV' FAT32 olarak biçimlendiriliyor..."
        mkfs.vfat -F 32 -n "MUHUROS" "$TARGET_DEV"
    else
        echo "İşlem kullanıcı tarafından iptal edildi."
        exit 1
    fi
fi

# 8. Bölüm türünü bağımsız ESP (EF00) olarak işaretle
DISK_NAME=$(echo "$TARGET_DEV" | sed -E 's/p?[0-9]+$//')
PART_NUM=$(echo "$TARGET_DEV" | grep -o -E '[0-9]+$')

if command -v sgdisk >/dev/null 2>&1; then
    echo "[2/4] Bölüm türü bağımsız ESP olarak işaretleniyor (sgdisk -t $PART_NUM:EF00)..."
    sgdisk -t "$PART_NUM:EF00" "$DISK_NAME" >/dev/null 2>&1 || true
fi

# 9. Güvenli Bağlama (Mount) ve Dosya Kopyalama (Trap Korumalı)
MOUNT_DIR="/mnt/muhuros_part_$$"
mkdir -p "$MOUNT_DIR"
trap 'if [ -d "$MOUNT_DIR" ]; then umount -f "$MOUNT_DIR" 2>/dev/null || true; rmdir "$MOUNT_DIR" 2>/dev/null || true; fi' EXIT INT TERM

echo "[3/4] Mühür dosyaları kopyalanıyor (~19 MB)..."
mount "$TARGET_DEV" "$MOUNT_DIR"

# Kendi bağımsız BOOTX64.EFI ve önyükleyici yapılandırmasını kopyala
cp -r "${DIST_DIR}/"* "$MOUNT_DIR/"
sync
umount "$MOUNT_DIR"
rmdir "$MOUNT_DIR"
trap - EXIT INT TERM

# 10. BIOS NVRAM Kaydı Ekle (Ekstra kolaylık için)
echo "[4/4] UEFI BIOS Boot Menüsüne bağımsız kayıt ekleniyor..."
for old_id in $(efibootmgr | grep "Ayascell MuhurOS (Bölüm)" | awk '{print $1}' | tr -d 'Boot*'); do
    efibootmgr -b "$old_id" -B > /dev/null 2>&1 || true
done

efibootmgr -c -d "$DISK_NAME" -p "$PART_NUM" -L "Ayascell MuhurOS (Bölüm)" \
    -l '\EFI\BOOT\BOOTX64.EFI' >/dev/null 2>&1 || true

echo "=========================================================="
echo "🎉 TEBRİKLER! MuhurOS bağımsız mühür bölümüne kuruldu!"
echo "=========================================================="
echo "🛡️ ÖLÜMSÜZ VE SIFIR ZARAR MİMARİSİ:"
echo " 1. Ana sisteminizin (Windows / Arch Linux) EFI klasörüne"
echo "    veya BOOTX64.EFI dosyasına TEK BİR BAYT DAHİ DOKUNULMADI."
echo ""
echo " 2. BIOS pili bitip sıfırlansa veya NVRAM tamamen temizlense bile:"
echo "    Anakartınız bu $TARGET_DEV bölümünü bağımsız bir UEFI disk"
echo "    olarak görecek ve F12 Boot Menüsünde OTOMATİK listeleyecektir!"
echo ""
echo " 3. Açılışta F12 tuşuna basarak doğrudan başlatabilirsiniz."
echo "=========================================================="
