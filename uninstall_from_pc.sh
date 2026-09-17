#!/bin/bash
set -e

echo "========================================================="
echo "   Ayascell MuhurOS - Bilgisayardan Kaldırma Aracı"
echo "========================================================="

if [ "$EUID" -ne 0 ]; then
    echo "⚠️ Bu işlem root yetkisi gerektirir."
    echo "Lütfen 'sudo bash $0' şeklinde çalıştırın."
    exit 1
fi

# 1. Mühür Bölümünü Otomatik Tespit Et (Etiketi: MUHUROS)
DETECTED_PART=$(lsblk -no PATH,LABEL 2>/dev/null | grep -w "MUHUROS" | awk '{print $1}' | head -1 || true)

echo "Sistem Durumu:"
if [ -d "/boot/efi/EFI/AyascellMuhur" ] || [ -d "/boot/EFI/AyascellMuhur" ]; then
    echo " [✓] Ana EFI'de eski misafir kurulum mevcut (/boot/efi/EFI/AyascellMuhur)"
fi
if [ -n "$DETECTED_PART" ]; then
    echo " [✓] Bağımsız Mühür Bölümü tespit edildi: $DETECTED_PART (Etiket: MUHUROS)"
fi
echo ""

echo "Kurulum temizleme seçenekleri:"
echo " 1) Sadece ana EFI'deki eski misafir kurulumu temizle (50 MB mühür bölümüne dokunmaz)"
echo " 2) Bağımsız 50 MB Mühür Bölümünü temizle ($DETECTED_PART)"
echo " 3) Hepsini tamamen temizle (Tüm mühür sistemini kaldır)"
echo ""
read -p "Seçiminiz [1/2/3] (Varsayılan 1): " CHOICE
CHOICE=${CHOICE:-1}

# 2. Eski systemd servisini durdur ve devre dışı bırak
if systemctl is-active --quiet muhuros 2>/dev/null; then
    systemctl stop muhuros 2>/dev/null || true
fi
if systemctl is-enabled --quiet muhuros 2>/dev/null; then
    systemctl disable muhuros 2>/dev/null || true
fi
rm -f /etc/systemd/system/muhuros.service 2>/dev/null || true
systemctl daemon-reload 2>/dev/null || true

# 3. UEFI NVRAM Kayıtlarını Sil
if [ "$CHOICE" == "1" ]; then
    # Sadece eski partitionsuz "Ayascell MuhurOS" kaydını sil, (Bölüm) kaydını koru!
    BOOT_ENTRIES=$(efibootmgr | grep -F "Ayascell MuhurOS" | grep -v "(Bölüm)" | awk '{print $1}' | sed -E 's/Boot([0-9A-Fa-f]+)\*?/\1/' || true)
elif [ "$CHOICE" == "2" ]; then
    # Sadece (Bölüm) kaydını sil
    BOOT_ENTRIES=$(efibootmgr | grep -F "Ayascell MuhurOS (Bölüm)" | awk '{print $1}' | sed -E 's/Boot([0-9A-Fa-f]+)\*?/\1/' || true)
else
    # Hepsini sil
    BOOT_ENTRIES=$(efibootmgr | grep -F "Ayascell MuhurOS" | awk '{print $1}' | sed -E 's/Boot([0-9A-Fa-f]+)\*?/\1/' || true)
fi

if [ -n "$BOOT_ENTRIES" ]; then
    for b in $BOOT_ENTRIES; do
        echo "[*] UEFI Girişi (Boot$b) siliniyor..."
        efibootmgr -b "$b" -B >/dev/null 2>&1 || true
        echo "✅ UEFI Boot kaydı temizlendi (Boot$b)."
    done
else
    echo "ℹ️ İlgili UEFI kaydı bulunamadı veya zaten silinmiş."
fi

