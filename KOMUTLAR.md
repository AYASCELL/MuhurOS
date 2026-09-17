# 🎖️ Ayascell MuhurOS - Eksiksiz Terminal ve Kullanım Rehberi

Bu rehber; **Ayascell MuhurOS** bağımsız donanım mühürleme ve kiosk işletim sisteminin derlenmesi, sanal makinede test edilmesi, USB'ye aktarılması, bilgisayara kurulması, `config.txt` ile kodsuz düzenlenmesi ve güvenle kaldırılmasına dair **tüm adımları ve komutları** içerir.

---

## 🚀 1. HIZLI KOMUT REFERANSI (LİNUX)

Tüm komutlar `MuhurOS` klasörü içinden çalıştırılır:

> 💡 **Not:** MuhurOS `dist/` klasöründe derlenmiş, test edilmiş ve kullanıma hazır **(~19 MB)** olarak gelir. Kod derlemekle uğraşmadan doğrudan **1.3, 1.4 veya 1.5** adımıyla kuruluma geçebilirsiniz.

```bash
# 1.1. (Opsiyonel) Sıfırdan MuhurOS Derleme (~19 MB dist/ oluşturur):
./build.sh

# 1.2. Hızlı Sanal Makine Testi (QEMU UEFI ve GOP ekran testi):
./test_qemu.sh

# 1.3. USB Belleğe Kopyalama (Format atmaz, tak-çalıştır):
sudo ./copy_to_usb.sh /dev/sdX1   # (Kendi USB bölümünüz, Örn: /dev/sdb1)

# 1.4. Bilgisayarın Mevcut EFI Bölümüne Kurma (Misafir Mod):
sudo ./install_to_pc.sh

# 1.5. Bağımsız 50 MB Mühür Bölümüne Kurma (Ölümsüz / Çift ESP Modu):
# (Açtığınız bölümün adıyla çalıştırın, Örn: /dev/sda3 veya /dev/nvme0n1p4):
sudo ./install_to_partition.sh /dev/BÖLÜM_ADINIZ

# 1.6. Bilgisayardan Temiz Kaldırma:
sudo ./uninstall_from_pc.sh
```

---

## 🪟 2. WINDOWS KULLANICILARI İÇİN REHBER

Windows yüklü bilgisayarlarda komut satırıyla uğraşmadan çift tıklayarak çalıştırabileceğiniz hazır `.bat` betikleri:

- **`windows_usb_kopyala.bat`**: Çift tıklayın ➔ USB sürücü harfini (Örn: `E`) girin. MuhurOS dosyalarını USB belleğe kopyalar (format atmaz).
- **`windows_kurulum.bat`**: Sağ tık ➔ "Yönetici olarak çalıştır". Windows EFI Sistem Bölümüne kurar ve Windows Boot Manager menüsüne "Ayascell MuhurOS" seçeneğini ekler.
- **`windows_kaldir.bat`**: Sağ tık ➔ "Yönetici olarak çalıştır". Windows EFI bölümündeki dosyaları ve boot kaydını tek tıkla siler.

---

## 🎮 3. KİOSK EKRAN KONTROLLERİ

Bilgisayar mühür ekranında açıldığında kullanabileceğiniz kısayollar:

- **`[1]`**: **Mühür Tutanağı ve Gerekçe:** Resmi protokol, neden ve sonuç hükümlerini tam ekran açar ve kapatır.
- **`[2]`**: **Sistemi Acil Kapat:** Sayaç bitmesini beklemeden donanımsal güç kesme (ACPI S5) ile bilgisayarı anında kapatır.
- **`[P]`**: **Sayacı Duraklat:** Kapanma sayacını duraklatır veya tekrar başlatır.
- **`[F2]`**: **Yetkili Girişi (Varsayılan PIN: `1923`):** Yönetici düzenleme panelini açar.
- **`[ESC]`**: Açık olan modalları (tutanak veya ayar ekranını) kapatır.
- **⏱️ Otomatik Kapanma:** Cihaz açıldıktan sonra hiçbir tuşa basılmasa dahi belirlenen süre (varsayılan: 180 sn = 3 dakika) sonra donanım gücünü tamamen keser.

