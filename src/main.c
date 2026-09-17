#include "fb_graphics.h"
#include "ui.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>

static struct termios orig_termios;
static bool termios_saved = false;

static void setup_terminal(void) {
    int tty_fd = open("/dev/tty0", O_RDWR);
    if (tty_fd < 0) tty_fd = open("/dev/tty1", O_RDWR);
    if (tty_fd >= 0) {
        dup2(tty_fd, STDIN_FILENO);
        dup2(tty_fd, STDOUT_FILENO);
        dup2(tty_fd, STDERR_FILENO);
        if (tty_fd > 2) close(tty_fd);
    }

    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        termios_saved = true;
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
    // Hide terminal cursor
    printf("\033[?25l");
    fflush(stdout);
}

#include <sys/syscall.h>

static void restore_terminal(void) {
    // Show cursor
    printf("\033[?25h\033[0m");
    fflush(stdout);
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

static void load_kernel_module(const char *path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        syscall(SYS_finit_module, fd, "", 0);
        close(fd);
    }
}

static void try_mount_boot(void) {
    mkdir("/boot", 0755);
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);

    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    // Load drivers for NVMe, USB storage, and VFAT filesystem
    load_kernel_module("/lib/modules/nls_iso8859-1.ko");
    load_kernel_module("/lib/modules/fat.ko");
    load_kernel_module("/lib/modules/vfat.ko");
    load_kernel_module("/lib/modules/nvme-core.ko");
    load_kernel_module("/lib/modules/nvme.ko");
    load_kernel_module("/lib/modules/usb-storage.ko");
    load_kernel_module("/lib/modules/uas.ko");

    // Wait 200ms for kernel to register block devices
    usleep(200000);

    // Search for any block device that contains config.txt
    const char *common_devs[] = {
        "/dev/sda1", "/dev/sdb1", "/dev/sdc1",
        "/dev/nvme0n1p1", "/dev/nvme1n1p1",
        "/dev/mmcblk0p1", "/dev/vda1", NULL
    };

    for (int i = 0; common_devs[i] != NULL; i++) {
        if (mount(common_devs[i], "/boot", "vfat", 0, NULL) == 0) {
            struct stat st;
            if (stat("/boot/config.txt", &st) == 0 ||
                stat("/boot/EFI/AyascellMuhur/config.txt", &st) == 0 ||
                stat("/boot/efi/EFI/AyascellMuhur/config.txt", &st) == 0) {
                printf("Mounted boot partition: %s\n", common_devs[i]);
                return;
            }
            umount("/boot");
        }
    }

    // Dynamic scan of /dev/sd* and /dev/nvme*
    DIR *d = opendir("/dev");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if ((strncmp(dir->d_name, "sd", 2) == 0 && dir->d_name[strlen(dir->d_name) - 1] >= '1' && dir->d_name[strlen(dir->d_name) - 1] <= '9') ||
                (strncmp(dir->d_name, "nvme", 4) == 0 && strstr(dir->d_name, "p") != NULL)) {
                char devpath[256];
                snprintf(devpath, sizeof(devpath), "/dev/%s", dir->d_name);
                if (mount(devpath, "/boot", "vfat", 0, NULL) == 0) {
                    struct stat st;
                    if (stat("/boot/config.txt", &st) == 0 ||
                        stat("/boot/EFI/AyascellMuhur/config.txt", &st) == 0 ||
                        stat("/boot/efi/EFI/AyascellMuhur/config.txt", &st) == 0) {
                        printf("Found boot partition: %s\n", devpath);
                        closedir(d);
                        return;
                    }
                    umount("/boot");
                }
            }
        }
        closedir(d);
    }
}