# 4. Ana EFI Dosyalarını Temizle (1 veya 3 seçildiyse)
if [ "$CHOICE" == "1" ] || [ "$CHOICE" == "3" ]; then
    echo "[*] Ana EFI'deki misafir kurulum dosyaları temizleniyor..."
    for p in /boot/efi/EFI/AyascellMuhur /boot/EFI/AyascellMuhur; do
        if [ -d "$p" ]; then
            rm -rf "$p"
            echo "✅ '$p' dizini silindi."
        fi
    done

    for esp in /boot/efi /boot; do
        if [ -f "$esp/loader/entries/ayascell-muhur.conf" ]; then
            rm -f "$esp/loader/entries/ayascell-muhur.conf"
            echo "✅ '$esp/loader/entries/ayascell-muhur.conf' silindi."
        fi
    done
fi

# 5. Bağımsız Mühür Bölümünü Temizle (2 veya 3 seçildiyse)
if [ "$CHOICE" == "2" ] || [ "$CHOICE" == "3" ]; then
    echo ""
    echo "========================================================="
    echo "⚠️ GÜVENLİK KORUMASI: Hedef Mühür Bölümünü Elle Doğrulayın"
    echo "========================================================="
    if [ -n "$DETECTED_PART" ]; then
        echo "ℹ️ Sistemde tespit edilen mühür bölümü: $DETECTED_PART (MUHUROS)"
    fi
    echo "Kazara yanlış bir diski silmemek için, silinecek bölüm yolunu"
    echo "tam olarak elle yazmanız ZORUNLUDUR (Örn: /dev/nvme1n1p3)."
    echo ""
    read -p "Temizlenecek bölümü yazın (İptal için boş bırakıp Enter'a basın): " TARGET_CLEAN

    if [ -z "$TARGET_CLEAN" ]; then
        echo "ℹ️ Hiçbir disk bölümü yazılmadı. Mühür bölümüne dokunulmadı."
    elif [ ! -b "$TARGET_CLEAN" ]; then
        echo "❌ HATA: '$TARGET_CLEAN' geçerli bir blok aygıtı/bölüm değil! İşlem iptal edildi."
    else
        # KORUMA: Ham disk kontrolü
        DEV_TYPE=$(lsblk -no TYPE "$TARGET_CLEAN" 2>/dev/null | head -1 || true)
        if [ "$DEV_TYPE" != "part" ]; then
            echo "❌ KRİTİK KORUMA: '$TARGET_CLEAN' bir bölüm değil, ham disktir! İşlem engellendi."
            exit 1
        fi

        # KORUMA: Aktif sistem mount kontrolü
        CUR_MNT=$(lsblk -no MOUNTPOINT "$TARGET_CLEAN" 2>/dev/null | head -1 || true)
        if [ -n "$CUR_MNT" ]; then
            if [[ "$CUR_MNT" == "/" || "$CUR_MNT" =~ ^/(boot|home|etc|usr|var|root) ]]; then
                echo "❌ KRİTİK KORUMA: '$TARGET_CLEAN' sisteminizde '$CUR_MNT' olarak bağlı! Silinemez."
                exit 1
            fi
        fi

        read -p "⚠️ DİKKAT: '$TARGET_CLEAN' içindeki MuhurOS dosyaları silinecektir. Onaylıyor musunuz? [e/H]: " CONFIRM_DEL
        if [[ "$CONFIRM_DEL" =~ ^[eEçÇyY]$ ]]; then
            echo "[*] '$TARGET_CLEAN' mühür bölümü bağlanıp temizleniyor..."
            MNT_TMP="/mnt/muhur_clean_$$"
            mkdir -p "$MNT_TMP"
            mount "$TARGET_CLEAN" "$MNT_TMP" 2>/dev/null || true
            rm -rf "$MNT_TMP"/*
            sync
            umount "$MNT_TMP" 2>/dev/null || true
            rmdir "$MNT_TMP" 2>/dev/null || true
            echo "✅ '$TARGET_CLEAN' bölümünün içi tamamen temizlendi."
            echo "ℹ️ İsterseniz GParted üzerinden bu 50 MB'lık bölümü tamamen silebilirsiniz."
        else
            echo "ℹ️ Kullanıcı onayı verilmedi. Bölüme dokunulmadı."
        fi
    fi
fi

echo "========================================================="
echo "🎉 Temizleme işlemi başarıyla tamamlandı!"
echo "========================================================="