---

## 📝 4. YAZILARI VE AYARLARI DÜZENLEME (3 FARKLI YOL)

### Yöntem A: `config.txt` ile Kodsuz Düzenleme (En Kolay)
USB kök dizinindeki veya EFI bölümündeki `config.txt` dosyasını Windows Not Defteri veya Linux metin düzenleyiciyle açıp kaydedebilirsiniz. Kod derlemenize gerek yoktur:

```ini
# =========================================================
#   Ayascell MuhurOS Kiosk Yapilandirma Dosyasi
# =========================================================

sahip = Ayascell
baslik = BU BILGISAYAR GUVENLIK PROTOKOLU GEREGINCE MUHURLUDUR

# --- HEDEF MUHUR TARIHI ---
hedef_yil = 2026
hedef_ay = 11
hedef_gun = 1

# --- SISTEM VE GUVENLIK AYARLARI ---
# Acilis ekraninda otomatik kapanis suresi (saniye, varsayilan: 180 sn = 3 dk)
kapanma_suresi = 180

# Yonetici ekrani acis PIN kodu (F2 tusu ile acilir)
pin = 1923

# --- MUHUR TUTANAGI METINLERI ---
tutanak_baslik = AYASCELL SISTEM GUVENLIK VE MUHUR PROTOKOLU TUTANAGI
gerekce = Bilgisayar sahibi Ayascell'in belirledigi sure ve amac dogrultusunda cihazin guvenligini saglamak, yetkisiz mudahaleleri onlemek ve kisisel verileri koruma altina almaktir.
sonuc = Bu cihaz; hedef tarihe kadar her turlu ucuncu sahis kurcalamasi ve veri transferine karsi muhur altindadir. Sistem birkac dakika icinde otomatik kapanacaktir.
```

### Yöntem B: Açılış Ekranından Canlı Düzenleme (`F2`)
1. Mühür ekranındayken **`[F2]`** tuşuna basın.
2. PIN kodunu girin: **`1923`** ve **`[ENTER]`** yapın.
3. Açılan canlı düzenleme panelinde:
   - **`[↑] / [↓]`**: Hedef Yıl, Ay, Gün veya Kapanma Süresi alanını seçin.
   - **`[←] / [→]`**: Değerleri artırın veya azaltın.
   - **`[ENTER]`**: Ayarları diske kalıcı olarak kaydedin (`config.txt` güncellenir).
   - **`[ESC]`**: Paneli kapatıp ana ekrana dönün.

### Yöntem C: C Kaynak Kodundan Varsayılanları Değiştirme
Varsayılan değerleri C kodunun içine gömmek isterseniz `src/config.c` dosyasındaki `config_set_defaults()` fonksiyonunu düzenleyip `./build.sh` komutunu çalıştırabilirsiniz.

---

## 🛡️ 5. İZOLE TEST YÖNTEMLERİ

Bilgisayarınızın ana işletim sistemine veya dahili disklerine temas etmeden sistemi denemek için:

### Seçenek 1: Sanal Makinede Test Etme (QEMU UEFI)
Doğrudan donanımsal GOP ekran ve güç kapatma simülasyonunu çalıştırır:
```bash
./test_qemu.sh
```

### Seçenek 2: Fiziksel USB ile Gerçek Bilgisayarda Test
`copy_to_usb.sh` USB'nizi formatlamaz; sadece MuhurOS dosyalarını içine kopyalar:
```bash
sudo ./copy_to_usb.sh /dev/sda1
```
USB'yi istediğiniz bilgisayara takıp açılışta **F12 / F11 / F8** tuşuyla USB'yi seçerek test edin.

---

## 💻 6. BİLGİSAYARA KURULUM DETAYLARI

### 🥇 YÖNTEM 1: Mevcut EFI Bölümüne Kurulum (Misafir Mod)
Diskinizi bölmeden, mevcut sisteminize hiçbir risk oluşturmadan kurmak için:
```bash
sudo ./install_to_pc.sh
```
- `/boot/efi/EFI/AyascellMuhur/` içine yerleşir.
- Mevcut Windows veya Linux dosyalarınıza dokunmaz.
- BIOS NVRAM menüsüne "Ayascell MuhurOS" seçeneği eklenir.