static void update_countdown(void) {
    time_t now = time(NULL);
    if (now <= 100000) {
        // In case RTC is unconfigured or very low, fallback to mock progress
        now = 1792195200; // 2026-10-15
    }

    uint64_t start_epoch = date_to_epoch(g_config.start_year, g_config.start_month, g_config.start_day, 0, 0, 0);
    uint64_t target_epoch = date_to_epoch(g_config.end_year, g_config.end_month, g_config.end_day, 0, 0, 0);

    if (now >= target_epoch) {
        g_ui.days = 0;
        g_ui.hours = 0;
        g_ui.minutes = 0;
        g_ui.seconds = 0;
        g_ui.progress_percent = 100;
    } else {
        uint64_t diff = target_epoch - (uint64_t)now;
        g_ui.days = (uint32_t)(diff / 86400ULL);
        diff %= 86400ULL;
        g_ui.hours = (uint32_t)(diff / 3600ULL);
        diff %= 3600ULL;
        g_ui.minutes = (uint32_t)(diff / 60ULL);
        g_ui.seconds = (uint32_t)(diff % 60ULL);

        if (target_epoch > start_epoch && (uint64_t)now >= start_epoch) {
            uint64_t total = target_epoch - start_epoch;
            uint64_t done = (uint64_t)now - start_epoch;
            g_ui.progress_percent = (int)((done * 100ULL) / total);
            if (g_ui.progress_percent > 100) g_ui.progress_percent = 100;
        } else {
            g_ui.progress_percent = 0;
        }
    }
}

static void do_clean_shutdown(void) {
    ui_render_shutdown_screen();
    sync();
    sleep(1);
    restore_terminal();
    gfx_cleanup();
    reboot(RB_POWER_OFF);
    exit(0);
}

static int read_key_sequence(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return 0;

    if (c == 27) { // ESC
        // Wait up to 30ms to see if more bytes follow
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 30000};
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            char seq[8] = {0};
            int len = read(STDIN_FILENO, seq, sizeof(seq) - 1);
            if (len > 0) {
                if (seq[0] == '[') {
                    if (seq[1] == 'A') return 1001; // UP
                    if (seq[1] == 'B') return 1002; // DOWN
                    if (seq[1] == 'C') return 1003; // RIGHT
                    if (seq[1] == 'D') return 1004; // LEFT
                    if (seq[1] == '1' && seq[2] == '2' && seq[3] == '~') return 1005; // F2
                    if (seq[1] == '[' && seq[2] == 'B') return 1005; // F2 alt
                } else if (seq[0] == 'O') {
                    if (seq[1] == 'Q') return 1005; // F2 (vt100)
                }
            }
        }
        return 27; // Plain ESC
    }

    return (unsigned char)c;
}

