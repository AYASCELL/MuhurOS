#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${SCRIPT_DIR}/src"
DIST_DIR="${SCRIPT_DIR}/dist"
INITRAMFS_BUILD_DIR="${SCRIPT_DIR}/initramfs_root"

echo "=========================================================="
echo "   Ayascell MuhurOS (~19 MB) Derleyici"
echo "=========================================================="

# 1. Compile static muhur_fb
echo "[1/5] muhur_fb statik C binary derleniyor..."
cd "${SRC_DIR}"
gcc -static -O2 fb_graphics.c config.c ui.c main.c -o "${SCRIPT_DIR}/muhur_fb"
strip -s "${SCRIPT_DIR}/muhur_fb"

# 2. Prepare initramfs directory
echo "[2/5] Mikro initramfs ağacı hazırlanıyor..."
rm -rf "${INITRAMFS_BUILD_DIR}"
mkdir -p "${INITRAMFS_BUILD_DIR}"/{bin,sbin,dev,proc,sys,boot,mnt,etc}

# Copy muhur_fb as /bin/muhur_fb
cp "${SCRIPT_DIR}/muhur_fb" "${INITRAMFS_BUILD_DIR}/bin/muhur_fb"

# Check and copy busybox if available
if [ -f /usr/lib/initcpio/busybox ]; then
    cp /usr/lib/initcpio/busybox "${INITRAMFS_BUILD_DIR}/bin/busybox"
    (cd "${INITRAMFS_BUILD_DIR}/bin" && for app in $(/usr/lib/initcpio/busybox --list 2>/dev/null); do ln -sf busybox "$app"; done)

    for lib in $(ldd /usr/lib/initcpio/busybox 2>/dev/null | grep "=>" | awk '{print $3}'); do
        if [ -f "$lib" ]; then
            libdir="${INITRAMFS_BUILD_DIR}$(dirname "$lib")"
            mkdir -p "$libdir"
            cp "$lib" "$libdir/"
        fi
    done
    ldso=$(ldd /usr/lib/initcpio/busybox 2>/dev/null | grep "ld-linux" | awk '{print $1}')
    if [ -f "$ldso" ]; then
        mkdir -p "${INITRAMFS_BUILD_DIR}$(dirname "$ldso")"
        cp "$ldso" "${INITRAMFS_BUILD_DIR}$(dirname "$ldso")/"
    fi
fi

# Copy and decompress storage and filesystem kernel modules
echo "[*] NVMe, USB ve VFAT çekirdek modülleri initramfs'e ekleniyor..."
mkdir -p "${INITRAMFS_BUILD_DIR}/lib/modules"

KVER_DIR=""
if [ -d "/lib/modules/6.18.51-1-lts" ]; then
    KVER_DIR="/lib/modules/6.18.51-1-lts"
elif [ -d "/lib/modules/7.2.4-zen2-1-zen" ]; then
    KVER_DIR="/lib/modules/7.2.4-zen2-1-zen"
else
    KVER_DIR=$(ls -d /lib/modules/* 2>/dev/null | head -1)
fi

MODULE_NAMES="fat.ko.zst vfat.ko.zst nvme-core.ko.zst nvme.ko.zst usb-storage.ko.zst uas.ko.zst nls_iso8859-1.ko.zst"
for m in $MODULE_NAMES; do
    mod_path=$(find "$KVER_DIR" -name "$m" 2>/dev/null | head -1)
    if [ -n "$mod_path" ]; then
        zstd -d -c "$mod_path" > "${INITRAMFS_BUILD_DIR}/lib/modules/${m%.zst}" 2>/dev/null || cp "$mod_path" "${INITRAMFS_BUILD_DIR}/lib/modules/${m%.zst}"
    fi
done

# Create clean /init script
cat << 'EOF' > "${INITRAMFS_BUILD_DIR}/init"
#!/bin/sh
export PATH=/bin:/sbin

# Standard filesystems
mkdir -p /proc /sys /dev /boot
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

# Run MuhurOS Framebuffer Kiosk
/bin/muhur_fb

# If muhur_fb exits, execute clean hardware shutdown
sync
poweroff -f 2>/dev/null || reboot -f -p 2>/dev/null
EOF

chmod +x "${INITRAMFS_BUILD_DIR}/init"

# Copy default config.txt inside initramfs as fallback
if [ -f "${SCRIPT_DIR}/config.txt" ]; then
    cp "${SCRIPT_DIR}/config.txt" "${INITRAMFS_BUILD_DIR}/config.txt"
fi

# 3. Package initramfs.img
echo "[3/5] initramfs.img paketleniyor (cpio + gzip)..."
mkdir -p "${DIST_DIR}"
cd "${INITRAMFS_BUILD_DIR}"
find . -print0 | cpio --null --create --format=newc | gzip -9 > "${DIST_DIR}/initramfs.img"
echo "  -> initramfs.img boyutu: $(ls -lh "${DIST_DIR}/initramfs.img" | awk '{print $5}')"

# 4. Copy kernel (vmlinuz)
echo "[4/5] Linux LTS çekirdeği kopyalanıyor..."
KERNEL_SRC=""
if [ -f /boot/vmlinuz-linux-lts ]; then
    KERNEL_SRC="/boot/vmlinuz-linux-lts"
elif [ -f /boot/vmlinuz-linux-zen ]; then
    KERNEL_SRC="/boot/vmlinuz-linux-zen"
elif [ -f /boot/vmlinuz-linux ]; then
    KERNEL_SRC="/boot/vmlinuz-linux"
else
    echo "HATA: Sistemde /boot/vmlinuz-* bulunamadı!"
    exit 1
fi
cp "${KERNEL_SRC}" "${DIST_DIR}/vmlinuz"
echo "  -> vmlinuz boyutu: $(ls -lh "${DIST_DIR}/vmlinuz" | awk '{print $5}')"

# 5. Setup EFI bootloader (systemd-boot) and config.txt
echo "[5/5] UEFI önyükleyici ve yapılandırma dosyaları hazırlanıyor..."
mkdir -p "${DIST_DIR}/EFI/BOOT"
mkdir -p "${DIST_DIR}/loader/entries"

if [ -f /usr/lib/systemd/boot/efi/systemd-bootx64.efi ]; then
    cp /usr/lib/systemd/boot/efi/systemd-bootx64.efi "${DIST_DIR}/EFI/BOOT/BOOTX64.EFI"
fi

cat << 'EOF' > "${DIST_DIR}/loader/loader.conf"
default muhur.conf
timeout 0
console-mode max
EOF

cat << 'EOF' > "${DIST_DIR}/loader/entries/muhur.conf"
title Ayascell MuhurOS
linux /vmlinuz
initrd /initramfs.img
options console=tty1 quiet loglevel=0 fbcon=nodefer vt.global_cursor_default=0
EOF

if [ -f "${SCRIPT_DIR}/config.txt" ]; then
    cp "${SCRIPT_DIR}/config.txt" "${DIST_DIR}/config.txt"
fi

# Clean up temporary build files
rm -rf "${INITRAMFS_BUILD_DIR}" "${SCRIPT_DIR}/muhur_fb"

TOTAL_SIZE=$(du -sh "${DIST_DIR}" | awk '{print $1}')
echo "=========================================================="
echo " Başarılı! MuhurOS hazırlandı."
echo " Dağıtım Dizini : ${DIST_DIR}"
echo " Toplam Boyut   : ${TOTAL_SIZE}"
echo "=========================================================="