---

### 🥈 YÖNTEM 2: Bağımsız 50 MB Mühür Bölümüne Kurulum (Ölümsüz / Çift ESP Modu)
Bu yöntem ana sisteminizin `BOOTX64.EFI` veya EFI dosyalarına asla temas etmez. Kendi bağımsız bölümünde yaşar ve **BIOS sıfırlansa dahi anakart tarafından otomatik olarak tanınır.**

#### Adım 1: GParted ile 50 MB Alan Açma:
1. GParted uygulamasını açın.
2. Herhangi bir diskinize sağ tıklayıp **Boyutlandır/Taşı** deyin.
3. **"Ardındaki boş alan (MiB)"** kutusuna `50` (veya `100`) yazıp **Boyutlandır**'a basın.
4. Oluşan gri *"Ayrılmamış"* alana sağ tıklayıp **Yeni** deyin:
   - **Dosya Sistemi:** `fat32`
   - **Etiket (Label):** `MUHUROS`
   - **+ Ekle** butonuna basın.
5. Üstteki yeşil onay butonuna (**✔ Tüm İşlemleri Uygula**) basın.

#### Adım 2: Bölüme Kurulumu Başlatın:
`lsblk` komutunu yazarak yeni açtığınız `MUHUROS` etiketli bölümün adını öğrenin ve o bölümü belirterek kurun:

```bash
# Örnek kullanım (Kendi açtığınız bölümün adını yazın):
sudo ./install_to_partition.sh /dev/sda3
# veya NVMe SSD kullanıyorsanız:
sudo ./install_to_partition.sh /dev/nvme0n1p4
```
*(Eğer parametre yazmadan doğrudan `sudo ./install_to_partition.sh` yazarsanız, betik mevcut tüm disk bölümlerinizi listeler ve sizden bölüm adını girmenizi ister).*

---

## ⚡ 7. AÇILIŞ ÖNCELİĞİNİ (BOOT ORDER) DEĞİŞTİRME

Bilgisayarınızda `efibootmgr` ile açılış sırasını dilediğiniz gibi ayarlayabilirsiniz:

- **Seçenek 1: Bilgisayar Açılır Açılmaz Doğrudan Mühür Gelsin:**
  ```bash
  # Mühür kaydının numarasını (Örn: 0001) en başa alın:
  sudo efibootmgr -o 0001,0000,0007,0005,0002,0003,0004
  ```
  *(Açılışta hiçbir tuşa basılmasa bile doğrudan mühür açılır; 3 dakika sonra cihaz otomatik kapanır).*

- **Seçenek 2: Normalde Kendi İşletim Sisteminiz Açılsın, Mühür Sadece F12'ye Basınca Gelsin:**
  ```bash
  # Ana sisteminizin numarasını (Örn: 0000) en başa alın:
  sudo efibootmgr -o 0000,0001,0007,0005,0002,0003,0004
  ```
  *(Bilgisayar normal açıldığında ana masaüstünüz açılır; mühür ekranı sadece açılışta F12 Boot Menüsünden seçildiğinde açılır).*

---

## 🔄 8. BİLGİSAYARDAN TEMİZLEME VE KALDIRMA

Mühür süresi bittiğinde sistemi tamamen temizlemek için:

### 🐧 Linux'ta Kaldırma:
```bash
sudo ./uninstall_from_pc.sh
```
Karşınıza gelen menüden:
- **`1`**: Ana EFI'deki misafir kurulumu temizler (50 MB bölüme dokunmaz).
- **`2`**: 50 MB mühür bölümünü doğrulatıp temizler.
- **`3`**: Tüm mühür sistemini tamamen süpürür.

### 🪟 Windows'ta Kaldırma:
`windows_kaldir.bat` dosyasına **Sağ Tıklayıp "Yönetici olarak çalıştır"** demeniz yeterlidir. Hem EFI klasörünü hem de boot kaydını tek saniyede siler.