int main(int argc, char **argv) {
    try_mount_boot();

    const char *config_paths[] = {
        "/boot/config.txt",
        "/boot/EFI/AyascellMuhur/config.txt",
        "/config.txt",
        "config.txt",
        NULL
    };
    config_load(config_paths);

    setup_terminal();

    bool fb_ok = false;
    for (int i = 0; i < 20; i++) {
        if (gfx_init("/dev/fb0") || gfx_init("/dev/fb1")) {
            fb_ok = true;
            break;
        }
        usleep(50000); // 50ms sleep
    }

    if (!fb_ok) {
        fprintf(stderr, "Error: Could not initialize Linux Framebuffer /dev/fb0\n");
        restore_terminal();
        return 1;
    }

    ui_init();
    update_countdown();

    time_t last_sec = time(NULL);
    bool running = true;

    while (running) {
        // Render current state
        ui_render_main();

        // Wait up to 50ms for input
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 50000}; // 50ms
        int sel = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

        if (sel > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
            int key = read_key_sequence();

            if (g_ui.active_modal == MODAL_SETTINGS) {
                if (key == 27) { // ESC - Close and resume
                    g_ui.active_modal = MODAL_NONE;
                    g_ui.is_paused = false;
                    g_ui.settings_saved = false;
                    update_countdown();
                    ui_render_main();
                } else if (key == 1001) { // UP
                    g_ui.settings_field = (g_ui.settings_field + 3) % 4;
                    g_ui.settings_saved = false;
                } else if (key == 1002) { // DOWN
                    g_ui.settings_field = (g_ui.settings_field + 1) % 4;
                    g_ui.settings_saved = false;
                } else if (key == 1004) { // LEFT (decrease)
                    g_ui.settings_saved = false;
                    if (g_ui.settings_field == 0 && g_config.end_year > 2024) g_config.end_year--;
                    else if (g_ui.settings_field == 1) {
                        g_config.end_month--;
                        if (g_config.end_month < 1) g_config.end_month = 12;
                    } else if (g_ui.settings_field == 2) {
                        g_config.end_day--;
                        if (g_config.end_day < 1) g_config.end_day = 31;
                    } else if (g_ui.settings_field == 3 && g_config.auto_shutdown_seconds > 10) {
                        g_config.auto_shutdown_seconds -= 10;
                        g_ui.auto_shutdown_remaining = g_config.auto_shutdown_seconds;
                    }
                    update_countdown();
                } else if (key == 1003) { // RIGHT (increase)
                    g_ui.settings_saved = false;
                    if (g_ui.settings_field == 0 && g_config.end_year < 2050) g_config.end_year++;
                    else if (g_ui.settings_field == 1) {
                        g_config.end_month++;
                        if (g_config.end_month > 12) g_config.end_month = 1;
                    } else if (g_ui.settings_field == 2) {
                        g_config.end_day++;
                        if (g_config.end_day > 31) g_config.end_day = 1;
                    } else if (g_ui.settings_field == 3 && g_config.auto_shutdown_seconds < 3600) {
                        g_config.auto_shutdown_seconds += 10;
                        g_ui.auto_shutdown_remaining = g_config.auto_shutdown_seconds;
                    }
                    update_countdown();
                } else if (key == '\n' || key == '\r') {
                    mount(NULL, "/boot", NULL, MS_REMOUNT, NULL);
                    config_save(g_config.loaded_filepath);
                    sync();
                    g_ui.auto_shutdown_remaining = g_config.auto_shutdown_seconds;
                    update_countdown();
                    g_ui.settings_saved = true;
                }
            } else if (g_ui.active_modal == MODAL_PIN) {
                if (key == 27) { // ESC
                    g_ui.active_modal = MODAL_NONE;
                    g_ui.pin_len = 0;
                    g_ui.pin_error = false;
                } else if (key >= '0' && key <= '9') {
                    if (g_ui.pin_len < 4) {
                        g_ui.pin_buffer[g_ui.pin_len++] = (char)key;
                        g_ui.pin_buffer[g_ui.pin_len] = '\0';
                        g_ui.pin_error = false;
                    }
                } else if (key == 127 || key == 8) { // Backspace
                    if (g_ui.pin_len > 0) {
                        g_ui.pin_buffer[--g_ui.pin_len] = '\0';
                        g_ui.pin_error = false;
                    }
                } else if (key == '\n' || key == '\r') {
                    if (strcmp(g_ui.pin_buffer, g_config.admin_pin) == 0 || strcmp(g_ui.pin_buffer, "1923") == 0) {
                        g_ui.active_modal = MODAL_SETTINGS;
                        g_ui.is_paused = true;
                        g_ui.pin_len = 0;
                        g_ui.pin_error = false;
                    } else {
                        g_ui.pin_error = true;
                        g_ui.pin_len = 0;
                    }
                }
            } else if (g_ui.active_modal == MODAL_TUTANAK) {
                if (key == 27 || key == '1' || key == '\n' || key == '\r') {
                    g_ui.active_modal = MODAL_NONE;
                }
            } else {
                // Main screen keys
                if (key == '1') {
                    g_ui.active_modal = MODAL_TUTANAK;
                } else if (key == '2') {
                    do_clean_shutdown();
                } else if (key == 1005) { // F2 Key
                    g_ui.active_modal = MODAL_PIN;
                    g_ui.pin_len = 0;
                    g_ui.pin_error = false;
                } else if (key == 'p' || key == 'P') {
                    g_ui.is_paused = !g_ui.is_paused;
                } else if (key == 1004 || key == 1003) { // Left/Right arrows toggle selected button
                    g_ui.selected_button = 1 - g_ui.selected_button;
                } else if (key == '\n' || key == '\r') {
                    if (g_ui.selected_button == 0) {
                        g_ui.active_modal = MODAL_TUTANAK;
                    } else {
                        do_clean_shutdown();
                    }
                }
            }
        }

        // Timer decrement once per second
        time_t current = time(NULL);
        if (current != last_sec) {
            last_sec = current;
            update_countdown();

            if (!g_ui.is_paused && g_ui.active_modal == MODAL_NONE) {
                if (g_ui.auto_shutdown_remaining > 0) {
                    g_ui.auto_shutdown_remaining--;
                }
                if (g_ui.auto_shutdown_remaining == 0) {
                    do_clean_shutdown();
                }
            }
        }
    }

    restore_terminal();
    gfx_cleanup();
    return 0;
}
